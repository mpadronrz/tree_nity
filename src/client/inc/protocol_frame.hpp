#ifndef CLIENT_PROTOCOL_FRAME_HPP
#define CLIENT_PROTOCOL_FRAME_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace client {

// Los encabezados permiten recibir primero una comunicación completa y parsearla después.
const std::size_t kRequestHeaderSize = 9;
const std::size_t kResponseHeaderSize = 5;

// Describe los nueve bytes iniciales de toda petición enviada al FIFO principal del servidor.
struct RequestHeader {
    std::uint8_t action;
    std::uint32_t client_pid;
    std::uint32_t total_length;
};

// Describe los cinco bytes iniciales de toda respuesta recibida desde el servidor.
struct ResponseHeader {
    std::uint8_t code;
    std::uint32_t total_length;
};

// Convierte un encabezado de petición a sus nueve bytes con enteros little-endian.
std::vector<unsigned char> encode_request_header(const RequestHeader& header);

// Convierte un encabezado de respuesta a sus cinco bytes con enteros little-endian.
std::vector<unsigned char> encode_response_header(const ResponseHeader& header);

// Lee y valida un encabezado de petición; la longitud total nunca puede ser menor que nueve.
bool decode_request_header(const unsigned char* bytes, std::size_t size, RequestHeader& header);

// Lee y valida un encabezado de respuesta; la longitud total nunca puede ser menor que cinco.
bool decode_response_header(const unsigned char* bytes, std::size_t size, ResponseHeader& header);

// Une encabezado y payload para que el cliente pueda escribir una petición como un único frame.
bool build_request_frame(std::uint8_t action, std::uint32_t client_pid,
    const std::vector<unsigned char>& payload, std::vector<unsigned char>& frame);

} // namespace client

#endif
