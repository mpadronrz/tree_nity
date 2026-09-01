#include "fifo_io.hpp"

#include <cerrno>
#include <unistd.h>

namespace client {

bool write_all(int file_descriptor, const void* data, std::size_t size, int& error_number) {
    const char* bytes = static_cast<const char*>(data);
    std::size_t written = 0;
    error_number = 0;

    // write puede escribir menos bytes que los solicitados, por eso se repite hasta completarlos.
    while (written < size) {
        const ssize_t result = write(file_descriptor, bytes + written, size - written);
        if (result > 0) {
            written += static_cast<std::size_t>(result);
            continue;
        }
        // Una señal puede interrumpir write sin consumir datos; en ese caso se intenta de nuevo.
        if (result == -1 && errno == EINTR)
            continue;

        error_number = (result == -1) ? errno : EIO;
        return false;
    }
    return true;
}

FifoReadResult read_exact(int file_descriptor, void* data, std::size_t size) {
    char* bytes = static_cast<char*>(data);
    std::size_t read_count = 0;
    FifoReadResult result = {};
    result.status = FifoReadStatus::Complete;
    result.error_number = 0;

    // read también puede entregar el mensaje IPC por partes, incluso si el emisor lo escribió de una vez.
    while (read_count < size) {
        const ssize_t received = read(file_descriptor, bytes + read_count, size - read_count);
        if (received > 0) {
            read_count += static_cast<std::size_t>(received);
            continue;
        }
        // EINTR no significa desconexión: el bucle puede reanudar la lectura segura.
        if (received == -1 && errno == EINTR)
            continue;
        // EOF sin bytes pertenece a un límite de mensaje; con bytes pendientes es un mensaje truncado.
        if (received == 0)
            result.status = (read_count == 0) ? FifoReadStatus::EndOfFile : FifoReadStatus::PartialEndOfFile;
        else {
            result.status = FifoReadStatus::Error;
            result.error_number = errno;
        }
        return result;
    }
    return result;
}

} // namespace client
