#include "ipc_exchange.hpp"

#include "ipc/ipc.hpp"
#include "protocol/Protocol.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace client {

std::string client_fifo_path(std::uint32_t client_pid) {
    std::ostringstream path;
    path << "/tmp/treenity.client." << client_pid;
    return path.str();
}

IpcExchangeResult exchange_request(const std::string& server_fifo, std::uint32_t client_pid,
    const std::vector<unsigned char>& request) {
    const std::string client_pipe = client_fifo_path(client_pid);
    ipc::FifoReader reader = ipc::FifoReader::create(client_pipe, true);
    if (!reader.is_open()) {
        return IpcExchangeResult{IpcExchangeStatus::CommunicationError};
    }

    ipc::FifoWriter writer = ipc::FifoWriter::open(server_fifo, std::chrono::milliseconds(2000));
    if (!writer.is_open()) {
        return IpcExchangeResult{IpcExchangeStatus::ServerUnavailable};
    }

    bool written = writer.write_exact(request.data(), request.size());
    if (!written) {
        return IpcExchangeResult{IpcExchangeStatus::CommunicationError};
    }
    writer.close();
    uint8_t header_buf[5] = {};
    if (!reader.read_exact(header_buf, 5, 2000))
        return IpcExchangeResult{IpcExchangeStatus::CommunicationError};
    Protocol::ManagementResponseHeader header = {};
    if (!Protocol::parse_response_header(header_buf, header))
        return IpcExchangeResult{IpcExchangeStatus::InvalidResponse};
    ResponseFrame response = {};
    response.code = header.status_code;
    response.payload.resize(header.payload_len);
    if (header.payload_len > 0) {
        if (!reader.read_exact(response.payload.data(), header.payload_len, 2000))
            return IpcExchangeResult{IpcExchangeStatus::CommunicationError};
    }
    IpcExchangeResult result{IpcExchangeStatus::Complete, std::move(response)};
    return result;
}

}
