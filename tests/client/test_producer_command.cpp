#include "gtest/gtest.h"
#include "producer_command.hpp"

#include <sstream>

namespace {

// Un formato de productor inválido se detecta antes de abrir ningún FIFO del servidor.
TEST(ClientProducerCommand, RejectsInvalidTextInputBeforeIpc) {
    client::Command command = {};
    command.type = client::CommandType::Produce;
    command.ipc_identifier = "/tmp/server-not-needed";
    command.topic = "orders";
    command.raw = false;
    std::istringstream input("missing-separator\n");
    std::ostringstream error_output;

    EXPECT_EQ(client::execute_producer_command(command, input, error_output), 1);
    EXPECT_NE(error_output.str().find("must contain ':'"), std::string::npos);
}

} // namespace
