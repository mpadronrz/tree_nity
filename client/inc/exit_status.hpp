#ifndef CLIENT_EXIT_STATUS_HPP
#define CLIENT_EXIT_STATUS_HPP

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

} // namespace client

#endif
