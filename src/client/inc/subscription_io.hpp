#ifndef CLIENT_SUBSCRIPTION_IO_HPP
#define CLIENT_SUBSCRIPTION_IO_HPP

#include "message_parser.hpp"

#include <cstdint>

namespace client {

// Un mensaje del canal dedicado llega como offset, key y value, todos delimitados por uint32.
struct SubscriptionMessage {
    std::uint32_t offset;
    Message message;
};

enum class SubscriptionReadStatus {
    Message,
    EndOfFile,
    PartialEndOfFile,
    Error,
    InvalidMessage
};

struct SubscriptionReadResult {
    SubscriptionReadStatus status;
    int error_number;
};

// Lee exactamente un mensaje del FIFO dedicado del consumidor.
SubscriptionReadResult read_subscription_message(int file_descriptor, SubscriptionMessage& message);

} // namespace client

#endif
