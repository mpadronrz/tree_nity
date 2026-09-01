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

// Publish necesita dos campos consecutivos: key y value, ambos con su propia longitud.
TEST(ClientProtocolPayload, EncodesAndDecodesTwoStrings) {
    std::vector<unsigned char> payload;
    std::string key;
    std::string value;

    ASSERT_TRUE(client::encode_two_string_payload("user.create", "{\"id\":42}", payload));
    ASSERT_EQ(payload[0], 11U);
    ASSERT_EQ(payload[15], 9U);
    ASSERT_TRUE(client::decode_two_string_payload(payload, key, value));
    EXPECT_EQ(key, "user.create");
    EXPECT_EQ(value, "{\"id\":42}");
}

// El segundo campo también debe estar completo: no se acepta un value que termine antes de tiempo.
TEST(ClientProtocolPayload, RejectsTruncatedSecondString) {
    const std::vector<unsigned char> payload = {1U, 0U, 0U, 0U, 'k', 3U, 0U, 0U, 0U, 'a'};
    std::string key;
    std::string value;

    EXPECT_FALSE(client::decode_two_string_payload(payload, key, value));
}

// SubscriberConnect conserva las tres cadenas y los campos binarios de offset al final del payload.
TEST(ClientProtocolPayload, EncodesSubscriberOptions) {
    std::vector<unsigned char> payload;

    ASSERT_TRUE(client::encode_subscriber_payload("events", "client0", "user", true, 42U, payload));
    ASSERT_EQ(payload.size(), 4U + 6U + 4U + 7U + 4U + 4U + 1U + 4U);
    EXPECT_EQ(payload[payload.size() - 5U], 1U);
    EXPECT_EQ(payload[payload.size() - 4U], 42U);
}

// Commit y desconexión tienen payloads independientes para no mezclar su estado con el de los mensajes.
TEST(ClientProtocolPayload, EncodesCommitAndDisconnect) {
    std::vector<unsigned char> commit;
    std::vector<unsigned char> disconnect;

    ASSERT_TRUE(client::encode_commit_payload("client0", 8U, commit));
    ASSERT_TRUE(client::encode_disconnect_payload("client0", disconnect));
    EXPECT_EQ(commit.size(), 4U + 7U + 4U);
    EXPECT_EQ(disconnect.size(), 4U + 7U);
}

} // namespace
