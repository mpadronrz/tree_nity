#include "protocol_payload.hpp"

#include <cstdint>
#include <limits>

namespace client {
namespace {

// Añade la longitud de un campo variable sin depender de la representación interna de C++.
void append_little_endian_uint32(std::vector<unsigned char>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<unsigned char>(value & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 8) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 16) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 24) & 0xffU));
}

// Lee una longitud codificada en little-endian a partir de los cuatro primeros bytes del payload.
std::uint32_t read_little_endian_uint32(const std::vector<unsigned char>& bytes) {
    std::uint32_t value = static_cast<std::uint32_t>(bytes[0]);
    value |= static_cast<std::uint32_t>(bytes[1]) << 8;
    value |= static_cast<std::uint32_t>(bytes[2]) << 16;
    value |= static_cast<std::uint32_t>(bytes[3]) << 24;
    return value;
}

// Lee una longitud desde una posición conocida sin crear copias temporales del payload.
std::uint32_t read_little_endian_uint32_at(const std::vector<unsigned char>& bytes, std::size_t position) {
    std::uint32_t value = static_cast<std::uint32_t>(bytes[position]);
    value |= static_cast<std::uint32_t>(bytes[position + 1U]) << 8;
    value |= static_cast<std::uint32_t>(bytes[position + 2U]) << 16;
    value |= static_cast<std::uint32_t>(bytes[position + 3U]) << 24;
    return value;
}

// Añade una cadena con su longitud y permite componer payloads con varios campos variables.
void append_string(std::vector<unsigned char>& payload, const std::string& value) {
    append_little_endian_uint32(payload, static_cast<std::uint32_t>(value.size()));
    for (std::string::size_type index = 0; index < value.size(); ++index)
        payload.push_back(static_cast<unsigned char>(value[index]));
}

// Lee un campo de cadena desde una posición y avanza esa posición solo si todos sus bytes existen.
bool read_string(const std::vector<unsigned char>& payload, std::size_t& position, std::string& value) {
    if (payload.size() - position < 4U)
        return false;

    const std::uint32_t encoded_size = read_little_endian_uint32_at(payload, position);
    position += 4U;
    const std::size_t string_size = static_cast<std::size_t>(encoded_size);
    if (string_size > payload.size() - position)
        return false;

    if (string_size == 0U)
        value.clear();
    else
        value.assign(reinterpret_cast<const char*>(payload.data() + position), string_size);
    position += string_size;
    return true;
}

} // namespace

bool encode_single_string_payload(const std::string& value, std::vector<unsigned char>& payload) {
    if (value.size() > static_cast<std::string::size_type>(std::numeric_limits<std::uint32_t>::max()))
        return false;

    payload.clear();
    payload.reserve(4U + value.size());
    append_string(payload, value);
    return true;
}

bool encode_two_string_payload(const std::string& first, const std::string& second,
    std::vector<unsigned char>& payload) {
    const std::size_t maximum_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
    if (first.size() > maximum_size || second.size() > maximum_size)
        return false;
    if (second.size() > maximum_size - 8U)
        return false;
    if (first.size() > maximum_size - second.size() - 8U)
        return false;

    payload.clear();
    payload.reserve(8U + first.size() + second.size());
    append_string(payload, first);
    append_string(payload, second);
    return true;
}

bool decode_two_string_payload(const std::vector<unsigned char>& payload, std::string& first,
    std::string& second) {
    std::size_t position = 0;
    if (!read_string(payload, position, first))
        return false;
    if (!read_string(payload, position, second))
        return false;
    return position == payload.size();
}

bool encode_subscriber_payload(const std::string& topic, const std::string& subscriber,
    const std::string& prefix, bool has_offset, std::uint32_t offset,
    std::vector<unsigned char>& payload) {
    const std::size_t maximum_size = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
    if (topic.size() > maximum_size || subscriber.size() > maximum_size || prefix.size() > maximum_size)
        return false;
    if (prefix.size() > maximum_size - 13U)
        return false;
    if (subscriber.size() > maximum_size - prefix.size() - 13U)
        return false;
    if (topic.size() > maximum_size - subscriber.size() - prefix.size() - 13U)
        return false;

    payload.clear();
    payload.reserve(13U + topic.size() + subscriber.size() + prefix.size());
    append_string(payload, topic);
    append_string(payload, subscriber);
    append_string(payload, prefix);
    payload.push_back(has_offset ? 1U : 0U);
    append_little_endian_uint32(payload, offset);
    return true;
}

bool encode_commit_payload(const std::string& subscriber, std::uint32_t offset,
    std::vector<unsigned char>& payload) {
    if (subscriber.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        return false;
    payload.clear();
    payload.reserve(8U + subscriber.size());
    append_string(payload, subscriber);
    append_little_endian_uint32(payload, offset);
    return true;
}

bool encode_disconnect_payload(const std::string& subscriber, std::vector<unsigned char>& payload) {
    return encode_single_string_payload(subscriber, payload);
}

bool decode_single_string_payload(const std::vector<unsigned char>& payload, std::string& value) {
    if (payload.size() < 4U)
        return false;

    const std::uint32_t encoded_size = read_little_endian_uint32(payload);
    const std::size_t string_size = static_cast<std::size_t>(encoded_size);
    if (string_size != payload.size() - 4U)
        return false;

    if (string_size == 0U) {
        value.clear();
        return true;
    }
    value.assign(reinterpret_cast<const char*>(&payload[4]), string_size);
    return true;
}

} // namespace client
