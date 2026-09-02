#include "gtest/gtest.h"
#include "protocol_frame.hpp"
#include "protocol_io.hpp"

#include <unistd.h>
#include <vector>

namespace {

// Cierra ambos extremos del pipe usado para simular una comunicación FIFO en los tests.
void close_pipe(int descriptors[2]) {
    close(descriptors[0]);
    close(descriptors[1]);
}

// La capa de protocolo recupera por separado el código y los bytes anunciados en el header.
TEST(ClientProtocolIo, ReadsCompleteResponseFrame) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    const client::ResponseHeader header = {0U, 3U};
    const std::vector<unsigned char> header_bytes = client::encode_response_header(header);
    const std::vector<unsigned char> payload = {'o', 'k', '!'};
    int error_number = 0;
    client::ResponseFrame response = {};

    ASSERT_TRUE(client::write_all(descriptors[1], header_bytes.data(), header_bytes.size(), error_number));
    ASSERT_TRUE(client::write_all(descriptors[1], payload.data(), payload.size(), error_number));
    close(descriptors[1]);
    const client::ResponseReadResult result = client::read_response_frame(descriptors[0], response);
    EXPECT_EQ(result.status, client::ResponseReadStatus::Complete);
    EXPECT_EQ(static_cast<int>(response.code), 0);
    EXPECT_EQ(response.payload, payload);
    close(descriptors[0]);
}

// Una respuesta sin payload ocupa solo el header y es válida para confirmaciones simples.
TEST(ClientProtocolIo, ReadsResponseWithoutPayload) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    const client::ResponseHeader header = {0U, 0U};
    const std::vector<unsigned char> header_bytes = client::encode_response_header(header);
    int error_number = 0;
    client::ResponseFrame response = {};

    ASSERT_TRUE(client::write_all(descriptors[1], header_bytes.data(), header_bytes.size(), error_number));
    close(descriptors[1]);
    const client::ResponseReadResult result = client::read_response_frame(descriptors[0], response);
    EXPECT_EQ(result.status, client::ResponseReadStatus::Complete);
    EXPECT_TRUE(response.payload.empty());
    close(descriptors[0]);
}

// Si el servidor anuncia bytes que no entrega, el cliente detecta un frame truncado.
TEST(ClientProtocolIo, DetectsPartialResponsePayload) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    const client::ResponseHeader header = {0U, 3U};
    const std::vector<unsigned char> header_bytes = client::encode_response_header(header);
    const unsigned char partial_payload[] = {'o', 'k'};
    int error_number = 0;
    client::ResponseFrame response = {};

    ASSERT_TRUE(client::write_all(descriptors[1], header_bytes.data(), header_bytes.size(), error_number));
    ASSERT_TRUE(client::write_all(descriptors[1], partial_payload, sizeof(partial_payload), error_number));
    close(descriptors[1]);
    const client::ResponseReadResult result = client::read_response_frame(descriptors[0], response);
    EXPECT_EQ(result.status, client::ResponseReadStatus::PartialEndOfFile);
    EXPECT_TRUE(response.payload.empty());
    close(descriptors[0]);
}

// El escritor no debe aceptar un request menor que su header porque nunca sería parseable por el servidor.
TEST(ClientProtocolIo, RejectsTooShortRequestFrame) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    const std::vector<unsigned char> invalid_frame = {1U, 2U};
    int error_number = 123;

    EXPECT_FALSE(client::write_request_frame(descriptors[1], invalid_frame, error_number));
    EXPECT_EQ(error_number, 0);
    close_pipe(descriptors);
}

} // namespace
