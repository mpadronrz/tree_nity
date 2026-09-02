#ifndef CLIENT_MANAGEMENT_COMMAND_HPP
#define CLIENT_MANAGEMENT_COMMAND_HPP

#include "command.hpp"

#include <ostream>

namespace client {

int execute_create_command(const Command& command);
int execute_list_command(const Command& command);
int execute_info_command(const Command& command);

} // namespace client

#endif
