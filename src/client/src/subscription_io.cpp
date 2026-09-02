#include "subscription_io.hpp"

#include "fifo_io.hpp"
#include "protocol/Protocol.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace client {
namespace {

const std::size_t kMaximumMessageSize = 1024U;

// Convierte cuatro bytes little-endian recibidos por FIFO a un entero de protocolo.
std::uint32_t decode_uint32(const unsigned char bytes[4]) {
    std::uint32_t value = static_cast<std::uint32_t>(bytes[0]);
    value |= static_cast<std::uint32_t>(bytes[1]) << 8;
    value |= static_cast<std::uint32_t>(bytes[2]) << 16;
    value |= static_cast<std::uint32_t>(bytes[3]) << 24;
    return value;
}

// Traduce la lectura de un campo fijo para conservar la diferencia entre EOF limpio y mensaje truncado.
SubscriptionReadStatus status_from_fifo(FifoReadStatus status) {
    if (status == FifoReadStatus::EndOfFile)
        return SubscriptionReadStatus::EndOfFile;
    if (status == FifoReadStatus::PartialEndOfFile)
        return SubscriptionReadStatus::PartialEndOfFile;
    return SubscriptionReadStatus::Error;
}

// Lee una cadena cuyo tamaño ya fue leído y evita reservar si supera el límite del subject.
bool read_string(int file_descriptor, std::uint32_t encoded_size, std::string& value,
    int& error_number) {
    if (encoded_size > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
        return false;
    const std::size_t size = static_cast<std::size_t>(encoded_size);
    value.resize(size);
    if (size == 0U)
        return true;
    const FifoReadResult result = read_exact(file_descriptor, &value[0], size);
    if (result.status != FifoReadStatus::Complete) {
        error_number = result.error_number;
        return false;
    }
    return true;
}

} // namespace

SubscriptionReadResult read_subscription_message(int file_descriptor, SubscriptionMessage& message) {
    SubscriptionReadResult result = {};
    result.status = SubscriptionReadStatus::Message;
    result.error_number = 0;
    unsigned char header_bytes[12] = {};

    FifoReadResult header_read = read_exact(file_descriptor, header_bytes, sizeof(header_bytes));
    if (header_read.status != FifoReadStatus::Complete) {
        result.status = status_from_fifo(header_read.status);
        result.error_number = header_read.error_number;
        return result;
    }

    message.offset = decode_uint32(header_bytes);
    if (Protocol::Response::is_shutdown(message.offset)) {
        result.status = SubscriptionReadStatus::EndOfFile;
        return result;
    }

    const std::uint32_t key_size = decode_uint32(header_bytes + 4);
    const std::uint32_t value_size = decode_uint32(header_bytes + 8);

    if (key_size > kMaximumMessageSize || value_size > kMaximumMessageSize - key_size) {
        result.status = SubscriptionReadStatus::InvalidMessage;
        return result;
    }

    if (!read_string(file_descriptor, key_size, message.message.key, result.error_number)) {
        result.status = SubscriptionReadStatus::InvalidMessage;
        return result;
    }

    if (!read_string(file_descriptor, value_size, message.message.value, result.error_number)) {
        result.status = SubscriptionReadStatus::InvalidMessage;
        return result;
    }

    return result;
}

} // namespace client
