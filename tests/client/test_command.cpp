#include "gtest/gtest.h"
#include "command.hpp"

#include <initializer_list>
#include <vector>

namespace {

// Convierte una lista legible de argumentos en el formato que recibe parse_command.
client::ParseResult parse(std::initializer_list<const char *> arguments) {
    std::vector<char *> argv;
    for (std::initializer_list<const char *>::const_iterator it = arguments.begin(); it != arguments.end(); ++it)
        argv.push_back(const_cast<char *>(*it));
    return client::parse_command(static_cast<int>(argv.size()), argv.data());
}

// Verifica el subcomando de gestión más sencillo y la conservación de su topic.
TEST(ClientCommandParser, ParsesCreate) {
    const client::ParseResult result = parse({"client", "/tmp/tree.42", "create", "user_events"});
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.command.type, Protocol::CommandType::CREATE);
    EXPECT_EQ(result.command.topic, "user_events");
}

// Rechaza espacios, que no pertenecen al formato de nombre exigido por el enunciado.
TEST(ClientCommandParser, RejectsInvalidTopicName) {
    const client::ParseResult result = parse({"client", "/tmp/tree.42", "create", "not valid"});
    EXPECT_FALSE(result.ok);
}

// Comprueba que el productor reconoce su única opción opcional.
TEST(ClientCommandParser, ParsesProduceRaw) {
    const client::ParseResult result = parse({"client", "/tmp/tree.42", "produce", "orders", "--raw"});
    ASSERT_TRUE(result.ok);
    EXPECT_TRUE(result.command.raw);
}

// Las opciones de subscribe son independientes, por lo que su orden no debe importar.
TEST(ClientCommandParser, ParsesSubscribeOptionsInAnyOrder) {
    const client::ParseResult result = parse({"client", "/tmp/tree.42", "subscribe", "orders", "client-1", "--raw", "--offset", "42", "--prefix", "user."});
    ASSERT_TRUE(result.ok);
    EXPECT_TRUE(result.command.raw);
    EXPECT_TRUE(result.command.has_offset);
    EXPECT_EQ(result.command.offset, 42U);
    EXPECT_EQ(result.command.prefix, "user.");
}

// Asegura que los casos ambiguos o incompletos no llegan a la futura capa FIFO.
TEST(ClientCommandParser, RejectsInvalidOrDuplicateSubscribeOptions) {
    EXPECT_FALSE(parse({"client", "/tmp/tree.42", "subscribe", "orders", "client-1", "--offset", "not-a-number"}).ok);
    EXPECT_FALSE(parse({"client", "/tmp/tree.42", "subscribe", "orders", "client-1", "--raw", "--raw"}).ok);
    EXPECT_FALSE(parse({"client", "/tmp/tree.42", "subscribe", "orders", "client-1", "--prefix"}).ok);
}

// Un comando no definido y una aridad incorrecta son errores generales de argumentos.
TEST(ClientCommandParser, RejectsUnknownCommandAndWrongArity) {
    EXPECT_FALSE(parse({"client", "/tmp/tree.42", "remove", "orders"}).ok);
    EXPECT_FALSE(parse({"client", "/tmp/tree.42", "list", "unexpected"}).ok);
}

} // namespace
