#include "server/inc/Topic.hpp"
#include "protocol/Protocol.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

Topic::Topic(const std::string& topic_name):
	name(topic_name),
	stop_flag(false)
{
	worker_thread = std::thread(&Topic::run, this);
}

Topic::~Topic()
{
	shutdown();
}

void Topic::shutdown()
{
	{
		std::lock_guard<std::mutex> lock(topic_mutex);
		if (stop_flag)
			return;
		stop_flag = true;

		// Sending message to all clients
		auto shutdown_buf = Protocol::Response::serialize_shutdown();
		for (auto& pair : subscribers)
		{
			ActiveSubscriber& sub = pair.second;
			if (sub.fifo_fd != -1)
			{
				write(sub.fifo_fd, shutdown_buf.data(), shutdown_buf.size());
				close(sub.fifo_fd);
				sub.fifo_fd = -1;
			}
		}
		subscribers.clear();
		prefix_tree.clear();
	}
	cv.notify_all();

	if (worker_thread.joinable())
		worker_thread.join();
}

bool Topic::add_subscriber(const std::string& client_id, const std::string& prefix, const std::string& fifo_path, uint32_t start_offset)
{
	std::lock_guard<std::mutex> lock(topic_mutex);

	ActiveSubscriber* old_sub = subscribers.find(client_id);
	if (old_sub != nullptr) {
		prefix_tree.remove(old_sub->prefix, client_id);
		if (old_sub->fifo_fd != -1)
		{
			close(old_sub->fifo_fd);
			old_sub->fifo_fd = -1;
		}
		subscribers.erase(client_id);
	}

	int fd = open(fifo_path.c_str(), O_WRONLY | O_NONBLOCK);
	if (fd == -1)
		return false;

	prefix_tree.insert(prefix, client_id);

	ActiveSubscriber sub;
	sub.client_id = client_id;
	sub.prefix = prefix;
	sub.fifo_path = fifo_path;
	sub.fifo_fd = fd;
	sub.next_offset = start_offset;

	subscribers.insert(client_id, sub);

	cv.notify_all();
	return true;
}

bool Topic::remove_subscriber(const std::string& client_id)
{
	std::lock_guard<std::mutex> lock(topic_mutex);

	ActiveSubscriber* sub = subscribers.find(client_id);
	if (sub == nullptr)
		return false;

	prefix_tree.remove(sub->prefix, client_id);
	if (sub->fifo_fd != -1)
	{
		close(sub->fifo_fd);
		sub->fifo_fd = -1;
	}

	subscribers.erase(client_id);
	return true;
}

void Topic::append_message(const std::string& key, const std::string& value)
{
	std::lock_guard<std::mutex> lock(topic_mutex);

	uint32_t current_offset = static_cast<uint32_t>(messages.size());

	Message new_msg;
	new_msg.offset = current_offset;
	new_msg.key = key;
	new_msg.value = value;

	messages.push_back(new_msg);

	cv.notify_all();
}

std::vector<Message> Topic::get_messages_from(uint32_t start_offset)
{
	std::lock_guard<std::mutex> lock(topic_mutex);

	std::vector<Message> result;
	if (start_offset < messages.size()) {
		result.assign(messages.begin() + start_offset, messages.end());
	}

	return result;
}

void Topic::run()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(topic_mutex);

		auto has_work = [this]() {
			if (stop_flag)
				return true;
			for (auto& pair : subscribers) {
				if (pair.second.next_offset < messages.size())
					return true;
			}
			return false;
		};

		cv.wait(lock, has_work);

		if (stop_flag)
			break;

		for (auto& pair : subscribers)
		{
			ActiveSubscriber& sub = pair.second;
			while (sub.next_offset < messages.size())
			{
				const Message& msg = messages[sub.next_offset];

				// Matching the message key with the sub
				auto matching_clients = prefix_tree.match(msg.key);
				bool is_matched = std::find(matching_clients.begin(), matching_clients.end(), sub.client_id) != matching_clients.end();

				if (is_matched && sub.fifo_fd != -1)
				{
					auto delivery_buf = Protocol::Response::serialize_delivery(msg.offset, msg.key, msg.value);
					ssize_t ret = write(sub.fifo_fd, delivery_buf.data(), delivery_buf.size());
					if (ret <= 0)
					{
						close(sub.fifo_fd);
						sub.fifo_fd = -1;
					}
				}

				sub.next_offset++;
			}
		}
	}
}
