#ifndef CLIENT_SUBSCRIBER_COMMAND_HPP
#define CLIENT_SUBSCRIBER_COMMAND_HPP

#include "command.hpp"

#include <ostream>

namespace client {

// Registra un consumidor, escucha su FIFO dedicado, imprime mensajes y confirma cada offset.
int execute_subscriber_command(const Command& command);

} // namespace client

#endif
