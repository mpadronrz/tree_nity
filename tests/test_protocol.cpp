#include <gtest/gtest.h>

#include "protocol/Protocol.hpp"



// ============================================================================
// Suite 1: Endianness Utilities
// ============================================================================


TEST(ProtocolEndianTest, LittleEndianWriteRead) {
	std::vector<uint8_t> buf;
	Protocol::write_uint32_le(buf, 0x12345678u);

	ASSERT_EQ(buf.size(), 4u);
	EXPECT_EQ(buf[0], 0x78);
	EXPECT_EQ(buf[1], 0x56);
	EXPECT_EQ(buf[2], 0x34);
	EXPECT_EQ(buf[3], 0x12);

	uint32_t val = Protocol::read_uint32_le(buf.data());
	EXPECT_EQ(val, 0x12345678u);
}

TEST(ProtocolEndianTest, LittleEndianReadWrite) {
	std::vector<uint8_t> buf = {0x43, 0x12, 0x21, 0x42, 0x98, 0x12};
	uint32_t val = Protocol::read_uint32_le(buf.data());

	EXPECT_EQ(val, 0x42211243u);

	std::vector<uint8_t> new_buf;
	Protocol::write_uint32_le(new_buf, val);
	ASSERT_EQ(new_buf.size(), 4u);
	EXPECT_EQ(new_buf[0], 0x43);
	EXPECT_EQ(new_buf[1], 0x12);
	EXPECT_EQ(new_buf[2], 0x21);
	EXPECT_EQ(new_buf[3], 0x42);
}

TEST(ProtocolEndianTest, BoundaryValues) {
	std::vector<uint8_t> buf;
	Protocol::write_uint32_le(buf, 0);
	Protocol::write_uint32_le(buf, std::numeric_limits<uint32_t>::max());

	ASSERT_EQ(buf.size(), 8u);
	EXPECT_EQ(Protocol::read_uint32_le(buf.data()), 0u);
	EXPECT_EQ(Protocol::read_uint32_le(buf.data() + 4), std::numeric_limits<uint32_t>::max());
}



// ============================================================================
// Suite 2: Client Request Serialization & Parsing Round-Trips
// ============================================================================

TEST(ProtocolRequestTest, CreateRoundTrip) {
	uint32_t pid = 1337;
	std::string topic = "user_events";

	auto frame = Protocol::Request::serialize_create(pid, topic);
	ASSERT_GE(frame.size(), 5u);

	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::CREATE));
	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, frame.size() - 5);

	uint32_t out_pid = 0;
	std::string out_topic;
	EXPECT_TRUE(Protocol::Request::parse_create(frame.data() + 5, payload_len, out_pid, out_topic));
	EXPECT_EQ(out_pid, pid);
	EXPECT_EQ(out_topic, topic);
}

TEST(ProtocolRequestTest, ListRoundTrip) {
	uint32_t pid = 2468;
	auto frame = Protocol::Request::serialize_list(pid);
	ASSERT_EQ(frame.size(), 9u);

	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::LIST));
	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, 4u);

	uint32_t out_pid = 0;
	EXPECT_TRUE(Protocol::Request::parse_list(frame.data() + 5, payload_len, out_pid));
	EXPECT_EQ(out_pid, pid);
}

TEST(ProtocolRequestTest, InfoRoundTrip) {
	uint32_t pid = 42;
	std::string client_id = "client_alpha";

	auto frame = Protocol::Request::serialize_info(pid, client_id);
	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::INFO));
	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, frame.size() - 5);

	uint32_t out_pid = 0;
	std::string out_id;
	EXPECT_TRUE(Protocol::Request::parse_info(frame.data() + 5, payload_len, out_pid, out_id));
	EXPECT_EQ(out_pid, pid);
	EXPECT_EQ(out_id, client_id);
}

TEST(ProtocolRequestTest, ConnectRoundTrip) {
	uint32_t pid = 5555;
	std::string topic = "sensor_stream";

	auto frame = Protocol::Request::serialize_connect(pid, topic);
	ASSERT_GE(frame.size(), 5u);

	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::CONNECT));
	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, frame.size() - 5);

	uint32_t out_pid = 0;
	std::string out_topic;
	EXPECT_TRUE(Protocol::Request::parse_connect(frame.data() + 5, payload_len, out_pid, out_topic));
	EXPECT_EQ(out_pid, pid);
	EXPECT_EQ(out_topic, topic);
}

TEST(ProtocolRequestTest, ProduceRoundTrip) {
	std::string topic = "telemetry";
	std::string key = "sensor.temp";
	std::string val = "24.5C";

	auto frame = Protocol::Request::serialize_produce(topic, key, val);
	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::PRODUCE));
	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, frame.size() - 5);

	std::string out_topic, out_key, out_val;
	EXPECT_TRUE(Protocol::Request::parse_produce(frame.data() + 5, payload_len, out_topic, out_key, out_val));
	EXPECT_EQ(out_topic, topic);
	EXPECT_EQ(out_key, key);
	EXPECT_EQ(out_val, val);
}

