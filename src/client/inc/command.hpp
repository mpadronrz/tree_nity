#ifndef CLIENT_COMMAND_HPP
#define CLIENT_COMMAND_HPP

#include "protocol/Protocol.hpp"

#include <cstdint>
#include <string>

namespace client {

struct Command {
    Protocol::CommandType type;
    std::string ipc_identifier;
    std::string topic;
    std::string subscriber;
    std::string prefix;
    std::uint32_t offset;
    bool has_offset;
    bool raw;
};

struct ParseResult {
    bool ok;
    Command command;
    std::string error;
};

ParseResult parse_command(int argc, char *const argv[]);

}
#endif
