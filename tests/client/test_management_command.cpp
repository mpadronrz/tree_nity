#include "gtest/gtest.h"
#include "management_command.hpp"

#include <cstdint>

namespace {

// Los códigos del protocolo preservan los códigos de salida que exige el subject.
TEST(ClientManagementCommand, ConvertsAllResponseCodesToExitCodes) {
    EXPECT_EQ(client::exit_code_for_response(0U), 0);
    EXPECT_EQ(client::exit_code_for_response(1U), 1);
    EXPECT_EQ(client::exit_code_for_response(2U), 2);
    EXPECT_EQ(client::exit_code_for_response(3U), 3);
}

// Un código desconocido no puede parecer éxito y se considera un fallo de comunicación de protocolo.
TEST(ClientManagementCommand, TreatsUnknownResponseCodeAsIpcError) {
    EXPECT_EQ(client::exit_code_for_response(4U), 3);
    EXPECT_EQ(client::exit_code_for_response(255U), 3);
}

} // namespace
