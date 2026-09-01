#ifndef CLIENT_FIFO_IO_HPP
#define CLIENT_FIFO_IO_HPP

#include <cstddef>

namespace client {

// Describe si una lectura recibió todos los bytes, terminó limpiamente o se interrumpió a mitad.
enum class FifoReadStatus {
    Complete,
    EndOfFile,
    PartialEndOfFile,
    Error
};

// Conserva el resultado y errno para que la capa de protocolo pueda decidir el código de salida.
struct FifoReadResult {
    FifoReadStatus status;
    int error_number;
};

// Escribe todos los bytes aunque write los acepte en varias llamadas.
bool write_all(int file_descriptor, const void* data, std::size_t size, int& error_number);

// Lee exactamente size bytes o informa si el otro extremo del FIFO se cerró antes.
FifoReadResult read_exact(int file_descriptor, void* data, std::size_t size);

} // namespace client

#endif
