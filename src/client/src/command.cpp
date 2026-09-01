#include "command.hpp"

#include <cctype>
#include <limits>

namespace client {
namespace {

// Crea una estructura con valores seguros para que incluso los errores tengan estado definido.
Command make_empty_command() {
    Command command = {};
    command.type = CommandType::List;
    command.offset = 0;
    command.has_offset = false;
    command.raw = false;
    return command;
}

// Construye de forma uniforme un resultado de error para el main y para los tests.
ParseResult make_failure(const std::string& error) {
    ParseResult result = {};
    result.ok = false;
    result.command = make_empty_command();
    result.error = error;
    return result;
}

// Construye un resultado correcto después de completar todas las validaciones necesarias.
ParseResult make_success(const Command& command) {
    ParseResult result = {};
    result.ok = true;
    result.command = command;
    return result;
}

// Comprueba la regla del enunciado para nombres de topic y de suscriptor: 1 a 32 caracteres permitidos.
bool is_valid_name(const std::string& value) {
    if (value.empty() || value.size() > 32)
        return false;

    for (std::string::size_type index = 0; index < value.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(value[index]);
        if (!std::isalnum(character) && value[index] != '_' && value[index] != '.' && value[index] != '-')
            return false;
    }
    return true;
}

// Convierte un offset decimal a uint32_t y evita tanto signos como desbordamientos.
bool parse_offset(const std::string& value, std::uint32_t& offset) {
    if (value.empty())
        return false;

    std::uint64_t number = 0;
    for (std::string::size_type index = 0; index < value.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(value[index])))
            return false;
        number = number * 10 + static_cast<unsigned int>(value[index] - '0');
        if (number > std::numeric_limits<std::uint32_t>::max())
            return false;
    }
    offset = static_cast<std::uint32_t>(number);
    return true;
}

// Procesa las opciones opcionales exclusivas del subcomando subscribe.
ParseResult parse_subscribe(int argc, char *const argv[], Command command) {
    if (argc < 5)
        return make_failure("usage: client <ipc_identifier> subscribe <topic_name> <subscriber_name> [--prefix <prefix>] [--offset <offset>] [--raw]");

    command.topic = argv[3];
    command.subscriber = argv[4];
    if (!is_valid_name(command.topic))
        return make_failure("invalid topic name");
    if (!is_valid_name(command.subscriber))
        return make_failure("invalid subscriber name");

    bool prefix_seen = false;
    bool offset_seen = false;
    for (int index = 5; index < argc; ++index) {
        const std::string option = argv[index];

        // --raw no recibe valor y solo puede aparecer una vez.
        if (option == "--raw") {
            if (command.raw)
                return make_failure("duplicate --raw option");
            command.raw = true;
        // --prefix recibe cualquier cadena, incluido el prefijo vacío que significa "todos".
        } else if (option == "--prefix") {
            if (prefix_seen || ++index >= argc)
                return make_failure("--prefix requires one value and may only be used once");
            command.prefix = argv[index];
            prefix_seen = true;
        // --offset debe ser un entero sin signo dentro del rango de 32 bits.
        } else if (option == "--offset") {
            if (offset_seen || ++index >= argc || !parse_offset(argv[index], command.offset))
                return make_failure("--offset requires one unsigned 32-bit integer and may only be used once");
            command.has_offset = true;
            offset_seen = true;
        } else {
            return make_failure("unknown subscribe option: " + option);
        }
    }
    return make_success(command);
}

} // namespace

ParseResult parse_command(int argc, char *const argv[]) {
    // Todos los subcomandos comparten, como mínimo, el identificador FIFO y la operación.
    if (argc < 3)
        return make_failure("usage: client <ipc_identifier> <create|list|produce|subscribe|info> ...");

    Command command = make_empty_command();
    command.ipc_identifier = argv[1];
    if (command.ipc_identifier.empty())
        return make_failure("invalid IPC identifier");

    const std::string operation = argv[2];
    if (operation == "create") {
        if (argc != 4)
            return make_failure("usage: client <ipc_identifier> create <topic_name>");
        command.type = CommandType::Create;
        command.topic = argv[3];
        if (!is_valid_name(command.topic))
            return make_failure("invalid topic name");
    } else if (operation == "list") {
        if (argc != 3)
            return make_failure("usage: client <ipc_identifier> list");
        command.type = CommandType::List;
    } else if (operation == "produce") {
        if (argc != 4 && argc != 5)
            return make_failure("usage: client <ipc_identifier> produce <topic_name> [--raw]");
        command.type = CommandType::Produce;
        command.topic = argv[3];
        if (!is_valid_name(command.topic))
            return make_failure("invalid topic name");
        if (argc == 5) {
            if (std::string(argv[4]) != "--raw")
                return make_failure("unknown produce option: " + std::string(argv[4]));
            command.raw = true;
        }
    } else if (operation == "subscribe") {
        command.type = CommandType::Subscribe;
        return parse_subscribe(argc, argv, command);
    } else if (operation == "info") {
        if (argc != 4)
            return make_failure("usage: client <ipc_identifier> info <subscriber_name>");
        command.type = CommandType::Info;
        command.subscriber = argv[3];
        if (!is_valid_name(command.subscriber))
            return make_failure("invalid subscriber name");
    } else {
        return make_failure("unknown command: " + operation);
    }
    return make_success(command);
}

} // namespace client
