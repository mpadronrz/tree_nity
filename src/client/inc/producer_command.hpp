#ifndef CLIENT_PRODUCER_COMMAND_HPP
#define CLIENT_PRODUCER_COMMAND_HPP

#include "command.hpp"

#include <istream>
#include <ostream>

namespace client {

// Ejecuta la conexión del productor y publica todos los mensajes leídos desde stdin.
int execute_producer_command(const Command& command, std::istream& input, std::ostream& error_output);

} // namespace client

#endif
