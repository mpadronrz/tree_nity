#ifndef CLIENT_MESSAGE_OUTPUT_HPP
#define CLIENT_MESSAGE_OUTPUT_HPP

#include "message_parser.hpp"

#include <cstdint>
#include <ostream>
#include <string>

namespace client {

// Construye la línea key:body que debe mostrar un consumidor en el modo por defecto.
std::string format_text_message(const Message& message);

// Escribe un registro del consumidor en formato raw: offset, keysize, key, valuesize y value.
bool write_raw_message(std::ostream& output, std::uint32_t offset, const Message& message);

} // namespace client

#endif
