#include "exit_status.hpp"

namespace client {
namespace {

// Asigna una prioridad numérica para poder aplicar la regla del enunciado de forma centralizada.
int priority_for(ClientError error) {
    if (error == ClientError::General)
        return 3;
    if (error == ClientError::Ipc)
        return 2;
    if (error == ClientError::Topic)
        return 1;
    return 0;
}

// Traduce el tipo de error ganador al código público del ejecutable.
int code_for(ClientError error) {
    if (error == ClientError::General)
        return 1;
    if (error == ClientError::Ipc)
        return 3;
    if (error == ClientError::Topic)
        return 2;
    return 0;
}

} // namespace

int exit_code_for(ClientError first, ClientError second) {
    // Si coinciden varias causas, conserva la de mayor prioridad: 1 > 3 > 2.
    if (priority_for(second) > priority_for(first))
        return code_for(second);
    return code_for(first);
}

int exit_code_from_response_code(std::uint8_t response_code) {
    if (response_code == 0U)
        return 0;
    if (response_code == 1U)
        return 1;
    if (response_code == 2U)
        return 2;
    if (response_code == 3U)
        return 3;
    return 3;
}

} // namespace client
