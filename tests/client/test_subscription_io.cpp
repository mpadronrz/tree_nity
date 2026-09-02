#include "gtest/gtest.h"
#include "protocol/Protocol.hpp"
#include "subscription_io.hpp"

#include <cstdint>
#include <unistd.h>
#include <vector>

namespace {

// Serializa un entero para construir en el test el mismo formato little-endian del servidor.
void append_uint32(std::vector<unsigned char>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<unsigned char>(value & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 8) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 16) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 24) & 0xffU));
}

// Un record completo se separa en offset, key y value sin perder bytes binarios.
TEST(ClientSubscriptionIo, ReadsCompleteSubscriptionMessage) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    const std::vector<unsigned char> bytes = Protocol::Response::serialize_delivery(12U, "user.create", "body");
    ASSERT_EQ(write(descriptors[1], bytes.data(), bytes.size()), static_cast<ssize_t>(bytes.size()));
    close(descriptors[1]);

    client::SubscriptionMessage message = {};
    const client::SubscriptionReadResult result = client::read_subscription_message(descriptors[0], message);
    EXPECT_EQ(result.status, client::SubscriptionReadStatus::Message);
    EXPECT_EQ(message.offset, 12U);
    EXPECT_EQ(message.message.key, "user.create");
    EXPECT_EQ(message.message.value, "body");
    close(descriptors[0]);
}

// EOF antes de un nuevo offset indica cierre limpio del canal dedicado.
TEST(ClientSubscriptionIo, DetectsCleanEndOfFile) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    close(descriptors[1]);
    client::SubscriptionMessage message = {};

    const client::SubscriptionReadResult result = client::read_subscription_message(descriptors[0], message);
    EXPECT_EQ(result.status, client::SubscriptionReadStatus::EndOfFile);
    close(descriptors[0]);
}

// Una key declarada mayor que 1024 bytes se rechaza antes de reservar memoria para el resto.
TEST(ClientSubscriptionIo, RejectsOversizedKey) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    std::vector<unsigned char> bytes;
    append_uint32(bytes, 0U);
    append_uint32(bytes, 1025U);
    append_uint32(bytes, 0U);
    ASSERT_EQ(write(descriptors[1], bytes.data(), bytes.size()), static_cast<ssize_t>(bytes.size()));
    close(descriptors[1]);
    client::SubscriptionMessage message = {};

    const client::SubscriptionReadResult result = client::read_subscription_message(descriptors[0], message);
    EXPECT_EQ(result.status, client::SubscriptionReadStatus::InvalidMessage);
    close(descriptors[0]);
}

} // namespace
