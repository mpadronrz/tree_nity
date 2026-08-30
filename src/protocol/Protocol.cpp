#include "protocol/Protocol.hpp"

namespace Protocol {
	void	write_uint32_le(std::vector<uint8_t>& buf, uint32_t val) {
		buf.push_back(static_cast<uint8_t>(val & 0xFF));
		buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
		buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
		buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
	}

	uint32_t read_uint32_le(const uint8_t* buf) {
		return static_cast<uint32_t>(buf[0]) |
				(static_cast<uint32_t>(buf[1]) << 8) |
				(static_cast<uint32_t>(buf[2]) << 16) |
				(static_cast<uint32_t>(buf[3]) << 24);
	}

	namespace Request {
		std::vector<uint8_t>	serialize_create(uint32_t pid, std::string_view topic) {
			uint32_t	payload_len = static_cast<uint32_t>(4 + topic.size());
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::CREATE));
			write_uint32_le(buf, payload_len);
			write_uint32_le(buf, pid);
			buf.insert(buf.end(), topic.begin(), topic.end());

			return buf;
		}

		std::vector<uint8_t>	serialize_list(uint32_t pid) {
			uint32_t	payload_len = 4;
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::LIST));
			write_uint32_le(buf, payload_len);
			write_uint32_le(buf, pid);

			return buf;
		}

		std::vector<uint8_t>	serialize_info(uint32_t pid, std::string_view client_id) {
			uint32_t	payload_len = static_cast<uint32_t>(4 + client_id.size());
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::INFO));
			write_uint32_le(buf, payload_len);
			write_uint32_le(buf, pid);
			buf.insert(buf.end(), client_id.begin(), client_id.end());

			return buf;
		}

		std::vector<uint8_t>	serialize_connect(uint32_t pid, std::string_view topic) {
			uint32_t	payload_len = static_cast<uint32_t>(4 + topic.size());
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::CONNECT));
			write_uint32_le(buf, payload_len);
			write_uint32_le(buf, pid);
			buf.insert(buf.end(), topic.begin(), topic.end());

			return buf;
		}

		std::vector<uint8_t>	serialize_produce(std::string_view topic, std::string_view key, std::string_view val) {
			uint32_t	payload_len = static_cast<uint32_t>(12 + topic.size() + key.size() + val.size());
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::PRODUCE));
			write_uint32_le(buf, payload_len);

			write_uint32_le(buf, static_cast<uint32_t>(topic.size()));
			buf.insert(buf.end(), topic.begin(), topic.end());
			write_uint32_le(buf, static_cast<uint32_t>(key.size()));
			buf.insert(buf.end(), key.begin(), key.end());
			write_uint32_le(buf, static_cast<uint32_t>(val.size()));
			buf.insert(buf.end(), val.begin(), val.end());

			return buf;
		}

		std::vector<uint8_t>	serialize_subscribe(uint32_t pid, std::string_view client_id, std::string_view topic, std::string_view prefix, uint32_t offset) {
			uint32_t	payload_len = static_cast<uint32_t>(20 + client_id.size() + topic.size() + prefix.size());
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::SUBSCRIBE));
			write_uint32_le(buf, payload_len);
			write_uint32_le(buf, pid);

			write_uint32_le(buf, static_cast<uint32_t>(client_id.size()));
			buf.insert(buf.end(), client_id.begin(), client_id.end());
			write_uint32_le(buf, static_cast<uint32_t>(topic.size()));
			buf.insert(buf.end(), topic.begin(), topic.end());
			write_uint32_le(buf, static_cast<uint32_t>(prefix.size()));
			buf.insert(buf.end(), prefix.begin(), prefix.end());
			write_uint32_le(buf, offset);

			return buf;
		}

		std::vector<uint8_t>	serialize_ack(std::string_view client_id, uint32_t next_offset) {
			uint32_t	payload_len = static_cast<uint32_t>(8 + client_id.size());
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::ACK));
			write_uint32_le(buf, payload_len);

			write_uint32_le(buf, static_cast<uint32_t>(client_id.size()));
			buf.insert(buf.end(), client_id.begin(), client_id.end());
			write_uint32_le(buf, next_offset);

			return buf;
		}

		std::vector<uint8_t>	serialize_disconnect(std::string_view client_id) {
			uint32_t	payload_len = static_cast<uint32_t>(4 + client_id.size());
			std::vector<uint8_t>	buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(CommandType::DISCONNECT));
			write_uint32_le(buf, payload_len);
			write_uint32_le(buf, static_cast<uint32_t>(client_id.size()));
			buf.insert(buf.end(), client_id.begin(), client_id.end());

			return buf;
		}

		bool parse_create(const uint8_t* buf, size_t len, uint32_t& out_pid, std::string& out_topic) {
			if (len < 4) {
				return false;
			}

			out_pid = read_uint32_le(buf);
			size_t topic_len = len - 4;

			out_topic.assign(reinterpret_cast<const char*>(buf + 4), topic_len);
			return true;
		}

		bool parse_list(const uint8_t* buf, size_t len, uint32_t& out_pid) {
			if (len != 4) {
				return false;
			}

			out_pid = read_uint32_le(buf);
			return true;
		}

		bool parse_info(const uint8_t* buf, size_t len, uint32_t& out_pid, std::string& out_client_id) {
			if (len < 4) {
				return false;
			}

			out_pid = read_uint32_le(buf);
			size_t id_len = len - 4;

			out_client_id.assign(reinterpret_cast<const char*>(buf + 4), id_len);
			return true;
		}

		bool parse_connect(const uint8_t* buf, size_t len, uint32_t& out_pid, std::string& out_topic) {
			if (len < 4) {
				return false;
			}

			out_pid = read_uint32_le(buf);
			size_t topic_len = len - 4;

			out_topic.assign(reinterpret_cast<const char*>(buf + 4), topic_len);
			return true;
		}

		bool parse_produce(const uint8_t* buf, size_t len,
						std::string& out_topic, std::string& out_key, std::string& out_val) {
			size_t offset = 0;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t topic_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + topic_len > len) {
				return false;
			}
			out_topic.assign(reinterpret_cast<const char*>(buf + offset), topic_len);
			offset += topic_len;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t key_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + key_len > len) {
				return false;
			}
			out_key.assign(reinterpret_cast<const char*>(buf + offset), key_len);
			offset += key_len;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t val_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + val_len > len) {
				return false;
			}
			out_val.assign(reinterpret_cast<const char*>(buf + offset), val_len);
			offset += val_len;

			if (offset != len) {
				return false;
			}
			if (key_len + val_len > MAX_PAYLOAD_SIZE) {
				return false;
			}

			return true;
		}

		bool parse_subscribe(const uint8_t* buf, size_t len,
							uint32_t& out_pid,
							std::string& out_client_id,
							std::string& out_topic,
							std::string& out_prefix,
							uint32_t& out_offset) {
			size_t offset = 0;

			if (offset + 4 > len) {
				return false;
			}
			out_pid = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t id_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + id_len > len) {
				return false;
			}
			out_client_id.assign(reinterpret_cast<const char*>(buf + offset), id_len);
			offset += id_len;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t topic_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + topic_len > len) {
				return false;
			}
			out_topic.assign(reinterpret_cast<const char*>(buf + offset), topic_len);
			offset += topic_len;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t pref_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + pref_len > len) {
				return false;
			}
			out_prefix.assign(reinterpret_cast<const char*>(buf + offset), pref_len);
			offset += pref_len;

			if (offset + 4 > len) {
				return false;
			}
			out_offset = read_uint32_le(buf + offset);
			offset += 4;

			return offset == len;
		}

		bool parse_ack(const uint8_t* buf, size_t len, std::string& out_client_id, uint32_t& out_next_offset) {
			size_t offset = 0;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t id_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + id_len > len) {
				return false;
			}
			out_client_id.assign(reinterpret_cast<const char*>(buf + offset), id_len);
			offset += id_len;

			if (offset + 4 > len) {
				return false;
			}
			out_next_offset = read_uint32_le(buf + offset);
			offset += 4;

			return offset == len;
		}

		bool parse_disconnect(const uint8_t* buf, size_t len, std::string& out_client_id) {
			size_t offset = 0;

			if (offset + 4 > len) {
				return false;
			}
			uint32_t id_len = read_uint32_le(buf + offset);
			offset += 4;

			if (offset + id_len > len) {
				return false;
			}
			out_client_id.assign(reinterpret_cast<const char*>(buf + offset), id_len);
			offset += id_len;

			return offset == len;
		}
	}

	namespace Response {
		std::vector<uint8_t>	serialize_response(StatusCode status, std::string_view payload) {
			uint32_t payload_len = static_cast<uint32_t>(payload.size());
			std::vector<uint8_t> buf;
			buf.reserve(5 + payload_len);

			buf.push_back(static_cast<uint8_t>(status));
			write_uint32_le(buf, payload_len);
			buf.insert(buf.end(), payload.begin(), payload.end());

			return buf;
		}

		std::vector<uint8_t>	serialize_delivery(uint32_t offset,
												std::string_view key,
												std::string_view val) {
			uint32_t key_len = static_cast<uint32_t>(key.size());
			uint32_t val_len = static_cast<uint32_t>(val.size());

			std::vector<uint8_t> buf;
			buf.reserve(12 + key_len + val_len);

			write_uint32_le(buf, offset);
			write_uint32_le(buf, key_len);
			write_uint32_le(buf, val_len);
			buf.insert(buf.end(), key.begin(), key.end());
			buf.insert(buf.end(), val.begin(), val.end());

			return buf;
		}

		std::vector<uint8_t>	serialize_shutdown() {
			return serialize_delivery(std::numeric_limits<uint32_t>::max(), "", "");
		}

		bool	parse_response(const uint8_t* buf, size_t len, StatusCode& out_status, std::string& out_payload) {
			if (len < 5) {
				return false;
			}

			out_status = static_cast<StatusCode>(buf[0]);
			uint32_t payload_len = read_uint32_le(buf + 1);

			if (len - 5 != payload_len) {
				return false;
			}

			out_payload.assign(reinterpret_cast<const char*>(buf + 5), payload_len);
			return true;
		}

		bool parse_delivery(const uint8_t* buf, size_t len,
							uint32_t& out_offset,
							std::string& out_key,
							std::string& out_val) {
			size_t cursor = 0;

			if (cursor + 4 > len) {
				return false;
			}
			out_offset = read_uint32_le(buf + cursor);
			cursor += 4;

			if (cursor + 4 > len) {
				return false;
			}
			uint32_t key_len = read_uint32_le(buf + cursor);
			cursor += 4;

			if (cursor + 4 > len) {
				return false;
			}
			uint32_t val_len = read_uint32_le(buf + cursor);
			cursor += 4;

			if (out_offset != SHUTDOWN_OFFSET && key_len + val_len > MAX_PAYLOAD_SIZE) {
				return false;
			}

			if (cursor + key_len > len) {
				return false;
			}
			out_key.assign(reinterpret_cast<const char*>(buf + cursor), key_len);
			cursor += key_len;

			if (cursor + val_len > len) {
				return false;
			}
			out_val.assign(reinterpret_cast<const char*>(buf + cursor), val_len);
			cursor += val_len;

			return cursor == len;
		}
	}
}
