#include "ipc_exchange.hpp"

#include "protocol_io.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace client {
namespace {

// Cierra un descriptor válido sin sustituir el error principal que ya se iba a comunicar al usuario.
void close_if_open(int file_descriptor) {
    if (file_descriptor >= 0)
        close(file_descriptor);
}

// Elimina solamente el FIFO privado creado para esta petición, incluso cuando falla un paso intermedio.
void remove_client_fifo(const std::string& path) {
    unlink(path.c_str());
}

// Construye un resultado uniforme para que main pueda traducirlo después al código de salida correcto.
IpcExchangeResult make_result(IpcExchangeStatus status, int error_number) {
    IpcExchangeResult result = {};
    result.status = status;
    result.error_number = error_number;
    result.response.code = 0;
    return result;
}

} // namespace

std::string client_fifo_path(std::uint32_t client_pid) {
    std::ostringstream path;
    path << "/tmp/treenity.client." << client_pid;
    return path.str();
}

IpcExchangeResult exchange_request(const std::string& server_fifo, std::uint32_t client_pid,
    const std::vector<unsigned char>& request) {
    const std::string response_fifo = client_fifo_path(client_pid);
    if (mkfifo(response_fifo.c_str(), S_IRUSR | S_IWUSR) == -1)
        return make_result(IpcExchangeStatus::CommunicationError, errno);

    // O_RDWR mantiene abierto un extremo escritor propio hasta que el servidor abra el suyo.
    const int response_descriptor = open(response_fifo.c_str(), O_RDWR);
    if (response_descriptor == -1) {
        const int error_number = errno;
        remove_client_fifo(response_fifo);
        return make_result(IpcExchangeStatus::CommunicationError, error_number);
    }

    // O_NONBLOCK evita que un identificador inexistente deje bloqueado al cliente al abrir el FIFO principal.
    const int server_descriptor = open(server_fifo.c_str(), O_WRONLY | O_NONBLOCK);
    if (server_descriptor == -1) {
        const int error_number = errno;
        close_if_open(response_descriptor);
        remove_client_fifo(response_fifo);
        return make_result(IpcExchangeStatus::ServerUnavailable, error_number);
    }

    int write_error = 0;
    if (!write_request_frame(server_descriptor, request, write_error)) {
        close_if_open(server_descriptor);
        close_if_open(response_descriptor);
        remove_client_fifo(response_fifo);
        return make_result(IpcExchangeStatus::CommunicationError, write_error);
    }
    close_if_open(server_descriptor);

    IpcExchangeResult result = make_result(IpcExchangeStatus::Complete, 0);
    const ResponseReadResult response_read = read_response_frame(response_descriptor, result.response);
    close_if_open(response_descriptor);
    remove_client_fifo(response_fifo);

    if (response_read.status == ResponseReadStatus::Complete)
        return result;
    if (response_read.status == ResponseReadStatus::InvalidFrame)
        return make_result(IpcExchangeStatus::InvalidResponse, response_read.error_number);
    return make_result(IpcExchangeStatus::CommunicationError, response_read.error_number);
}

} // namespace client
