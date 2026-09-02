#include "producer_command.hpp"

#include "exit_status.hpp"
#include "ipc/ipc.hpp"
#include "ipc_exchange.hpp"
#include "message_output.hpp"
#include "protocol/Protocol.hpp"

#include <iostream>
#include <limits>
#include <unistd.h>

namespace client {
namespace {

enum class IntegerReadStatus {
    Ok,
    EndOfFile,
    Partial
};

IntegerReadStatus read_little_endian_int32(std::uint32_t& number) {
    char bytes[4] = {};
    std::size_t total_read = 0;

    while (total_read < 4) {
        std::cin.read(bytes + total_read, static_cast<std::streamsize>(4 - total_read));
        const std::streamsize bytes_read = std::cin.gcount();
        if (bytes_read <= 0) {
            if (total_read == 0 && std::cin.eof())
                return IntegerReadStatus::EndOfFile;
            return IntegerReadStatus::Partial;
        }
        total_read += static_cast<std::size_t>(bytes_read);
    }

    number = static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[0]));
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[1])) << 8;
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[2])) << 16;
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[3])) << 24;
    return IntegerReadStatus::Ok;
}

bool read_exact(std::size_t size, std::string& value) {
    value.clear();
    if (size == 0)
        return true;

    value.resize(size);
    std::size_t total_read = 0;

    while (total_read < size) {
        std::cin.read(&value[total_read], static_cast<std::streamsize>(size - total_read));
        const std::streamsize bytes_read = std::cin.gcount();
        if (bytes_read <= 0)
            return false;
        total_read += static_cast<std::size_t>(bytes_read);
    }
    return true;
}

int execute_publish_text(const Command& command, ipc::FifoWriter& writer) {
    std::string line;
    while (std::getline(std::cin, line)) {
        const size_t separator = line.find(':');
        if (separator == std::string::npos) {
            std::cerr << "text message must contain ':' between key and body" << '\n';
            return 1;
        }

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (key.size() > Protocol::MAX_PAYLOAD_SIZE || value.size() > Protocol::MAX_PAYLOAD_SIZE - key.size()) {
            std::cerr << "message key and body exceed 1024 bytes" << '\n';
            return 1;
        }

        std::vector<uint8_t> frame = Protocol::Request::serialize_produce(command.topic, key, value);
        if (!writer.write_exact(frame.data(), frame.size())) {
            std::cerr << "IPC communication error" << '\n';
            return 3;
        }
    }

    if (!std::cin.eof()) {
        std::cerr << "failed to read text input" << '\n';
        return 1;
    }
    return 0;
}

int execute_publish_raw(const Command& command, ipc::FifoWriter& writer) {
    while (true) {
        std::uint32_t encoded_key_size = 0;
        const IntegerReadStatus key_size_status = read_little_endian_int32(encoded_key_size);
        if (key_size_status == IntegerReadStatus::EndOfFile)
            return 0;
        if (key_size_status == IntegerReadStatus::Partial) {
            std::cerr << "partial raw record at EOF" << '\n';
            return 1;
        }

        const std::size_t key_size = static_cast<std::size_t>(encoded_key_size);
        if (key_size > Protocol::MAX_PAYLOAD_SIZE) {
            std::cerr << "message key and body exceed 1024 bytes" << '\n';
            return 1;
        }

        std::string key;
        if (!read_exact(key_size, key)) {
            std::cerr << "partial raw record at EOF" << '\n';
            return 1;
        }

        std::uint32_t encoded_val_size = 0;
        const IntegerReadStatus val_size_status = read_little_endian_int32(encoded_val_size);
        if (val_size_status != IntegerReadStatus::Ok) {
            std::cerr << "partial raw record at EOF" << '\n';
            return 1;
        }

        const std::size_t val_size = static_cast<std::size_t>(encoded_val_size);
        if (val_size > Protocol::MAX_PAYLOAD_SIZE - key_size) {
            std::cerr << "message key and body exceed 1024 bytes" << '\n';
            return 1;
        }

        std::string value;
        if (!read_exact(val_size, value)) {
            std::cerr << "partial raw record at EOF" << '\n';
            return 1;
        }

        std::vector<uint8_t> frame = Protocol::Request::serialize_produce(command.topic, key, value);
        if (!writer.write_exact(frame.data(), frame.size())) {
            std::cerr << "IPC communication error" << '\n';
            return 3;
        }
    }
}

int execute_connect_command(const Command& command) {
    const std::uint32_t client_pid = static_cast<std::uint32_t>(getpid());
    std::vector<unsigned char> request = Protocol::Request::serialize_connect(client_pid, command.topic);

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

    const int exit_code = exit_code_from_response_code(static_cast<std::uint8_t>(exchange.response.code));
    if (exit_code != 0) {
        write_error(exchange.response.payload);
        return exit_code;
    }
    return 0;
}

} // namespace

int execute_producer_command(const Command& command) {
    const int connect_code = execute_connect_command(command);
    if (connect_code != 0) {
        return connect_code;
    }

    ipc::FifoWriter writer = ipc::FifoWriter::open(command.ipc_identifier, std::chrono::milliseconds(2000));
    if (!writer.is_open()) {
        std::cerr << "failed to connect to server FIFO" << '\n';
        return 1;
    }

    if (command.raw)
        return execute_publish_raw(command, writer);
    return execute_publish_text(command, writer);
}

} // namespace client

