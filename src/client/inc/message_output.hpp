#ifndef CLIENT_MESSAGE_OUTPUT_HPP
#define CLIENT_MESSAGE_OUTPUT_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace client {

struct Message {
    std::string key;
    std::string value;
};

// Construye la línea key:body que debe mostrar un consumidor en el modo por defecto.
std::string format_text_message(const Message& message);

// Escribe un registro del consumidor en formato raw: offset, keysize, key, valuesize y value.
bool write_raw_message(std::ostream& output, std::uint32_t offset, const Message& message);

// Escribe el payload devuelto por el servidor en stdout seguido de salto de línea.
void write_output(const std::vector<unsigned char>& payload);

// Escribe el payload de error del servidor (o un mensaje por defecto) en stderr.
void write_error(const std::vector<unsigned char>& payload);

} // namespace client

#endif
