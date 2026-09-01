#include "gtest/gtest.h"
#include "protocol_payload.hpp"

#include <string>
#include <vector>

namespace {

// Create e info usan este mismo campo: longitud seguida de los bytes del topic o del nombre.
TEST(ClientProtocolPayload, EncodesAndDecodesSingleStringInLittleEndian) {
    std::vector<unsigned char> payload;
    std::string decoded;

    ASSERT_TRUE(client::encode_single_string_payload("orders", payload));
    ASSERT_EQ(payload.size(), 10U);
    EXPECT_EQ(payload[0], 6U);
    EXPECT_EQ(payload[1], 0U);
    EXPECT_EQ(payload[2], 0U);
    EXPECT_EQ(payload[3], 0U);
    EXPECT_EQ(payload[4], 'o');
    EXPECT_EQ(payload[9], 's');
    ASSERT_TRUE(client::decode_single_string_payload(payload, decoded));
    EXPECT_EQ(decoded, "orders");
}

// Una cadena vacía sigue teniendo su longitud de cuatro bytes y es válida para futuros campos opcionales.
TEST(ClientProtocolPayload, SupportsEmptyString) {
    std::vector<unsigned char> payload;
    std::string decoded;

    ASSERT_TRUE(client::encode_single_string_payload("", payload));
    ASSERT_EQ(payload.size(), 4U);
    ASSERT_TRUE(client::decode_single_string_payload(payload, decoded));
    EXPECT_TRUE(decoded.empty());
}

// Un payload debe contener exactamente el número de bytes declarado: ni menos ni bytes adicionales.
TEST(ClientProtocolPayload, RejectsTruncatedOrTrailingStringBytes) {
    const std::vector<unsigned char> truncated = {3U, 0U, 0U, 0U, 'a', 'b'};
    const std::vector<unsigned char> trailing = {2U, 0U, 0U, 0U, 'a', 'b', 'c'};
    std::string decoded;

    EXPECT_FALSE(client::decode_single_string_payload(truncated, decoded));
    EXPECT_FALSE(client::decode_single_string_payload(trailing, decoded));
}

} // namespace
