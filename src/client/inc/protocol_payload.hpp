#ifndef CLIENT_PROTOCOL_PAYLOAD_HPP
#define CLIENT_PROTOCOL_PAYLOAD_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace client {

// Codifica una cadena como [longitud:uint32 little-endian][bytes de la cadena].
bool encode_single_string_payload(const std::string& value, std::vector<unsigned char>& payload);

// Decodifica un payload formado por una única cadena y rechaza bytes sobrantes o truncados.
bool decode_single_string_payload(const std::vector<unsigned char>& payload, std::string& value);

// Codifica dos cadenas consecutivas, formato usado para key y value en una publicación.
bool encode_two_string_payload(const std::string& first, const std::string& second,
    std::vector<unsigned char>& payload);

// Decodifica exactamente dos cadenas consecutivas y rechaza campos truncados o sobrantes.
bool decode_two_string_payload(const std::vector<unsigned char>& payload, std::string& first,
    std::string& second);

// Payload de SubscriberConnect: topic, nombre, prefijo, indicador de offset y offset solicitado.
bool encode_subscriber_payload(const std::string& topic, const std::string& subscriber,
    const std::string& prefix, bool has_offset, std::uint32_t offset,
    std::vector<unsigned char>& payload);

// Payload de SubscriberCommit: nombre del suscriptor y próximo offset que se desea consumir.
bool encode_commit_payload(const std::string& subscriber, std::uint32_t offset,
    std::vector<unsigned char>& payload);

// Payload de Disconnect: nombre del suscriptor que abandona su registro.
bool encode_disconnect_payload(const std::string& subscriber, std::vector<unsigned char>& payload);

} // namespace client

#endif
