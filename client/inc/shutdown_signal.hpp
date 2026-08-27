#ifndef CLIENT_SHUTDOWN_SIGNAL_HPP
#define CLIENT_SHUTDOWN_SIGNAL_HPP

namespace client {

// Instala handlers mínimos para SIGINT y SIGTERM; el cierre real se hará fuera del handler.
bool install_shutdown_handlers();

// Permite que el bucle del consumidor compruebe si debe desconectarse de forma ordenada.
bool shutdown_requested();

// Restablece el estado entre tests; no debe usarse en el flujo normal del programa.
void reset_shutdown_request_for_tests();

} // namespace client

#endif
