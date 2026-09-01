#ifndef CLIENT_IPC_EXCHANGE_HPP
#define CLIENT_IPC_EXCHANGE_HPP

#include "protocol_io.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace client {

// Distingue un identificador de servidor inválido de un fallo durante una comunicación ya iniciada.
enum class IpcExchangeStatus {
    Complete,
    ServerUnavailable,
    CommunicationError,
    InvalidResponse
};

struct IpcExchangeResult {
    IpcExchangeStatus status;
    int error_number;
    ResponseFrame response;
};

// Construye la ruta privada acordada que el servidor obtiene a partir del PID incluido en el request.
std::string client_fifo_path(std::uint32_t client_pid);

// Ejecuta una petición con respuesta: crea el FIFO privado, envía el frame y lo elimina al finalizar.
IpcExchangeResult exchange_request(const std::string& server_fifo, std::uint32_t client_pid,
    const std::vector<unsigned char>& request);

} // namespace client

#endif
