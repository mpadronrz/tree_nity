#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <limits>

namespace Protocol {
	inline constexpr size_t MAX_PAYLOAD_SIZE = 1024;
	inline constexpr uint32_t OFFSET_UNSET = std::numeric_limits<uint32_t>::max();
	inline constexpr uint32_t SHUTDOWN_OFFSET = std::numeric_limits<uint32_t>::max();

	enum class	CommandType : uint8_t {
		CREATE = 1,
		LIST = 2,
		INFO = 3,
		CONNECT = 4,
		PRODUCE = 5,
		SUBSCRIBE = 6,
		ACK = 7,
		DISCONNECT = 8
	};

	enum class	StatusCode : uint8_t {
		SUCCESS = 0,
		GENERAL_ERROR = 1,
		TOPIC_CLIENT_ERR = 2,
		IPC_ERROR = 3
	};

	struct	ManagementRequestHeader {
		uint8_t	action;
		uint32_t	payload_len;
	};

	struct	ManagmentResponseHeader {
		uint8_t	status_code;
		uint32_t	payload_len;
	};

	void	write_uint32_le(std::vector<uint8_t>& buf, uint32_t val);
	[[nodiscard]] uint32_t read_uint32_le(const uint8_t* buf);

	namespace Request {
		[[nodiscard]] std::vector<uint8_t>	serialize_create(uint32_t pid, std::string_view topic);
		[[nodiscard]] std::vector<uint8_t>	serialize_list(uint32_t pid);
		[[nodiscard]] std::vector<uint8_t>	serialize_info(uint32_t pid, std::string_view client_id);
		[[nodiscard]] std::vector<uint8_t>	serialize_connect(uint32_t pid, std::string_view topic);
		[[nodiscard]] std::vector<uint8_t>	serialize_produce(std::string_view topic, std::string_view key, std::string_view val);
		[[nodiscard]] std::vector<uint8_t>	serialize_subscribe(uint32_t pid, std::string_view client_id, std::string_view topic, std::string_view prefix, uint32_t offset);
		[[nodiscard]] std::vector<uint8_t>	serialize_ack(std::string_view client_id, uint32_t next_offset);
		[[nodiscard]] std::vector<uint8_t>	serialize_disconnect(std::string_view client_id);

		bool	parse_create(const uint8_t* buf, size_t len, uint32_t& out_pid, std::string& out_topic);
		bool	parse_list(const uint8_t* buf, size_t len, uint32_t& out_pid);
		bool	parse_info(const uint8_t* buf, size_t len, uint32_t& out_pid, std::string& out_client_id);
		bool	parse_connect(const uint8_t* buf, size_t len, uint32_t& out_pid, std::string& out_topic);
		bool	parse_produce(const uint8_t* buf, size_t len, std::string& out_topic, std::string& out_key, std::string& out_val);
		bool	parse_subscribe(const uint8_t* buf, size_t len, uint32_t& pid, std::string& out_client_id, std::string& out_topic, std::string& out_prefix, uint32_t& out_offset);
		bool	parse_ack(const uint8_t* buf, size_t len, std::string& out_client_id, uint32_t& out_next_offset);
		bool	parse_disconnect(const uint8_t* buf, size_t len, std::string& out_client_id);
	}

	namespace Response {

		[[nodiscard]] std::vector<uint8_t>	serialize_response(StatusCode status, std::string_view payload = "");
		[[nodiscard]] std::vector<uint8_t>	serialize_delivery(uint32_t offset,
															  std::string_view key,
															  std::string_view val);
		[[nodiscard]] std::vector<uint8_t>	serialize_shutdown();

		bool	parse_response(const uint8_t* buf, size_t len, StatusCode& out_status, std::string& out_payload);
		bool	parse_delivery(const uint8_t* buf, size_t len,
							uint32_t& out_offset,
							std::string& out_key,
							std::string& out_val);


		[[nodiscard]] inline bool	is_shutdown(uint32_t offset) noexcept {
			return offset == SHUTDOWN_OFFSET;
		}
	}
}
