#include "gtest/gtest.h"
#include "exit_status.hpp"

// Cada error aislado debe producir el código público definido por el enunciado.
TEST(ClientExitStatus, ConvertsIndividualErrors) {
    EXPECT_EQ(client::exit_code_for(client::ClientError::None), 0);
    EXPECT_EQ(client::exit_code_for(client::ClientError::General), 1);
    EXPECT_EQ(client::exit_code_for(client::ClientError::Topic), 2);
    EXPECT_EQ(client::exit_code_for(client::ClientError::Ipc), 3);
}

// La precedencia obligatoria es General (1), después IPC (3), y finalmente Topic (2).
TEST(ClientExitStatus, AppliesRequiredErrorPrecedence) {
    EXPECT_EQ(client::exit_code_for(client::ClientError::Topic, client::ClientError::Ipc), 3);
    EXPECT_EQ(client::exit_code_for(client::ClientError::Ipc, client::ClientError::General), 1);
    EXPECT_EQ(client::exit_code_for(client::ClientError::Topic, client::ClientError::General), 1);
}
