#include "Server.hpp"
#include <csignal>
#include <iostream>
#include <unistd.h>

// Puntero global temporal solo para que la señal pueda avisar al Server
Server* g_server = nullptr;

void signal_handler(int signum)
{
    (void)signum;
    if (g_server != nullptr)
	{
        // Le avisamos al servidor que empiece el apagado
        g_server->stop(); 
    }
}

int main()
{
    std::string ipc_path = "/tmp/treenity.server." + std::to_string(getpid());
    
    Server server(ipc_path);
    g_server = &server;

    // Registramos la captura de SIGINT, SIGTERM e ignoramos SIGPIPE para desconexiones de clientes
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, SIG_IGN);

    std::cout << ipc_path << std::endl; // Exigido por el subject

    // Bucle principal del servidor
    server.run(); 

    return 0;
}