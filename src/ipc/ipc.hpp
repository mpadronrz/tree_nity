#pragma once

#include <cstddef>
#include <chrono>
#include <string>

namespace ipc {

    // Masks SIGPIPE globally so writes to broken pipes return EPIPE instead of killing the process
    void ignore_sigpipe() noexcept;

    // Checks if a reader is currently attached to the FIFO node
    bool is_reader_active(const std::string& path);

    // ========================================================================
    // FifoReader: Creates the FIFO, reads from it, and unlinks it on destruction
    // ========================================================================
    class FifoReader {
    public:
        FifoReader() = default;
        ~FifoReader();

        // Move-only: prevents duplicate closes/unlinks
        FifoReader(const FifoReader&) = delete;
        FifoReader& operator=(const FifoReader&) = delete;
        FifoReader(FifoReader&& other) noexcept;
        FifoReader& operator=(FifoReader&& other) noexcept;

        // Creates the node via mkfifo and opens descriptor.
        // If read_write is true, opens with O_RDWR (keeps dummy writer attached to avoid spinning on EOF)
        static FifoReader create(const std::string& path, bool read_write = false);

        // Guarantees all count bytes are read; retries on EINTR, returns false on EOF/error
        bool read_exact(void* dest, size_t count);

        [[nodiscard]] int fd() const noexcept { return fd_; }
        [[nodiscard]] bool is_open() const noexcept { return fd_ >= 0; }
        [[nodiscard]] const std::string& path() const noexcept { return path_; }
        void close();

    private:
        FifoReader(int fd, std::string path);

        int fd_{-1};
        std::string path_{};
    };

    // ========================================================================
    // FifoWriter: Opens existing FIFO, writes to it, and closes descriptor on destruction
    // ========================================================================
    class FifoWriter {
    public:
        FifoWriter() = default;
        ~FifoWriter();

        // Move-only: prevents duplicate descriptor closes
        FifoWriter(const FifoWriter&) = delete;
        FifoWriter& operator=(const FifoWriter&) = delete;
        FifoWriter(FifoWriter&& other) noexcept;
        FifoWriter& operator=(FifoWriter&& other) noexcept;

        // Connects to an existing FIFO with a non-blocking timeout loop
        static FifoWriter open(const std::string& path,
                               std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

        // Guarantees all count bytes are written; retries on EINTR, returns false on EPIPE/error
        bool write_exact(const void* src, size_t count);

        [[nodiscard]] int fd() const noexcept { return fd_; }
        [[nodiscard]] bool is_open() const noexcept { return fd_ >= 0; }
        void close();

    private:
        FifoWriter(int fd, std::string path);

        int fd_{-1};
        std::string path_{};
    };

} // namespace ipc
