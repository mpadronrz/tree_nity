#include "server/inc/Server.hpp"
#include "protocol/Protocol.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include <cctype>

namespace {
	bool is_valid_identifier(const std::string& str) {
		if (str.empty() || str.size() > 32)
			return false;
		for (char c : str) {
			if (!isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.' && c != '-')
				return false;
		}
		return true;
	}
}

Server::Server(const std::string& path):
	ipc_path(path),
	running(true)
{
	unlink(ipc_path.c_str());
	if (mkfifo(ipc_path.c_str(), 0666) == -1) {
		correct_start = false;
		return;
	}
	else
		correct_start = true;
	fifo_fd = open(ipc_path.c_str(), O_RDWR);
	if (fifo_fd == -1)
		correct_start = false;
}

Server::~Server()
{
	stop();
}

void Server::stop()
{
	if (!running)
		return;
	running = false;

	unlink(ipc_path.c_str());

	// Escribimos un byte en la FIFO para despertar cualquier read() bloqueado
	int dummy_fd = open(ipc_path.c_str(), O_WRONLY | O_NONBLOCK);
	if (dummy_fd != -1)
	{
		write(dummy_fd, "\0", 1);
		close(dummy_fd);
	}

	std::lock_guard<std::mutex> lock(server_mutex);
	for (auto& pair : topics)
	{
		if (pair.second)
			pair.second->shutdown();
	}
	close(fifo_fd);
}

void Server::send_response(uint32_t client_pid, Protocol::StatusCode status, std::string_view payload)
{
	std::string response_fifo = "/tmp/treenity.client." + std::to_string(client_pid);

	int fd = open(response_fifo.c_str(), O_WRONLY);
	if (fd != -1)
	{
		auto res = Protocol::Response::serialize_response(status, payload);
		write(fd, res.data(), res.size());
		close(fd);
	}
}

void Server::run()
{
	uint8_t buffer[1024];
	std::vector<uint8_t> recv_buf;

	while (running)
	{
		ssize_t bytes_read = read(fifo_fd, buffer, sizeof(buffer));

		if (bytes_read > 0)
		{
			recv_buf.insert(recv_buf.end(), buffer, buffer + bytes_read);

			// Cabecera de petición del cliente: 5 bytes (action: 1B, payload_len: 4B LE)
			while (recv_buf.size() >= 5)
			{
				uint8_t action = recv_buf[0];
				uint32_t payload_len = Protocol::read_uint32_le(recv_buf.data() + 1);

				if (recv_buf.size() < 5 + payload_len)
					break; // Esperamos a que lleguen los bytes restantes de la trama

				const uint8_t* payload = recv_buf.data() + 5;

				handle_request(action, payload, payload_len);

				recv_buf.erase(recv_buf.begin(), recv_buf.begin() + 5 + payload_len);
			}
		}
		else if (bytes_read <= 0)
		{
			if (!running)
				break;
		}
	}

	close(fifo_fd);
}

void Server::handle_request(uint8_t action, const uint8_t* payload, size_t payload_len)
{
	switch (action)
	{
		case 1: // CreateTopic
			handle_create(payload, payload_len);
			break;
		case 2: // ListTopics
			handle_list(payload, payload_len);
			break;
		case 3: // ClientInfo
			handle_info(payload, payload_len);
			break;
		case 4: // ProducerConnect
			handle_producer_connect(payload, payload_len);
			break;
		case 5: // Publish
			handle_publish(payload, payload_len);
			break;
		case 6: // SubscriberConnect
			handle_subscriber_connect(payload, payload_len);
			break;
		case 7: // SubscriberCommit
			handle_commit(payload, payload_len);
			break;
		case 8: // Disconnect
			handle_disconnect(payload, payload_len);
			break;
		default:
			break;
	}
}

void Server::handle_create(const uint8_t* payload, size_t len)
{
	uint32_t client_pid = 0;
	std::string topic_name;
	if (!Protocol::Request::parse_create(payload, len, client_pid, topic_name))
	{
		return;
	}

	if (!is_valid_identifier(topic_name))
	{
		send_response(client_pid, Protocol::StatusCode::GENERAL_ERROR);
		return;
	}

	if (!create_topic(topic_name))
	{
		send_response(client_pid, Protocol::StatusCode::TOPIC_CLIENT_ERR);
		return;
	}

	send_response(client_pid, Protocol::StatusCode::SUCCESS);
}

void Server::handle_list(const uint8_t* payload, size_t len)
{
	uint32_t client_pid = 0;
	if (!Protocol::Request::parse_list(payload, len, client_pid))
	{
		return;
	}

	std::lock_guard<std::mutex> lock(server_mutex);
	std::string result;
	bool first = true;

	for (auto& pair : topics)
	{
		if (!first)
			result += ",";
		result += pair.first;
		first = false;
	}

	send_response(client_pid, Protocol::StatusCode::SUCCESS, result);
}

void Server::handle_info(const uint8_t* payload, size_t len)
{
	uint32_t client_pid = 0;
	std::string client_id;
	if (!Protocol::Request::parse_info(payload, len, client_pid, client_id))
	{
		return;
	}

	std::lock_guard<std::mutex> lock(server_mutex);
	ClientMetadata* meta = client_index.find(client_id);
	if (meta == nullptr)
	{
		send_response(client_pid, Protocol::StatusCode::TOPIC_CLIENT_ERR);
		return;
	}

	std::string json = "{\"client\":\"" + meta->client_id +
					   "\",\"topic\":\"" + meta->topic +
					   "\",\"offset\":" + std::to_string(meta->offset) +
					   ",\"prefix\":\"" + meta->prefix +
					   "\",\"ipc\":\"" + meta->ipc_path + "\"}";

	send_response(client_pid, Protocol::StatusCode::SUCCESS, json);
}

void Server::handle_producer_connect(const uint8_t* payload, size_t len)
{
	uint32_t client_pid = 0;
	std::string topic_name;
	if (!Protocol::Request::parse_connect(payload, len, client_pid, topic_name))
	{
		return;
	}

	Topic* topic = get_topic(topic_name);
	if (topic == nullptr)
	{
		send_response(client_pid, Protocol::StatusCode::TOPIC_CLIENT_ERR);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(server_mutex);
		producer_topics.insert_or_assign(client_pid, topic_name);
	}

	send_response(client_pid, Protocol::StatusCode::SUCCESS);
}

void Server::handle_publish(const uint8_t* payload, size_t len)
{
	std::string topic_name, key, value;
	if (!Protocol::Request::parse_produce(payload, len, topic_name, key, value))
	{
		return;
	}

	Topic* topic = nullptr;
	if (!topic_name.empty()) {
		topic = get_topic(topic_name);
	}
	if (topic == nullptr)
	{
		return;
	}

	topic->append_message(key, value);
}

void Server::handle_subscriber_connect(const uint8_t* payload, size_t len)
{
	uint32_t client_pid = 0;
	std::string topic_name, client_id, prefix;
	uint32_t req_offset = Protocol::OFFSET_UNSET;

	if (!Protocol::Request::parse_subscribe(payload, len, client_pid, client_id, topic_name, prefix, req_offset))
	{
		return;
	}

	if (!is_valid_identifier(client_id) || !is_valid_identifier(topic_name))
	{
		send_response(client_pid, Protocol::StatusCode::GENERAL_ERROR);
		return;
	}

	std::lock_guard<std::mutex> lock(server_mutex);

	std::unique_ptr<Topic>* found_topic = topics.find(topic_name);
	if (found_topic == nullptr)
	{
		send_response(client_pid, Protocol::StatusCode::TOPIC_CLIENT_ERR);
		return;
	}

	ClientMetadata* meta = client_index.find(client_id);
	if (meta != nullptr && meta->is_active)
	{
		// Suscriptor duplicado ya activo
		send_response(client_pid, Protocol::StatusCode::TOPIC_CLIENT_ERR);
		return;
	}

	uint32_t start_offset = 0;
	if (req_offset != Protocol::OFFSET_UNSET)
	{
		start_offset = req_offset;
	}
	else if (meta != nullptr)
	{
		start_offset = meta->offset;
	}

	std::string consumer_ipc = "/tmp/treenity.client." + std::to_string(client_pid);

	ClientMetadata new_meta;
	new_meta.client_id = client_id;
	new_meta.topic = topic_name;
	new_meta.offset = start_offset;
	new_meta.prefix = prefix;
	new_meta.ipc_path = consumer_ipc;
	new_meta.is_active = true;

	client_index.insert_or_assign(client_id, new_meta);

	// Responder SUCCESS al cliente síncrono
	send_response(client_pid, Protocol::StatusCode::SUCCESS);

	// Dar de alta al suscriptor en el Topic
	(*found_topic)->add_subscriber(client_id, prefix, consumer_ipc, start_offset);
}

void Server::handle_commit(const uint8_t* payload, size_t len)
{
	std::string client_id;
	uint32_t next_offset = 0;

	if (!Protocol::Request::parse_ack(payload, len, client_id, next_offset))
	{
		return;
	}

	std::lock_guard<std::mutex> lock(server_mutex);
	ClientMetadata* meta = client_index.find(client_id);
	if (meta != nullptr)
	{
		meta->offset = next_offset;
	}
}

void Server::handle_disconnect(const uint8_t* payload, size_t len)
{
	std::string client_id;
	if (!Protocol::Request::parse_disconnect(payload, len, client_id))
	{
		return;
	}

	std::lock_guard<std::mutex> lock(server_mutex);
	ClientMetadata* meta = client_index.find(client_id);
	if (meta != nullptr)
	{
		meta->is_active = false;

		std::unique_ptr<Topic>* found_topic = topics.find(meta->topic);
		if (found_topic != nullptr)
		{
			(*found_topic)->remove_subscriber(client_id);
		}
	}
}

bool Server::create_topic(const std::string& topic_name)
{
	std::lock_guard<std::mutex> lock(server_mutex);

	if (topics.contains(topic_name))
		return false;

	topics[topic_name] = std::make_unique<Topic>(topic_name);
	return true;
}

Topic* Server::get_topic(const std::string& topic_name)
{
	std::lock_guard<std::mutex> lock(server_mutex);

	std::unique_ptr<Topic>* found = topics.find(topic_name);
	if (found != nullptr)
		return found->get();

	return nullptr;
}

bool	Server::is_correct_start() const noexcept
{
	return correct_start;
}
