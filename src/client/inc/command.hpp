#ifndef CLIENT_COMMAND_HPP
#define CLIENT_COMMAND_HPP

#include <cstdint>
#include <string>

namespace client {

// Identifica la acción que el cliente debe solicitar posteriormente al servidor.
enum class CommandType {
    Create,
    List,
    Produce,
    Subscribe,
    Info
};

// Agrupa los datos ya validados de una llamada a ./client.
struct Command {
    CommandType type;
    std::string ipc_identifier;
    std::string topic;
    std::string subscriber;
    std::string prefix;
    std::uint32_t offset;
    bool has_offset;
    bool raw;
};

// Separa el resultado correcto de un error de sintaxis o de validación.
struct ParseResult {
    bool ok;
    Command command;
    std::string error;
};

// Interpreta los argumentos de línea de comandos sin realizar todavía operaciones FIFO.
ParseResult parse_command(int argc, char *const argv[]);

} // namespace client

#endif
