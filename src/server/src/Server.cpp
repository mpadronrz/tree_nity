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

	bool decode_single_string(const uint8_t* payload, size_t len, std::string& out) {
		if (len < 4) return false;
		uint32_t str_len = Protocol::read_uint32_le(payload);
		if (len - 4 != str_len) return false;
		out.assign(reinterpret_cast<const char*>(payload + 4), str_len);
		return true;
	}

	bool decode_string_at(const uint8_t* payload, size_t len, size_t& pos, std::string& out) {
		if (len - pos < 4) return false;
		uint32_t str_len = Protocol::read_uint32_le(payload + pos);
		pos += 4;
		if (len - pos < str_len) return false;
		out.assign(reinterpret_cast<const char*>(payload + pos), str_len);
		pos += str_len;
		return true;
	}

	bool decode_two_strings(const uint8_t* payload, size_t len, std::string& out1, std::string& out2) {
		size_t pos = 0;
		if (!decode_string_at(payload, len, pos, out1)) return false;
		if (!decode_string_at(payload, len, pos, out2)) return false;
		return pos == len;
	}

	bool decode_subscriber_payload(const uint8_t* payload, size_t len,
								   std::string& topic, std::string& subscriber,
								   std::string& prefix, bool& has_offset, uint32_t& offset) {
		size_t pos = 0;
		if (!decode_string_at(payload, len, pos, topic)) return false;
		if (!decode_string_at(payload, len, pos, subscriber)) return false;
		if (!decode_string_at(payload, len, pos, prefix)) return false;
		if (len - pos < 5) return false;
		has_offset = (payload[pos] != 0);
		pos += 1;
		offset = Protocol::read_uint32_le(payload + pos);
		pos += 4;
		return pos == len;
	}

	bool decode_commit_payload(const uint8_t* payload, size_t len, std::string& subscriber, uint32_t& offset) {
		size_t pos = 0;
		if (!decode_string_at(payload, len, pos, subscriber)) return false;
		if (len - pos != 4) return false;
		offset = Protocol::read_uint32_le(payload + pos);
		return true;
	}
}

Server::Server(const std::string& path):
	ipc_path(path),
	running(true)
{
	unlink(ipc_path.c_str());
	if (mkfifo(ipc_path.c_str(), 0666) == -1)
	{
		// Fallo al crear FIFO
	}
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
}

void Server::send_response(uint32_t client_pid, Protocol::StatusCode status, std::string_view payload)
{
	std::string response_fifo = "/tmp/treenity.client." + std::to_string(client_pid);

	int fd = open(response_fifo.c_str(), O_WRONLY);
	if (fd != -1)
	{
		uint32_t total_length = static_cast<uint32_t>(5 + payload.size());
		std::vector<uint8_t> res;
		res.reserve(total_length);
		res.push_back(static_cast<uint8_t>(status));
		Protocol::write_uint32_le(res, total_length);
		res.insert(res.end(), payload.begin(), payload.end());

		write(fd, res.data(), res.size());
		close(fd);
	}
}

void Server::run()
{
	int fifo_fd = open(ipc_path.c_str(), O_RDWR);
	if (fifo_fd == -1)
		return;

	uint8_t buffer[1024];
	std::vector<uint8_t> recv_buf;

	while (running)
	{
		ssize_t bytes_read = read(fifo_fd, buffer, sizeof(buffer));

		if (bytes_read > 0)
		{
			recv_buf.insert(recv_buf.end(), buffer, buffer + bytes_read);

			// Cabecera de petición del cliente: 9 bytes (action: 1B, client_pid: 4B LE, total_length: 4B LE)
			while (recv_buf.size() >= 9)
			{
				uint8_t action = recv_buf[0];
				uint32_t client_pid = Protocol::read_uint32_le(recv_buf.data() + 1);
				uint32_t total_length = Protocol::read_uint32_le(recv_buf.data() + 5);

				if (total_length < 9)
				{
					recv_buf.erase(recv_buf.begin());
					continue;
				}

				if (recv_buf.size() < total_length)
					break; // Esperamos a que lleguen los bytes restantes de la trama

				const uint8_t* payload = recv_buf.data() + 9;
				size_t payload_len = total_length - 9;

				handle_request(action, client_pid, payload, payload_len);

				recv_buf.erase(recv_buf.begin(), recv_buf.begin() + total_length);
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

void Server::handle_request(uint8_t action, uint32_t client_pid, const uint8_t* payload, size_t payload_len)
{
	switch (action)
	{
		case 1: // CreateTopic
			handle_create(client_pid, payload, payload_len);
			break;
		case 2: // ListTopics
			handle_list(client_pid, payload, payload_len);
			break;
		case 3: // ClientInfo
			handle_info(client_pid, payload, payload_len);
			break;
		case 4: // ProducerConnect
			handle_producer_connect(client_pid, payload, payload_len);
			break;
		case 5: // Publish
			handle_publish(client_pid, payload, payload_len);
			break;
		case 6: // SubscriberConnect
			handle_subscriber_connect(client_pid, payload, payload_len);
			break;
		case 7: // SubscriberCommit
			handle_commit(client_pid, payload, payload_len);
			break;
		case 8: // Disconnect
			handle_disconnect(client_pid, payload, payload_len);
			break;
		default:
			break;
	}
}

void Server::handle_create(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	std::string topic_name;
	if (!decode_single_string(payload, len, topic_name))
	{
		send_response(client_pid, Protocol::StatusCode::GENERAL_ERROR);
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

void Server::handle_list(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	(void)payload;
	(void)len;

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

void Server::handle_info(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	std::string client_id;
	if (!decode_single_string(payload, len, client_id))
	{
		send_response(client_pid, Protocol::StatusCode::GENERAL_ERROR);
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

void Server::handle_producer_connect(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	std::string topic_name;
	if (!decode_single_string(payload, len, topic_name))
	{
		send_response(client_pid, Protocol::StatusCode::GENERAL_ERROR);
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

void Server::handle_publish(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	std::string key, value;
	if (!decode_two_strings(payload, len, key, value))
	{
		send_response(client_pid, Protocol::StatusCode::GENERAL_ERROR);
		return;
	}

	std::string topic_name;
	{
		std::lock_guard<std::mutex> lock(server_mutex);
		std::string* found_topic = producer_topics.find(client_pid);
		if (found_topic != nullptr)
			topic_name = *found_topic;
	}

	Topic* topic = get_topic(topic_name);
	if (topic == nullptr)
	{
		send_response(client_pid, Protocol::StatusCode::TOPIC_CLIENT_ERR);
		return;
	}

	topic->append_message(key, value);
	send_response(client_pid, Protocol::StatusCode::SUCCESS);
}

void Server::handle_subscriber_connect(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	std::string topic_name, client_id, prefix;
	bool has_offset = false;
	uint32_t req_offset = 0;

	if (!decode_subscriber_payload(payload, len, topic_name, client_id, prefix, has_offset, req_offset))
	{
		send_response(client_pid, Protocol::StatusCode::GENERAL_ERROR);
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
	if (has_offset)
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

void Server::handle_commit(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	(void)client_pid;
	std::string client_id;
	uint32_t next_offset = 0;

	if (!decode_commit_payload(payload, len, client_id, next_offset))
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

void Server::handle_disconnect(uint32_t client_pid, const uint8_t* payload, size_t len)
{
	(void)client_pid;
	std::string client_id;
	if (!decode_single_string(payload, len, client_id))
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
