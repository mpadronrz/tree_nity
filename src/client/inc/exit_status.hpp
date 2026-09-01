#ifndef CLIENT_EXIT_STATUS_HPP
#define CLIENT_EXIT_STATUS_HPP

#include <cstdint>

namespace client {

// Enumera los tipos de error que puede detectar el cliente según el enunciado.
enum class ClientError {
    None,
    Topic,
    Ipc,
    General
};

// Calcula el código final respetando la prioridad General > IPC > Topic.
int exit_code_for(ClientError first, ClientError second = ClientError::None);

// Traduce el código de una response del servidor al código de salida del ejecutable cliente.
int exit_code_from_response_code(std::uint8_t response_code);

} // namespace client

#endif
