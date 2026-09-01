#include "gtest/gtest.h"
#include "fifo_io.hpp"

#include <cstring>
#include <unistd.h>

namespace {

// Cierra los dos extremos del pipe para que cada test libere sus descriptores incluso al fallar.
void close_pipe(int descriptors[2]) {
    close(descriptors[0]);
    close(descriptors[1]);
}

// Un pipe tiene las mismas garantías de lectura/escritura de descriptor que usará el FIFO.
TEST(ClientFifoIo, WritesAndReadsAllBytes) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    const char sent[] = "hello";
    char received[sizeof(sent)] = {};
    int error_number = 0;

    ASSERT_TRUE(client::write_all(descriptors[1], sent, sizeof(sent), error_number));
    const client::FifoReadResult result = client::read_exact(descriptors[0], received, sizeof(received));
    EXPECT_EQ(result.status, client::FifoReadStatus::Complete);
    EXPECT_EQ(std::memcmp(sent, received, sizeof(sent)), 0);
    close_pipe(descriptors);
}

// Cerrar el escritor antes de leer es el EOF limpio que encontrará un consumidor desconectado.
TEST(ClientFifoIo, DetectsEndOfFileBeforeMessage) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    close(descriptors[1]);
    char received[4] = {};

    const client::FifoReadResult result = client::read_exact(descriptors[0], received, sizeof(received));
    EXPECT_EQ(result.status, client::FifoReadStatus::EndOfFile);
    close(descriptors[0]);
}

// Cerrar el escritor después de entregar solo parte del mensaje nunca debe parecer una lectura correcta.
TEST(ClientFifoIo, DetectsPartialEndOfFile) {
    int descriptors[2] = {};
    ASSERT_EQ(pipe(descriptors), 0);
    const char partial[] = "ab";
    ASSERT_EQ(write(descriptors[1], partial, sizeof(partial)), static_cast<ssize_t>(sizeof(partial)));
    close(descriptors[1]);
    char received[4] = {};

    const client::FifoReadResult result = client::read_exact(descriptors[0], received, sizeof(received));
    EXPECT_EQ(result.status, client::FifoReadStatus::PartialEndOfFile);
    close(descriptors[0]);
}

} // namespace
