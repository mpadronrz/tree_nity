#include "command.hpp"
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

    // La comunicación por FIFO se conectará en el siguiente punto del desarrollo.
    return 0;
}
