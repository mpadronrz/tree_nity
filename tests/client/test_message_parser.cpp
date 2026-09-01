#include "gtest/gtest.h"
#include "message_parser.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace {

// Escribe un entero de 32 bits en el orden little-endian que exige el formato --raw.
void append_little_endian_int32(std::string& data, std::uint32_t number) {
    data.push_back(static_cast<char>(number & 0xffU));
    data.push_back(static_cast<char>((number >> 8) & 0xffU));
    data.push_back(static_cast<char>((number >> 16) & 0xffU));
    data.push_back(static_cast<char>((number >> 24) & 0xffU));
}

// Añade un registro raw completo para que los tests describan claramente su entrada binaria.
void append_raw_message(std::string& data, const std::string& key, const std::string& value) {
    append_little_endian_int32(data, static_cast<std::uint32_t>(key.size()));
    data.append(key);
    append_little_endian_int32(data, static_cast<std::uint32_t>(value.size()));
    data.append(value);
}

// El separador es solo el primer ':', por lo que el body puede contener más dos puntos.
TEST(ClientMessageParser, ParsesTextMessages) {
    std::istringstream input("user.create:{\"name\":\"ada\"}\norder:paid\n");
    const client::MessageParseResult result = client::parse_text_messages(input);

    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.messages.size(), 2U);
    EXPECT_EQ(result.messages[0].key, "user.create");
    EXPECT_EQ(result.messages[0].value, "{\"name\":\"ada\"}");
    EXPECT_EQ(result.messages[1].key, "order");
    EXPECT_EQ(result.messages[1].value, "paid");
}

// Un registro textual sin separador no cumple la gramática key:body.
TEST(ClientMessageParser, RejectsTextMessageWithoutSeparator) {
    std::istringstream input("missing-separator\n");
    const client::MessageParseResult result = client::parse_text_messages(input);

    EXPECT_FALSE(result.ok);
}

// El límite de 1024 se aplica a la suma de key y body, sin incluir metadatos.
TEST(ClientMessageParser, RejectsOversizedTextMessage) {
    const std::string body(1025, 'x');
    std::istringstream input("key:" + body + "\n");
    const client::MessageParseResult result = client::parse_text_messages(input);

    EXPECT_FALSE(result.ok);
}

// Varios registros raw se leen consecutivamente sin saltos ni padding entre ellos.
TEST(ClientMessageParser, ParsesBackToBackRawMessages) {
    std::string bytes;
    append_raw_message(bytes, "user.create", "first");
    append_raw_message(bytes, "order", "second");
    std::istringstream input(bytes);
    const client::MessageParseResult result = client::parse_raw_messages(input);

    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.messages.size(), 2U);
    EXPECT_EQ(result.messages[0].key, "user.create");
    EXPECT_EQ(result.messages[0].value, "first");
    EXPECT_EQ(result.messages[1].key, "order");
    EXPECT_EQ(result.messages[1].value, "second");
}

// EOF dentro de un registro raw es un error explícito del productor según el enunciado.
TEST(ClientMessageParser, RejectsPartialRawRecordAtEndOfFile) {
    std::string bytes;
    append_little_endian_int32(bytes, 4);
    bytes.append("ab");
    std::istringstream input(bytes);
    const client::MessageParseResult result = client::parse_raw_messages(input);

    EXPECT_FALSE(result.ok);
}

// Los tamaños codificados como int32 negativo deben rechazarse antes de leer o reservar memoria.
TEST(ClientMessageParser, RejectsNegativeRawSize) {
    std::string bytes;
    append_little_endian_int32(bytes, 0xFFFFFFFFU);
    std::istringstream input(bytes);
    const client::MessageParseResult result = client::parse_raw_messages(input);

    EXPECT_FALSE(result.ok);
}

} // namespace
