#include "command.hpp"
#include "management_command.hpp"
#include "producer_command.hpp"
#include "shutdown_signal.hpp"

#include <iostream>

int main(int argc, char *argv[]) {
    // Desde el inicio el cliente puede reaccionar a Ctrl+C o SIGTERM sin terminar abruptamente.
    if (!client::install_shutdown_handlers()) {
        std::cerr << "failed to install shutdown signal handlers" << '\n';
        return 1;
    }

    // El main delega la validación para que la lógica pueda probarse sin lanzar procesos.
    const client::ParseResult result = client::parse_command(argc, argv);
    if (!result.ok) {
        // Los errores de argumentos se escriben en stderr y usan el código general 1.
        std::cerr << result.error << '\n';
        return 1;
    }

    // Las operaciones de gestión ya comparten el ciclo completo de request y response por FIFO.
    if (result.command.type == client::CommandType::Create || result.command.type == client::CommandType::List
        || result.command.type == client::CommandType::Info)
        return client::execute_management_command(result.command, std::cout, std::cerr);
    if (result.command.type == client::CommandType::Produce)
        return client::execute_producer_command(result.command, std::cin, std::cerr);

    // Subscribe se conectará cuando estén listos su FIFO dedicado y su bucle de escucha.
    std::cerr << "command not implemented yet" << '\n';
    return 1;
}
