#ifndef CLIENT_MANAGEMENT_COMMAND_HPP
#define CLIENT_MANAGEMENT_COMMAND_HPP

#include "command.hpp"

#include <ostream>

namespace client {

// Ejecuta create, list o info mediante FIFO y escribe únicamente la salida exigida por el subject.
int execute_management_command(const Command& command, std::ostream& output, std::ostream& error_output);

} // namespace client

#endif
