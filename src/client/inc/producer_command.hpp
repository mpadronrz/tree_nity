#ifndef CLIENT_PRODUCER_COMMAND_HPP
#define CLIENT_PRODUCER_COMMAND_HPP

#include "command.hpp"

namespace client {

// Ejecuta la conexión del productor y publica los mensajes leídos desde std::cin.
int execute_producer_command(const Command& command);

} // namespace client

#endif
