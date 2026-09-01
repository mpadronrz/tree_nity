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

} // namespace

bool encode_single_string_payload(const std::string& value, std::vector<unsigned char>& payload) {
    if (value.size() > static_cast<std::string::size_type>(std::numeric_limits<std::uint32_t>::max()))
        return false;

    payload.clear();
    payload.reserve(4U + value.size());
    append_little_endian_uint32(payload, static_cast<std::uint32_t>(value.size()));
    for (std::string::size_type index = 0; index < value.size(); ++index)
        payload.push_back(static_cast<unsigned char>(value[index]));
    return true;
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
