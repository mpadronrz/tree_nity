#ifndef CLIENT_PROTOCOL_PAYLOAD_HPP
#define CLIENT_PROTOCOL_PAYLOAD_HPP

#include <string>
#include <vector>

namespace client {

// Codifica una cadena como [longitud:uint32 little-endian][bytes de la cadena].
bool encode_single_string_payload(const std::string& value, std::vector<unsigned char>& payload);

// Decodifica un payload formado por una única cadena y rechaza bytes sobrantes o truncados.
bool decode_single_string_payload(const std::vector<unsigned char>& payload, std::string& value);

} // namespace client

#endif
