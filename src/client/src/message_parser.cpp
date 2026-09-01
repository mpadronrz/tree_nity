#include "message_parser.hpp"

#include <cstdint>
#include <limits>

namespace client {
namespace {

const std::size_t kMaximumMessageSize = 1024;

// Distingue un EOF limpio entre registros de un EOF que corta un campo binario.
// EOF significa “End Of File”: el momento en que un programa llega al final de la entrada
enum class IntegerReadStatus {
    Ok,
    EndOfFile,
    Partial
};

// Construye un resultado de error sin perder los registros completos leídos antes del fallo.
MessageParseResult make_failure(const std::vector<Message>& messages, const std::string& error) {
    MessageParseResult result = {};
    result.ok = false;
    result.messages = messages;
    result.error = error;
    return result;
}

// Construye el resultado correcto al llegar a EOF en un límite de registro válido.
MessageParseResult make_success(const std::vector<Message>& messages) {
    MessageParseResult result = {};
    result.ok = true;
    result.messages = messages;
    return result;
}

// Comprueba el límite de 1024 bytes para key + body antes de aceptar el mensaje.
bool has_valid_message_size(const std::string& key, const std::string& value) {
    if (key.size() > kMaximumMessageSize)
        return false;
    return value.size() <= kMaximumMessageSize - key.size();
}

// Lee exactamente cuatro bytes en little-endian y detecta si el campo quedó truncado.
IntegerReadStatus read_little_endian_int32(std::istream& input, std::uint32_t& number) {
    char bytes[4] = {};
    input.read(bytes, 4);
    const std::streamsize read_count = input.gcount();
    // EOF con cero bytes leídos significa que el registro anterior acabó bien y no hay otro.
    if (read_count == 0 && input.eof())
        return IntegerReadStatus::EndOfFile;
    // EOF después de uno, dos o tres bytes corta un int32 y deja el registro incompleto.
    if (read_count != 4)
        return IntegerReadStatus::Partial;

    number = static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[0]));
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[1])) << 8;
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[2])) << 16;
    number |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[3])) << 24;
    return IntegerReadStatus::Ok;
}

// Lee una secuencia binaria de tamaño conocido; los tamaños cero no requieren tocar el buffer.
bool read_bytes(std::istream& input, std::size_t size, std::string& value) {
    value.clear();
    if (size == 0)
        return true;

    value.resize(size);
    input.read(&value[0], static_cast<std::streamsize>(size));
    return input.gcount() == static_cast<std::streamsize>(size);
}

// Convierte un tamaño codificado como int32 y rechaza los negativos antes de reservar memoria.
bool is_valid_binary_size(std::uint32_t encoded_size) {
    return encoded_size <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
}

} // namespace

MessageParseResult parse_text_messages(std::istream& input) {
    std::vector<Message> messages;
    std::string line;

    // getline conserva todos los caracteres del body salvo el salto de línea separador.
    while (std::getline(input, line)) {
        const std::string::size_type separator = line.find(':');
        if (separator == std::string::npos)
            return make_failure(messages, "text message must contain ':' between key and body");

        Message message;
        message.key = line.substr(0, separator);
        message.value = line.substr(separator + 1);
        if (!has_valid_message_size(message.key, message.value))
            return make_failure(messages, "message key and body exceed 1024 bytes");
        messages.push_back(message);
    }

    // Si getline terminó sin EOF, stdin tuvo un error de lectura distinto de llegar al final.
    if (!input.eof())
        return make_failure(messages, "failed to read text input");
    return make_success(messages);
}

MessageParseResult parse_raw_messages(std::istream& input) {
    std::vector<Message> messages;

    // Cada iteración consume un registro completo: tamaño key, key, tamaño value y value.
    while (true) {
        std::uint32_t encoded_key_size = 0;
        const IntegerReadStatus key_size_status = read_little_endian_int32(input, encoded_key_size);
        // EOF justo antes del siguiente tamaño es válido: todos los registros anteriores están completos.
        if (key_size_status == IntegerReadStatus::EndOfFile)
            return make_success(messages);
        // EOF dentro de un tamaño, key, value_size o value es un error de productor.
        if (key_size_status == IntegerReadStatus::Partial)
            return make_failure(messages, "partial raw record at EOF");
        if (!is_valid_binary_size(encoded_key_size))
            return make_failure(messages, "raw key size must be a non-negative int32");

        Message message;
        const std::size_t key_size = static_cast<std::size_t>(encoded_key_size);
        if (key_size > kMaximumMessageSize)
            return make_failure(messages, "message key and body exceed 1024 bytes");
        if (!read_bytes(input, key_size, message.key))
            return make_failure(messages, "partial raw record at EOF");

        std::uint32_t encoded_value_size = 0;
        const IntegerReadStatus value_size_status = read_little_endian_int32(input, encoded_value_size);
        if (value_size_status != IntegerReadStatus::Ok)
            return make_failure(messages, "partial raw record at EOF");
        if (!is_valid_binary_size(encoded_value_size))
            return make_failure(messages, "raw value size must be a non-negative int32");

        const std::size_t value_size = static_cast<std::size_t>(encoded_value_size);
        if (value_size > kMaximumMessageSize - key_size)
            return make_failure(messages, "message key and body exceed 1024 bytes");
        if (!read_bytes(input, value_size, message.value))
            return make_failure(messages, "partial raw record at EOF");
        messages.push_back(message);
    }
}

} // namespace client
