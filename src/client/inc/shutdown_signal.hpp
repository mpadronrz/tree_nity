#ifndef CLIENT_SHUTDOWN_SIGNAL_HPP
#define CLIENT_SHUTDOWN_SIGNAL_HPP

namespace client {

bool install_shutdown_handlers();
bool shutdown_requested();
void reset_shutdown_request_for_tests();

}

#endif
