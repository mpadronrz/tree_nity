#include "message_output.hpp"

#include <iostream>
#include <limits>

namespace client {
namespace {

// Escribe un entero de 32 bits byte a byte para fijar explícitamente el orden little-endian.
bool write_little_endian_int32(std::ostream& output, std::uint32_t number) {
    char bytes[4] = {};
    bytes[0] = static_cast<char>(number & 0xffU);
    bytes[1] = static_cast<char>((number >> 8) & 0xffU);
    bytes[2] = static_cast<char>((number >> 16) & 0xffU);
    bytes[3] = static_cast<char>((number >> 24) & 0xffU);
    output.write(bytes, 4);
    return output.good();
}

// Los tamaños del protocolo son int32, por tanto no se pueden codificar valores mayores.
bool can_encode_size(std::string::size_type size) {
    return size <= static_cast<std::string::size_type>(std::numeric_limits<std::int32_t>::max());
}

} // namespace

std::string format_text_message(const Message& message) {
    // El salto de línea separa mensajes consecutivos en la salida de texto del consumidor.
    return message.key + ":" + message.value + "\n";
}

bool write_raw_message(std::ostream& output, std::uint32_t offset, const Message& message) {
    // La validación evita truncar silenciosamente un tamaño al convertirlo a int32.
    if (!can_encode_size(message.key.size()) || !can_encode_size(message.value.size()))
        return false;

    // El orden de estos campos coincide exactamente con el especificado para --raw del consumidor.
    if (!write_little_endian_int32(output, offset))
        return false;
    if (!write_little_endian_int32(output, static_cast<std::uint32_t>(message.key.size())))
        return false;
    output.write(message.key.data(), static_cast<std::streamsize>(message.key.size()));
    if (!output.good())
        return false;
    if (!write_little_endian_int32(output, static_cast<std::uint32_t>(message.value.size())))
        return false;
    output.write(message.value.data(), static_cast<std::streamsize>(message.value.size()));
    return output.good();
}

void write_output(const std::vector<unsigned char>& payload) {
    if (!payload.empty())
        std::cout.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    std::cout.put('\n');
}

void write_error(const std::vector<unsigned char>& payload) {
    if (payload.empty()) {
        std::cerr << "server returned an error" << '\n';
        return;
    }
    std::cerr.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    std::cerr.put('\n');
}

} // namespace client
