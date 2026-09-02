#include "management_command.hpp"

#include "exit_status.hpp"
#include "ipc_exchange.hpp"
#include "message_output.hpp"

#include <unistd.h>
#include <iostream>

namespace client {
int execute_create_command(const Command& command) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    std::vector<unsigned char> request = Protocol::Request::serialize_create(client_pid, command.topic);

    const IpcExchangeResult exchange = exchange_request(command.ipc_identifier, client_pid, request);
    if (exchange.status == IpcExchangeStatus::ServerUnavailable) {
        std::cerr << "failed to connect to server FIFO" << '\n';
        return 1;
    }
    if (exchange.status == IpcExchangeStatus::CommunicationError) {
        std::cerr << "IPC communication error" << '\n';
        return 3;
    }
    if (exchange.status == IpcExchangeStatus::InvalidResponse) {
        std::cerr << "invalid response from server" << '\n';
        return 3;
    }

    const int exit_code = static_cast<int>(exchange.response.code);
    if (exit_code != 0) {
        write_error(exchange.response.payload);
        return exit_code;
    }

    std::cout << "topic created" << '\n';
    return 0;
}

int execute_list_command(const Command& command) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    std::vector<unsigned char> request = Protocol::Request::serialize_list(client_pid);

    const IpcExchangeResult exchange = exchange_request(command.ipc_identifier, client_pid, request);
    if (exchange.status == IpcExchangeStatus::ServerUnavailable) {
        std::cerr << "failed to connect to server FIFO" << '\n';
        return 1;
    }
    if (exchange.status == IpcExchangeStatus::CommunicationError) {
        std::cerr << "IPC communication error" << '\n';
        return 3;
    }
    if (exchange.status == IpcExchangeStatus::InvalidResponse) {
        std::cerr << "invalid response from server" << '\n';
        return 3;
    }

    const int exit_code = static_cast<int>(exchange.response.code);
    if (exit_code != 0) {
        write_error(exchange.response.payload);
        return exit_code;
    }
    write_output(exchange.response.payload);
    return 0;
}

int execute_info_command(const Command& command) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    std::vector<unsigned char> request = Protocol::Request::serialize_info(client_pid, command.subscriber);

    const IpcExchangeResult exchange = exchange_request(command.ipc_identifier, client_pid, request);
    if (exchange.status == IpcExchangeStatus::ServerUnavailable) {
        std::cerr << "failed to connect to server FIFO" << '\n';
        return 1;
    }
    if (exchange.status == IpcExchangeStatus::CommunicationError) {
        std::cerr << "IPC communication error" << '\n';
        return 3;
    }
    if (exchange.status == IpcExchangeStatus::InvalidResponse) {
        std::cerr << "invalid response from server" << '\n';
        return 3;
    }

    const int exit_code = static_cast<int>(exchange.response.code);
    if (exit_code != 0) {
        write_error(exchange.response.payload);
        return exit_code;
    }
    write_output(exchange.response.payload);
    return 0;
}

} 
