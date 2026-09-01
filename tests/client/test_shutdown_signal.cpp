#include "gtest/gtest.h"
#include "shutdown_signal.hpp"

#include <csignal>

// SIGINT debe avisar al bucle principal sin terminar el proceso de forma abrupta.
TEST(ClientShutdownSignal, RecordsSigintRequest) {
    client::reset_shutdown_request_for_tests();
    ASSERT_TRUE(client::install_shutdown_handlers());
    ASSERT_EQ(raise(SIGINT), 0);
    EXPECT_TRUE(client::shutdown_requested());
}

// SIGTERM comparte el mismo mecanismo de cierre ordenado exigido por el subject.
TEST(ClientShutdownSignal, RecordsSigtermRequest) {
    client::reset_shutdown_request_for_tests();
    ASSERT_TRUE(client::install_shutdown_handlers());
    ASSERT_EQ(raise(SIGTERM), 0);
    EXPECT_TRUE(client::shutdown_requested());
}
