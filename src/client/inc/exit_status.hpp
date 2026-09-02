#ifndef CLIENT_EXIT_STATUS_HPP
#define CLIENT_EXIT_STATUS_HPP

#include <cstdint>

namespace client {

enum class ClientError {
    None,
    Topic,
    Ipc,
    General
};

int exit_code_for(ClientError first, ClientError second = ClientError::None);

int exit_code_from_response_code(std::uint8_t response_code);

}

#endif
