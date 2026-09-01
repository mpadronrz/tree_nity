#include "producer_command.hpp"

#include "exit_status.hpp"
#include "ipc_exchange.hpp"
#include "message_parser.hpp"
#include "request_builder.hpp"

#include <unistd.h>

namespace client {
namespace {

// Conserva el texto enviado por el servidor al informar del topic inexistente u otro fallo conocido.
void write_server_error(std::ostream& error_output, const std::vector<unsigned char>& payload) {
    if (payload.empty()) {
        error_output << "server returned an error" << '\n';
        return;
    }
    error_output.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    error_output.put('\n');
}

// Ejecuta un request de productor y traduce las capas de transporte y protocolo a códigos del subject.
int send_producer_request(const Command& command, const std::vector<unsigned char>& request,
    std::uint32_t client_pid, std::ostream& error_output) {
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
    if (exit_code != 0)
        write_server_error(error_output, exchange.response.payload);
    return exit_code;
}

} // namespace

int execute_producer_command(const Command& command, std::istream& input, std::ostream& error_output) {
    const MessageParseResult parsed = command.raw ? parse_raw_messages(input) : parse_text_messages(input);
    if (!parsed.ok) {
        error_output << parsed.error << '\n';
        return 1;
    }

    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    std::vector<unsigned char> request;
    if (!build_single_string_request(RequestAction::ProducerConnect, client_pid, command.topic, request)) {
        error_output << "failed to build producer connection request" << '\n';
        return 1;
    }
    int exit_code = send_producer_request(command, request, client_pid, error_output);
    if (exit_code != 0)
        return exit_code;

    for (std::vector<Message>::size_type index = 0; index < parsed.messages.size(); ++index) {
        if (!build_two_string_request(RequestAction::Publish, client_pid, parsed.messages[index].key,
                parsed.messages[index].value, request)) {
            error_output << "failed to build publish request" << '\n';
            return 1;
        }
        exit_code = send_producer_request(command, request, client_pid, error_output);
        if (exit_code != 0)
            return exit_code;
    }
    return 0;
}

} // namespace client
