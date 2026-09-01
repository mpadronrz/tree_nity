#include "gtest/gtest.h"
#include "protocol_frame.hpp"

#include <cstdint>
#include <vector>

namespace {

// El header de petición conserva acción, PID y longitud total en el orden acordado.
TEST(ClientProtocolFrame, EncodesAndDecodesRequestHeaderInLittleEndian) {
    client::RequestHeader sent = {};
    sent.action = 4U;
    sent.client_pid = 0x01020304U;
    sent.total_length = 0x0a0b0c0dU;

    const std::vector<unsigned char> bytes = client::encode_request_header(sent);
    ASSERT_EQ(bytes.size(), client::kRequestHeaderSize);
    EXPECT_EQ(bytes[0], 4U);
    EXPECT_EQ(bytes[1], 0x04U);
    EXPECT_EQ(bytes[4], 0x01U);
    EXPECT_EQ(bytes[5], 0x0dU);
    EXPECT_EQ(bytes[8], 0x0aU);

    client::RequestHeader received = {};
    ASSERT_TRUE(client::decode_request_header(bytes.data(), bytes.size(), received));
    EXPECT_EQ(received.action, sent.action);
    EXPECT_EQ(received.client_pid, sent.client_pid);
    EXPECT_EQ(received.total_length, sent.total_length);
}

// El header de respuesta usa su propio tamaño fijo y conserva el código de resultado.
TEST(ClientProtocolFrame, EncodesAndDecodesResponseHeaderInLittleEndian) {
    client::ResponseHeader sent = {};
    sent.code = 2U;
    sent.total_length = 0x01020304U;

    const std::vector<unsigned char> bytes = client::encode_response_header(sent);
    ASSERT_EQ(bytes.size(), client::kResponseHeaderSize);
    EXPECT_EQ(bytes[0], 2U);
    EXPECT_EQ(bytes[1], 0x04U);
    EXPECT_EQ(bytes[4], 0x01U);

    client::ResponseHeader received = {};
    ASSERT_TRUE(client::decode_response_header(bytes.data(), bytes.size(), received));
    EXPECT_EQ(received.code, sent.code);
    EXPECT_EQ(received.total_length, sent.total_length);
}

// Las longitudes inferiores al propio header son inválidas y no deben llegar al parser del payload.
TEST(ClientProtocolFrame, RejectsLengthsShorterThanTheirHeader) {
    const unsigned char request_bytes[] = {1U, 0U, 0U, 0U, 0U, 8U, 0U, 0U, 0U};
    const unsigned char response_bytes[] = {0U, 4U, 0U, 0U, 0U};
    client::RequestHeader request = {};
    client::ResponseHeader response = {};

    EXPECT_FALSE(client::decode_request_header(request_bytes, sizeof(request_bytes), request));
    EXPECT_FALSE(client::decode_response_header(response_bytes, sizeof(response_bytes), response));
}

// El frame completo permite enviarlo en una sola operación de escritura al FIFO principal.
TEST(ClientProtocolFrame, BuildsRequestFrameWithPayloadAfterHeader) {
    const std::vector<unsigned char> payload = {'t', 'o', 'p', 'i', 'c'};
    std::vector<unsigned char> frame;

    ASSERT_TRUE(client::build_request_frame(1U, 42U, payload, frame));
    ASSERT_EQ(frame.size(), client::kRequestHeaderSize + payload.size());
    EXPECT_EQ(frame[0], 1U);
    EXPECT_EQ(frame[1], 42U);
    EXPECT_EQ(frame[5], 14U);
    EXPECT_EQ(frame[8], 0U);
    EXPECT_EQ(frame[9], 't');
    EXPECT_EQ(frame[13], 'c');
}

} // namespace