TEST(ProtocolRequestTest, SubscribeRoundTrip) {
	uint32_t pid = 9999;
	std::string client_id = "client0";
	std::string topic = "orders";
	std::string prefix = "order.eu";
	uint32_t offset = 42;

	auto frame = Protocol::Request::serialize_subscribe(pid, client_id, topic, prefix, offset);
	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::SUBSCRIBE));

	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, frame.size() - 5);
	uint32_t out_pid = 0;
	std::string out_id, out_topic, out_prefix;
	uint32_t out_offset = 0;

	EXPECT_TRUE(Protocol::Request::parse_subscribe(frame.data() + 5, payload_len, out_pid, out_id, out_topic, out_prefix, out_offset));
	EXPECT_EQ(out_pid, pid);
	EXPECT_EQ(out_id, client_id);
	EXPECT_EQ(out_topic, topic);
	EXPECT_EQ(out_prefix, prefix);
	EXPECT_EQ(out_offset, offset);
}

TEST(ProtocolRequestTest, AckRoundTrip) {
	std::string client_id = "worker_node";
	uint32_t next_offset = 105;

	auto frame = Protocol::Request::serialize_ack(client_id, next_offset);
	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::ACK));
	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, frame.size() - 5);

	std::string ack_id;
	uint32_t out_next_offset = 0;

	EXPECT_TRUE(Protocol::Request::parse_ack(frame.data() + 5, payload_len, ack_id, out_next_offset));
	EXPECT_EQ(ack_id, client_id);
	EXPECT_EQ(out_next_offset, next_offset);
}

TEST(ProtocolRequestTest, DisconnectRoundTrip) {
	std::string client_id = "worker_node";

	auto frame = Protocol::Request::serialize_disconnect(client_id);
	EXPECT_EQ(frame[0], static_cast<uint8_t>(Protocol::CommandType::DISCONNECT));
	uint32_t payload_len = Protocol::read_uint32_le(frame.data() + 1);
	EXPECT_EQ(payload_len, frame.size() - 5);

	std::string disc_id;

	EXPECT_TRUE(Protocol::Request::parse_disconnect(frame.data() + 5, payload_len, disc_id));
	EXPECT_EQ(disc_id, client_id);
}



// ============================================================================
// Suite 3: Server Response & Delivery Parsing
// ============================================================================


TEST(ProtocolResponseTest, ResponseRoundTrip) {
	std::string json_data = "{\"client\":\"client0\",\"offset\":4}";
	auto resp = Protocol::Response::serialize_response(Protocol::StatusCode::SUCCESS, json_data);

	Protocol::StatusCode status;
	std::string payload;
	EXPECT_TRUE(Protocol::Response::parse_response(resp.data(), resp.size(), status, payload));
	EXPECT_EQ(status, Protocol::StatusCode::SUCCESS);
	EXPECT_EQ(payload, json_data);
}

TEST(ProtocolResponseTest, ResponseErrorWithoutPayload) {
	auto resp = Protocol::Response::serialize_response(Protocol::StatusCode::TOPIC_CLIENT_ERR, "");

	Protocol::StatusCode status;
	std::string payload;
	EXPECT_TRUE(Protocol::Response::parse_response(resp.data(), resp.size(), status, payload));
	EXPECT_EQ(status, Protocol::StatusCode::TOPIC_CLIENT_ERR);
	EXPECT_TRUE(payload.empty());
}

TEST(ProtocolResponseTest, DeliveryRoundTrip) {
	uint32_t offset = 12;
	std::string key = "auth.login";
	std::string val = "{\"user\":\"reach\"}";

	auto frame = Protocol::Response::serialize_delivery(offset, key, val);

	uint32_t out_offset = 0;
	std::string out_key, out_val;
	EXPECT_TRUE(Protocol::Response::parse_delivery(frame.data(), frame.size(), out_offset, out_key, out_val));
	EXPECT_EQ(out_offset, offset);
	EXPECT_EQ(out_key, key);
	EXPECT_EQ(out_val, val);
	EXPECT_FALSE(Protocol::Response::is_shutdown(out_offset));
}

TEST(ProtocolResponseTest, ShutdownRoundTrip) {
	auto sentinel = Protocol::Response::serialize_shutdown();

	uint32_t offset = 0;
	std::string key, val;
	EXPECT_TRUE(Protocol::Response::parse_delivery(sentinel.data(), sentinel.size(), offset, key, val));
	EXPECT_TRUE(Protocol::Response::is_shutdown(offset));
}
