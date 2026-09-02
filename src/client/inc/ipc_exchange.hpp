#ifndef CLIENT_IPC_EXCHANGE_HPP
#define CLIENT_IPC_EXCHANGE_HPP

#include "protocol/Protocol.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace client {

struct ResponseFrame {
    Protocol::StatusCode code = Protocol::StatusCode::SUCCESS;
    std::vector<unsigned char> payload;
};

enum class IpcExchangeStatus {
    Complete,
    ServerUnavailable,
    CommunicationError,
    InvalidResponse
};

struct IpcExchangeResult {
    IpcExchangeStatus status = IpcExchangeStatus::Complete;
    ResponseFrame response = {};
};

std::string client_fifo_path(std::uint32_t client_pid);

IpcExchangeResult old_exchange_request(const std::string& server_fifo, std::uint32_t client_pid,
    const std::vector<unsigned char>& request);

IpcExchangeResult exchange_request(const std::string& server_fifo, std::uint32_t client_pid,
    const std::vector<unsigned char>& request);

}

#endif
