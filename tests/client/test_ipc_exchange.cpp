#include "gtest/gtest.h"
#include "ipc_exchange.hpp"
#include "protocol_frame.hpp"
#include "protocol_io.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

// Crea un directorio temporal aislado para que el FIFO principal simulado no choque con otros tests.
std::string create_temporary_directory() {
    char directory_template[] = "/tmp/treenity-client-test-XXXXXX";
    char* directory = mkdtemp(directory_template);
    if (directory == 0)
        return "";
    return directory;
}

// El servidor simulado confirma que recibe el frame y responde usando el FIFO derivado del PID del cliente.
void serve_one_request(const std::string& server_fifo, std::uint32_t client_pid) {
    const int server_descriptor = open(server_fifo.c_str(), O_RDONLY);
    if (server_descriptor == -1)
        return;

    unsigned char request_header[client::kRequestHeaderSize] = {};
    const client::FifoReadResult request_read = client::read_exact(server_descriptor,
        request_header, client::kRequestHeaderSize);
    if (request_read.status != client::FifoReadStatus::Complete) {
        close(server_descriptor);
        return;
    }

    client::RequestHeader decoded_header = {};
    if (!client::decode_request_header(request_header, client::kRequestHeaderSize, decoded_header)) {
        close(server_descriptor);
        return;
    }
    const std::size_t payload_size = decoded_header.total_length;
    std::vector<unsigned char> request_payload(payload_size);
    if (payload_size > 0U) {
        const client::FifoReadResult payload_read = client::read_exact(server_descriptor,
            request_payload.data(), payload_size);
        if (payload_read.status != client::FifoReadStatus::Complete) {
            close(server_descriptor);
            return;
        }
    }
    close(server_descriptor);

    const std::string response_fifo = client::client_fifo_path(client_pid);
    const int response_descriptor = open(response_fifo.c_str(), O_WRONLY);
    if (response_descriptor == -1)
        return;
    const client::ResponseHeader response_header = {0U, 2U};
    const std::vector<unsigned char> response_bytes = client::encode_response_header(response_header);
    const unsigned char payload[] = {'o', 'k'};
    int error_number = 0;
    client::write_all(response_descriptor, response_bytes.data(), response_bytes.size(), error_number);
    client::write_all(response_descriptor, payload, sizeof(payload), error_number);
    close(response_descriptor);
}

// Una petición completa crea su FIFO privado, lo usa para la respuesta y lo elimina al terminar.
TEST(ClientIpcExchange, ExchangesRequestAndCleansPrivateFifo) {
    const std::string directory = create_temporary_directory();
    ASSERT_FALSE(directory.empty());
    const std::string server_fifo = directory + "/server";
    ASSERT_EQ(mkfifo(server_fifo.c_str(), S_IRUSR | S_IWUSR), 0);
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    const std::vector<unsigned char> request(client::kRequestHeaderSize, 0U);
    std::vector<unsigned char> valid_request;
    ASSERT_TRUE(client::build_request_frame(2U, client_pid, request, valid_request));

    std::thread server(serve_one_request, server_fifo, client_pid);
    const client::IpcExchangeResult result = client::exchange_request(server_fifo, client_pid, valid_request);
    server.join();

    EXPECT_EQ(result.status, client::IpcExchangeStatus::Complete);
    EXPECT_EQ(static_cast<int>(result.response.code), 0);
    ASSERT_EQ(result.response.payload.size(), 2U);
    EXPECT_EQ(result.response.payload[0], 'o');
    EXPECT_EQ(result.response.payload[1], 'k');
    struct stat information = {};
    EXPECT_EQ(stat(client::client_fifo_path(client_pid).c_str(), &information), -1);
    EXPECT_EQ(errno, ENOENT);
    unlink(server_fifo.c_str());
    rmdir(directory.c_str());
}

// Un FIFO principal inexistente se identifica sin quedarse bloqueado y permite el error general requerido.
TEST(ClientIpcExchange, DetectsUnavailableServer) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    const std::vector<unsigned char> request(client::kRequestHeaderSize, 0U);
    std::vector<unsigned char> valid_request;
    ASSERT_TRUE(client::build_request_frame(2U, client_pid, request, valid_request));

    const client::IpcExchangeResult result = client::exchange_request(
        "/tmp/treenity-no-server-for-client-tests", client_pid, valid_request);
    EXPECT_EQ(result.status, client::IpcExchangeStatus::ServerUnavailable);
    EXPECT_TRUE(result.error_number == ENOENT || result.error_number == ENXIO);
}

} // namespace
