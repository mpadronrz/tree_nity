#include "management_command.hpp"

#include "exit_status.hpp"
#include "ipc_exchange.hpp"
#include "request_builder.hpp"

#include <unistd.h>

namespace client {
namespace {

// Escribe el payload del servidor como una línea de stdout para list e info.
void write_response_line(std::ostream& output, const std::vector<unsigned char>& payload) {
    if (!payload.empty())
        output.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    output.put('\n');
}

// Conserva el mensaje que envía el servidor y aporta uno mínimo cuando la respuesta no trae texto.
void write_server_error(std::ostream& error_output, const std::vector<unsigned char>& payload) {
    if (payload.empty()) {
        error_output << "server returned an error" << '\n';
        return;
    }
    error_output.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    error_output.put('\n');
}

// Construye el request específico de cada comando de gestión ya validado por parse_command.
bool build_management_request(const Command& command, std::uint32_t client_pid,
    std::vector<unsigned char>& request) {
    if (command.type == CommandType::Create)
        return build_single_string_request(RequestAction::CreateTopic, client_pid, command.topic, request);
    if (command.type == CommandType::List)
        return build_empty_request(RequestAction::ListTopics, client_pid, request);
    if (command.type == CommandType::Info)
        return build_single_string_request(RequestAction::ClientInfo, client_pid, command.subscriber, request);
    return false;
}

} // namespace

int execute_management_command(const Command& command, std::ostream& output, std::ostream& error_output) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    std::vector<unsigned char> request;
    if (!build_management_request(command, client_pid, request)) {
        error_output << "command is not a management operation" << '\n';
        return 1;
    }

    const IpcExchangeResult exchange = exchange_request(command.ipc_identifier, client_pid, request);
    if (exchange.status == IpcExchangeStatus::ServerUnavailable) {
        error_output << "failed to connect to server FIFO" << '\n';
        return 1;
    }
    if (exchange.status == IpcExchangeStatus::CommunicationError) {
        error_output << "IPC communication error" << '\n';
        return 3;
    }
    if (exchange.status == IpcExchangeStatus::InvalidResponse) {
        error_output << "invalid response from server" << '\n';
        return 3;
    }

    const int exit_code = exit_code_from_response_code(exchange.response.code);
    if (exit_code != 0) {
        write_server_error(error_output, exchange.response.payload);
        return exit_code;
    }

    if (command.type == CommandType::Create)
        output << "topic created" << '\n';
    else
        write_response_line(output, exchange.response.payload);
    return 0;
}

} // namespace client
