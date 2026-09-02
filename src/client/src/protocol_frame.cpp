#include "protocol_frame.hpp"

#include <limits>

namespace client {
namespace {

// Añade un entero de 32 bits byte a byte para fijar el orden little-endian del protocolo.
void append_little_endian_uint32(std::vector<unsigned char>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<unsigned char>(value & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 8) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 16) & 0xffU));
    bytes.push_back(static_cast<unsigned char>((value >> 24) & 0xffU));
}

// Reconstruye un entero de 32 bits sin depender del orden de bytes de la máquina.
std::uint32_t read_little_endian_uint32(const unsigned char* bytes) {
    std::uint32_t value = static_cast<std::uint32_t>(bytes[0]);
    value |= static_cast<std::uint32_t>(bytes[1]) << 8;
    value |= static_cast<std::uint32_t>(bytes[2]) << 16;
    value |= static_cast<std::uint32_t>(bytes[3]) << 24;
    return value;
}

} // namespace

std::vector<unsigned char> encode_request_header(const RequestHeader& header) {
    std::vector<unsigned char> bytes;
    bytes.reserve(kRequestHeaderSize);
    bytes.push_back(header.action);
    append_little_endian_uint32(bytes, header.total_length);
    return bytes;
}

std::vector<unsigned char> encode_response_header(const ResponseHeader& header) {
    std::vector<unsigned char> bytes;
    bytes.reserve(kResponseHeaderSize);
    bytes.push_back(header.code);
    append_little_endian_uint32(bytes, header.total_length);
    return bytes;
}

bool decode_request_header(const unsigned char* bytes, std::size_t size, RequestHeader& header) {
    if (bytes == 0 || size != kRequestHeaderSize)
        return false;

    header.action = bytes[0];
    header.total_length = read_little_endian_uint32(bytes + 1);
    return true;
}

bool decode_response_header(const unsigned char* bytes, std::size_t size, ResponseHeader& header) {
    if (bytes == 0 || size != kResponseHeaderSize)
        return false;

    header.code = bytes[0];
    header.total_length = read_little_endian_uint32(bytes + 1);
    return true;
}

bool build_request_frame(std::uint8_t action, std::uint32_t client_pid,
    const std::vector<unsigned char>& payload, std::vector<unsigned char>& frame) {
    (void)client_pid;
    if (payload.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
        return false;

    RequestHeader header = {};
    header.action = action;
    header.total_length = static_cast<std::uint32_t>(payload.size());
    frame = encode_request_header(header);
    frame.insert(frame.end(), payload.begin(), payload.end());
    return true;
}

} // namespace client
