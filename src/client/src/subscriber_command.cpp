#include "subscriber_command.hpp"

#include "exit_status.hpp"
#include "fifo_io.hpp"
#include "ipc_exchange.hpp"
#include "message_output.hpp"
#include "request_builder.hpp"
#include "shutdown_signal.hpp"
#include "subscription_io.hpp"

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace client {
namespace {

// Escribe el texto de error del servidor sin depender de que el payload termine en salto de línea.
void write_server_error(std::ostream& error_output, const std::vector<unsigned char>& payload) {
    if (payload.empty())
        error_output << "server returned an error" << '\n';
    else {
        error_output.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
        error_output.put('\n');
    }
}

// Cambia un descriptor no bloqueante a bloqueante después del handshake inicial del consumidor.
bool set_blocking(int file_descriptor) {
    const int flags = fcntl(file_descriptor, F_GETFL, 0);
    if (flags == -1)
        return false;
    return fcntl(file_descriptor, F_SETFL, flags & ~O_NONBLOCK) != -1;
}

// Envía ACK o disconnect por el FIFO principal sin crear un segundo FIFO de respuesta.
bool send_control_frame(const std::string& server_fifo, const std::vector<unsigned char>& frame) {
    const int descriptor = open(server_fifo.c_str(), O_WRONLY | O_NONBLOCK);
    if (descriptor == -1)
        return false;
    int error_number = 0;
    const bool written = write_request_frame(descriptor, frame, error_number);
    close(descriptor);
    return written;
}

// Registra al consumidor y devuelve el descriptor que quedará escuchando mensajes y la ruta a limpiar.
int connect_subscriber(const Command& command, std::uint32_t client_pid,
    int& channel_descriptor, int& helper_writer, std::string& channel_path,
    std::ostream& error_output) {
    channel_path = client_fifo_path(client_pid);
    if (mkfifo(channel_path.c_str(), S_IRUSR | S_IWUSR) == -1)
        return 1;

    channel_descriptor = open(channel_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (channel_descriptor == -1) {
        const int error_number = errno;
        unlink(channel_path.c_str());
        (void)error_number;
        return 1;
    }
    helper_writer = open(channel_path.c_str(), O_WRONLY | O_NONBLOCK);
    if (helper_writer == -1) {
        close(channel_descriptor);
        unlink(channel_path.c_str());
        return 1;
    }

    std::vector<unsigned char> request;
    if (!build_subscriber_request(client_pid, command.topic, command.subscriber, command.prefix,
            command.has_offset, command.offset, request)) {
        close(helper_writer);
        close(channel_descriptor);
        unlink(channel_path.c_str());
        error_output << "failed to build subscriber request" << '\n';
        return 1;
    }

    const int server_descriptor = open(command.ipc_identifier.c_str(), O_WRONLY | O_NONBLOCK);
    if (server_descriptor == -1) {
        close(helper_writer);
        close(channel_descriptor);
        unlink(channel_path.c_str());
        error_output << "failed to connect to server FIFO" << '\n';
        return 1;
    }
    int write_error = 0;
    if (!write_request_frame(server_descriptor, request, write_error)) {
        close(server_descriptor);
        close(helper_writer);
        close(channel_descriptor);
        unlink(channel_path.c_str());
        error_output << "IPC communication error" << '\n';
        return 3;
    }
    close(server_descriptor);

    if (!set_blocking(channel_descriptor)) {
        close(helper_writer);
        close(channel_descriptor);
        unlink(channel_path.c_str());
        error_output << "IPC communication error" << '\n';
        return 3;
    }

    ResponseFrame response = {};
    const ResponseReadResult response_read = read_response_frame(channel_descriptor, response);
    if (response_read.status != ResponseReadStatus::Complete) {
        close(helper_writer);
        close(channel_descriptor);
        unlink(channel_path.c_str());
        error_output << "invalid response from server" << '\n';
        return 3;
    }
    const int response_code = exit_code_from_response_code(response.code);
    if (response_code != 0) {
        write_server_error(error_output, response.payload);
        close(helper_writer);
        close(channel_descriptor);
        unlink(channel_path.c_str());
        return response_code;
    }

    // El servidor ya ha abierto su extremo escritor; el helper deja de ser necesario para detectar EOF.
    close(helper_writer);
    helper_writer = -1;
    return 0;
}

} // namespace

int execute_subscriber_command(const Command& command, std::ostream& output, std::ostream& error_output) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    int channel_descriptor = -1;
    int helper_writer = -1;
    std::string channel_path;
    const int connection_code = connect_subscriber(command, client_pid, channel_descriptor,
        helper_writer, channel_path, error_output);
    if (connection_code != 0)
        return connection_code;

    int final_code = 0;
    while (true) {
        if (shutdown_requested()) {
            std::vector<unsigned char> disconnect;
            if (!build_disconnect_request(client_pid, command.subscriber, disconnect)
                || !send_control_frame(command.ipc_identifier, disconnect))
                final_code = 3;
            break;
        }

        struct pollfd descriptor = {};
        descriptor.fd = channel_descriptor;
        descriptor.events = POLLIN;
        const int poll_result = poll(&descriptor, 1, 100);
        if (poll_result == -1) {
            if (errno == EINTR)
                continue;
            final_code = 3;
            break;
        }
        if (poll_result == 0)
            continue;
        if ((descriptor.revents & (POLLERR | POLLNVAL)) != 0) {
            final_code = 3;
            break;
        }
        if ((descriptor.revents & (POLLIN | POLLHUP)) == 0)
            continue;

        SubscriptionMessage message = {};
        const SubscriptionReadResult read_result = read_subscription_message(channel_descriptor, message);
        if (read_result.status == SubscriptionReadStatus::EndOfFile)
            break;
        if (read_result.status != SubscriptionReadStatus::Message) {
            error_output << "invalid subscription message" << '\n';
            final_code = 3;
            break;
        }

        if (command.raw) {
            if (!write_raw_message(output, message.offset, message.message)) {
                error_output << "failed to write subscriber output" << '\n';
                final_code = 1;
                break;
            }
        } else
            output << format_text_message(message.message);
        output.flush();

        std::vector<unsigned char> commit;
        if (!build_commit_request(client_pid, command.subscriber, message.offset + 1U, commit)
            || !send_control_frame(command.ipc_identifier, commit)) {
            error_output << "IPC communication error" << '\n';
            final_code = 3;
            break;
        }
    }

    if (helper_writer >= 0)
        close(helper_writer);
    close(channel_descriptor);
    unlink(channel_path.c_str());
    return final_code;
}

} // namespace client
