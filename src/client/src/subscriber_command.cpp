#include "subscriber_command.hpp"

#include "exit_status.hpp"
#include "ipc_exchange.hpp"
#include "message_output.hpp"
#include "shutdown_signal.hpp"
#include "ipc/ipc.hpp"

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <iostream>

namespace client {

int execute_subscriber_command(const Command& command) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    const std::string client_pipe = client_fifo_path(client_pid);

    ipc::FifoReader reader = ipc::FifoReader::create(client_pipe, true);
    if (!reader.is_open()) {
        std::cerr << "IPC communication error\n";
        return 3;
    }

    uint32_t requested_offset = command.has_offset ? command.offset : Protocol::OFFSET_UNSET;
    std::vector<unsigned char> request = Protocol::Request::serialize_subscribe(
        client_pid, command.subscriber, command.topic, command.prefix, requested_offset);

    ipc::FifoWriter writer = ipc::FifoWriter::open(command.ipc_identifier, std::chrono::milliseconds(2000));
    if (!writer.is_open()) {
        std::cerr << "failed to connect to server FIFO\n";
        return 1;
    }

    if (!writer.write_exact(request.data(), request.size())) {
        std::cerr << "IPC communication error\n";
        return 3;
    }

    uint8_t handshake_hdr[5] = {};
    if (!reader.read_exact(handshake_hdr, 5, 2000)) {
        std::cerr << "invalid response from server\n";
        return 3;
    }

    Protocol::ManagementResponseHeader response_hdr = {};
    if (!Protocol::parse_response_header(handshake_hdr, response_hdr)) {
        std::cerr << "invalid response from server\n";
        return 3;
    }

    if (response_hdr.payload_len > 0) {
        std::vector<unsigned char> err_payload(response_hdr.payload_len);
        (void)reader.read_exact(err_payload.data(), response_hdr.payload_len, 2000);
        if (response_hdr.status_code != Protocol::StatusCode::SUCCESS) {
            write_error(err_payload);
            return exit_code_from_response_code(static_cast<std::uint8_t>(response_hdr.status_code));
        }
    } else if (response_hdr.status_code != Protocol::StatusCode::SUCCESS) {
        return exit_code_from_response_code(static_cast<std::uint8_t>(response_hdr.status_code));
    }

    while (true) {
        if (shutdown_requested()) {
            std::vector<unsigned char> disc_frame = Protocol::Request::serialize_disconnect(command.subscriber);
            (void)writer.write_exact(disc_frame.data(), disc_frame.size());
            return 0;
        }

        struct pollfd pfd = {};
        pfd.fd = reader.fd();
        pfd.events = POLLIN;

        int poll_res = poll(&pfd, 1, 100);
        if (poll_res < 0) {
            if (errno == EINTR)
                continue;
            return 3;
        }
        if (poll_res == 0)
            continue;

        if ((pfd.revents & (POLLERR | POLLNVAL)) != 0)
            return 3;
        if ((pfd.revents & (POLLIN | POLLHUP)) == 0)
            continue;

        uint8_t delivery_hdr[12] = {};
        if (!reader.read_exact(delivery_hdr, 12, 1000)) {
            std::cerr << "invalid subscription message\n";
            return 3;
        }

        uint32_t offset = Protocol::read_uint32_le(&delivery_hdr[0]);
        uint32_t key_len = Protocol::read_uint32_le(&delivery_hdr[4]);
        uint32_t val_len = Protocol::read_uint32_le(&delivery_hdr[8]);

        if (Protocol::Response::is_shutdown(offset)) {
            return 0;
        }

        if (key_len + val_len > Protocol::MAX_PAYLOAD_SIZE) {
            std::cerr << "invalid subscription message\n";
            return 3;
        }

        Message message;
        message.key.resize(key_len);
        if (key_len > 0 && !reader.read_exact(&message.key[0], key_len, 1000)) {
            std::cerr << "invalid subscription message\n";
            return 3;
        }

        message.value.resize(val_len);
        if (val_len > 0 && !reader.read_exact(&message.value[0], val_len, 1000)) {
            std::cerr << "invalid subscription message\n";
            return 3;
        }

        if (command.raw) {
            if (!write_raw_message(std::cout, offset, message)) {
                std::cerr << "failed to write subscriber output\n";
                return 1;
            }
        } else {
            std::cout << format_text_message(message);
        }
        std::cout.flush();

        std::vector<unsigned char> ack_frame = Protocol::Request::serialize_ack(command.subscriber, offset + 1U);
        if (!writer.write_exact(ack_frame.data(), ack_frame.size())) {
            std::cerr << "IPC communication error\n";
            return 3;
        }
    }
}

} // namespace client
