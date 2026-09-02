#include "gtest/gtest.h"
#include "subscriber_command.hpp"

#include <sstream>

namespace {

// Un identificador de servidor inválido se detecta durante el registro y devuelve el código general.
TEST(ClientSubscriberCommand, DetectsUnavailableServerDuringRegistration) {
    client::Command command = {};
    command.type = Protocol::CommandType::SUBSCRIBE;
    command.ipc_identifier = "/tmp/treenity-no-subscriber-server";
    command.topic = "events";
    command.subscriber = "client0";
    command.prefix = "";
    command.has_offset = false;
    command.raw = false;
    std::ostringstream output;
    std::ostringstream error_output;

    EXPECT_EQ(client::execute_subscriber_command(command), 1);
}

} // namespace
