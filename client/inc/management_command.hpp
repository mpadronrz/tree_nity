#ifndef CLIENT_MANAGEMENT_COMMAND_HPP
#define CLIENT_MANAGEMENT_COMMAND_HPP

#include "command.hpp"

#include <cstdint>
#include <ostream>

namespace client {

// Traduce el código de respuesta acordado al código de salida público del cliente.
int exit_code_for_response(std::uint8_t response_code);

// Ejecuta create, list o info mediante FIFO y escribe únicamente la salida exigida por el subject.
int execute_management_command(const Command& command, std::ostream& output, std::ostream& error_output);

} // namespace client

#endif
