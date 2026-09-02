#ifndef CLIENT_PROTOCOL_IO_HPP
#define CLIENT_PROTOCOL_IO_HPP

#include "fifo_io.hpp"
#include "protocol/Protocol.hpp"

#include <cstdint>
#include <vector>

namespace client {

// Representa una respuesta completa, separada de la forma en que se recibió por el descriptor.
struct ResponseFrame {
    Protocol::StatusCode code = Protocol::StatusCode::SUCCESS;
    std::vector<unsigned char> payload;
};

// Explica si falló el header, el payload o la validación del frame recibido.
enum class ResponseReadStatus {
    Complete,
    EndOfFile,
    PartialEndOfFile,
    Error,
    InvalidFrame
};

struct ResponseReadResult {
    ResponseReadStatus status;
    int error_number;
};

// Escribe un frame entero, para preservar la delimitación acordada del request.
bool write_request_frame(int file_descriptor, const std::vector<unsigned char>& frame, int& error_number);

// Lee primero los cinco bytes de header y después exactamente el payload anunciado por la respuesta.
ResponseReadResult read_response_frame(int file_descriptor, ResponseFrame& response);

} // namespace client

#endif
