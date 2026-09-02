#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <vector>

#include "data_structures/HashMap.hpp"
#include "protocol/Protocol.hpp"
#include "server/inc/Topic.hpp"

struct ClientMetadata
{
	std::string	client_id;
	std::string	topic;
	uint32_t	offset = 0;
	std::string	prefix;
	std::string	ipc_path;
	bool		is_active = false;
};

class Server
{
	private:
		// Used to guess if the server fifo was created
		bool											correct_start;
		int												fifo_fd;

		std::string										ipc_path;

		// Active topics
		HashMap<std::string, std::unique_ptr<Topic>>	topics;

		// Client metadata
		HashMap<std::string, ClientMetadata>			client_index;

		// Assignes the producer pid to its connected topic.
		HashMap<uint32_t, std::string>					producer_topics;

		// For topic and metadata
		std::mutex										server_mutex;

		// For the main loop
		std::atomic<bool>								running{true};

		void	send_response(uint32_t client_pid, Protocol::StatusCode status, std::string_view payload = "");

		void	handle_request(uint8_t action, const uint8_t* payload, size_t payload_len);
		void	handle_create(const uint8_t* payload, size_t len);
		void	handle_list(const uint8_t* payload, size_t len);
		void	handle_info(const uint8_t* payload, size_t len);
		void	handle_producer_connect(const uint8_t* payload, size_t len);
		void	handle_publish(const uint8_t* payload, size_t len);
		void	handle_subscriber_connect(const uint8_t* payload, size_t len);
		void	handle_commit(const uint8_t* payload, size_t len);
		void	handle_disconnect(const uint8_t* payload, size_t len);

	public:
		explicit	Server(const std::string& path);
		~Server();

		void	run();
		void	stop();

		bool	create_topic(const std::string& topic_name);
		Topic*	get_topic(const std::string& topic_name);

		bool	is_correct_start() const noexcept;
};
