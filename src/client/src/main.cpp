#include "command.hpp"
#include "management_command.hpp"
#include "producer_command.hpp"
#include "shutdown_signal.hpp"
#include "subscriber_command.hpp"

#include <iostream>
#include <csignal>

int main(int argc, char *argv[]) {
    if (!client::install_shutdown_handlers()) {
        std::cerr << "failed to install shutdown signal handlers" << '\n';
        return 1;
    }
    std::signal(SIGPIPE, SIG_IGN);

    const client::ParseResult result = client::parse_command(argc, argv);
    if (!result.ok) {
        std::cerr << result.error << '\n';
        return 1;
    }

    if (result.command.type == Protocol::CommandType::CREATE)
        return client::execute_create_command(result.command);
    else if (result.command.type == Protocol::CommandType::LIST)
        return client::execute_list_command(result.command);
    else if (result.command.type == Protocol::CommandType::INFO)
        return client::execute_info_command(result.command);
    else if (result.command.type == Protocol::CommandType::CONNECT)
        return client::execute_producer_command(result.command);

    return client::execute_subscriber_command(result.command);
}
