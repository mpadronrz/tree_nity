#include "gtest/gtest.h"
#include "protocol_frame.hpp"
#include "protocol_payload.hpp"
#include "request_builder.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

// List no necesita campos adicionales, pero conserva el header común de nueve bytes.
TEST(ClientRequestBuilder, BuildsListRequestWithoutPayload) {
    std::vector<unsigned char> frame;
    client::RequestHeader header = {};

    ASSERT_TRUE(client::build_empty_request(client::RequestAction::ListTopics, 99U, frame));
    ASSERT_EQ(frame.size(), client::kRequestHeaderSize);
    ASSERT_TRUE(client::decode_request_header(frame.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::ListTopics));
    EXPECT_EQ(header.client_pid, 99U);
    EXPECT_EQ(header.total_length, client::kRequestHeaderSize);
}

// Create conserva el topic dentro del payload de cadena con longitud prefijada acordado.
TEST(ClientRequestBuilder, BuildsCreateRequestWithTopicPayload) {
    std::vector<unsigned char> frame;
    std::vector<unsigned char> payload;
    client::RequestHeader header = {};
    std::string topic;

    ASSERT_TRUE(client::build_single_string_request(client::RequestAction::CreateTopic,
        42U, "user_events", frame));
    ASSERT_TRUE(client::decode_request_header(frame.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::CreateTopic));
    EXPECT_EQ(header.total_length, frame.size());
    payload.assign(frame.begin() + static_cast<std::vector<unsigned char>::difference_type>(client::kRequestHeaderSize), frame.end());
    ASSERT_TRUE(client::decode_single_string_payload(payload, topic));
    EXPECT_EQ(topic, "user_events");
}

// Info usa el mismo formato de payload que create, pero con su acción propia.
TEST(ClientRequestBuilder, BuildsInfoRequestWithSubscriberPayload) {
    std::vector<unsigned char> frame;
    std::vector<unsigned char> payload;
    client::RequestHeader header = {};
    std::string subscriber;

    ASSERT_TRUE(client::build_single_string_request(client::RequestAction::ClientInfo,
        7U, "client0", frame));
    ASSERT_TRUE(client::decode_request_header(frame.data(), client::kRequestHeaderSize, header));
    EXPECT_EQ(header.action, static_cast<std::uint8_t>(client::RequestAction::ClientInfo));
    payload.assign(frame.begin() + static_cast<std::vector<unsigned char>::difference_type>(client::kRequestHeaderSize), frame.end());
    ASSERT_TRUE(client::decode_single_string_payload(payload, subscriber));
    EXPECT_EQ(subscriber, "client0");
}

} // namespace
