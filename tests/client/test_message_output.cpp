#include "gtest/gtest.h"
#include "message_output.hpp"

#include <cstdint>
#include <sstream>
#include <string>

namespace {

// Lee un int32 little-endian de una cadena para verificar los bytes exactos escritos por --raw.
std::uint32_t read_little_endian_int32(const std::string& bytes, std::string::size_type position) {
    std::uint32_t number = static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[position]));
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[position + 1])) << 8;
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[position + 2])) << 16;
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[position + 3])) << 24;
    return number;
}

// La salida de texto conserva key y body y termina cada mensaje con un salto de línea.
TEST(ClientMessageOutput, FormatsTextMessage) {
    client::Message message;
    message.key = "user.create";
    message.value = "{\"name\":\"ada\"}";

    EXPECT_EQ(client::format_text_message(message), "user.create:{\"name\":\"ada\"}\n");
}

// La salida raw del consumidor añade el offset antes de los campos que ya usaba el productor.
TEST(ClientMessageOutput, WritesRawMessageInLittleEndian) {
    client::Message message;
    message.key = "key";
    message.value = "body";
    std::ostringstream output;

    ASSERT_TRUE(client::write_raw_message(output, 42U, message));
    const std::string bytes = output.str();
    ASSERT_EQ(bytes.size(), 19U);
    EXPECT_EQ(read_little_endian_int32(bytes, 0), 42U);
    EXPECT_EQ(read_little_endian_int32(bytes, 4), 3U);
    EXPECT_EQ(bytes.substr(8, 3), "key");
    EXPECT_EQ(read_little_endian_int32(bytes, 11), 4U);
    EXPECT_EQ(bytes.substr(15, 4), "body");
}

} // namespace
