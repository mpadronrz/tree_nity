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
        g_server->stop();
    std::cout << "\rCtrl + C pressed\n";
}

int main()
{
    std::string ipc_path = "/tmp/treenity.server." + std::to_string(getpid());
    
    Server server(ipc_path);
    g_server = &server;

    if (!server.is_correct_start())
    {
        std::cout << "Server fifo creation failed\n";
        return (1);
    }

    // SIGPIPE is ignored for client disconnection
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, SIG_IGN);

    std::cout << ipc_path << std::endl;

    // Main loop
    server.run();

    return 0;
}
