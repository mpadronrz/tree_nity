#include "gtest/gtest.h"
#include "protocol/Protocol.hpp"
#include "protocol_frame.hpp"
#include "protocol_payload.hpp"
#include "request_builder.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

// List no necesita campos adicionales, pero conserva el header común de 5 bytes.
TEST(ClientRequestBuilder, BuildsListRequestWithoutPayload) {
    std::vector<unsigned char> frame;
    client::RequestHeader header = {};

    ASSERT_TRUE(client::build_empty_request(client::RequestAction::ListTopics, 99U, frame));
    ASSERT_TRUE(client::decode_request_header(frame.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::ListTopics));

    std::uint32_t parsed_pid = 0;
    ASSERT_TRUE(Protocol::Request::parse_list(frame.data() + 5, frame.size() - 5, parsed_pid));
    EXPECT_EQ(parsed_pid, 99U);
}

// Create conserva el topic dentro del payload.
TEST(ClientRequestBuilder, BuildsCreateRequestWithTopicPayload) {
    std::vector<unsigned char> frame;
    client::RequestHeader header = {};

    ASSERT_TRUE(client::build_single_string_request(client::RequestAction::CreateTopic,
        42U, "user_events", frame));
    ASSERT_TRUE(client::decode_request_header(frame.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::CreateTopic));

    std::uint32_t parsed_pid = 0;
    std::string topic;
    ASSERT_TRUE(Protocol::Request::parse_create(frame.data() + 5, frame.size() - 5, parsed_pid, topic));
    EXPECT_EQ(parsed_pid, 42U);
    EXPECT_EQ(topic, "user_events");
}

// Info usa el mismo formato de payload que create, pero con su acción propia.
TEST(ClientRequestBuilder, BuildsInfoRequestWithSubscriberPayload) {
    std::vector<unsigned char> frame;
    client::RequestHeader header = {};

    ASSERT_TRUE(client::build_single_string_request(client::RequestAction::ClientInfo,
        7U, "client0", frame));
    ASSERT_TRUE(client::decode_request_header(frame.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::ClientInfo));

    std::uint32_t parsed_pid = 0;
    std::string subscriber;
    ASSERT_TRUE(Protocol::Request::parse_info(frame.data() + 5, frame.size() - 5, parsed_pid, subscriber));
    EXPECT_EQ(parsed_pid, 7U);
    EXPECT_EQ(subscriber, "client0");
}

// Publish transporta topic, key y value dentro de un mismo payload.
TEST(ClientRequestBuilder, BuildsPublishRequestWithKeyAndValue) {
    std::vector<unsigned char> frame;
    client::RequestHeader header = {};

    ASSERT_TRUE(client::build_publish_request("events", "user.create", "body", frame));
    ASSERT_TRUE(client::decode_request_header(frame.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::Publish));

    std::string topic, key, value;
    ASSERT_TRUE(Protocol::Request::parse_produce(frame.data() + 5, frame.size() - 5, topic, key, value));
    EXPECT_EQ(topic, "events");
    EXPECT_EQ(key, "user.create");
    EXPECT_EQ(value, "body");
}

// El request de suscripción incluye topic, identidad, prefijo y offset solicitado.
TEST(ClientRequestBuilder, BuildsSubscriberCommitAndDisconnectRequests) {
    std::vector<unsigned char> subscribe;
    std::vector<unsigned char> commit;
    std::vector<unsigned char> disconnect;
    client::RequestHeader header = {};

    ASSERT_TRUE(client::build_subscriber_request(7U, "events", "client0", "user", true, 3U, subscribe));
    ASSERT_TRUE(client::decode_request_header(subscribe.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::SubscriberConnect));

    std::uint32_t pid = 0, offset = 0;
    std::string sub_id, topic, prefix;
    ASSERT_TRUE(Protocol::Request::parse_subscribe(subscribe.data() + 5, subscribe.size() - 5, pid, sub_id, topic, prefix, offset));
    EXPECT_EQ(pid, 7U);
    EXPECT_EQ(sub_id, "client0");
    EXPECT_EQ(topic, "events");
    EXPECT_EQ(prefix, "user");
    EXPECT_EQ(offset, 3U);

    ASSERT_TRUE(client::build_commit_request(7U, "client0", 4U, commit));
    ASSERT_TRUE(client::build_disconnect_request(7U, "client0", disconnect));
    ASSERT_TRUE(client::decode_request_header(commit.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::SubscriberCommit));
    ASSERT_TRUE(client::decode_request_header(disconnect.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::Disconnect));
}

} // namespace
