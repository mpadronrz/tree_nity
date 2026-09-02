#include "shutdown_signal.hpp"

#include <csignal>

namespace client {
namespace {

volatile std::sig_atomic_t g_shutdown_requested = 0;

void request_shutdown(int signal_number) {
    (void)signal_number;
    g_shutdown_requested = 1;
}

}

bool install_shutdown_handlers() {
    struct sigaction action = {};
    action.sa_handler = request_shutdown;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, 0) == -1)
        return false;
    if (sigaction(SIGTERM, &action, 0) == -1)
        return false;
    return true;
}

bool shutdown_requested() {
    return g_shutdown_requested != 0;
}

void reset_shutdown_request_for_tests() {
    g_shutdown_requested = 0;
}

}
