#pragma once

#include <cstddef>
#include <chrono>
#include <string>

namespace ipc {

    bool is_reader_active(const std::string& path);

    class FifoReader {
    public:
        FifoReader() = default;
        ~FifoReader();

        FifoReader(const FifoReader&) = delete;
        FifoReader& operator=(const FifoReader&) = delete;
        FifoReader(FifoReader&& other) noexcept;
        FifoReader& operator=(FifoReader&& other) noexcept;

        static FifoReader create(const std::string& path, bool read_write = false);
        bool read_exact(void* dest, size_t count, int timeout_ms = -1);

        [[nodiscard]] int fd() const noexcept { return fd_; }
        [[nodiscard]] bool is_open() const noexcept { return fd_ >= 0; }
        [[nodiscard]] const std::string& path() const noexcept { return path_; }
        void close();

    private:
        FifoReader(int fd, std::string path);

        int fd_{-1};
        std::string path_{};
    };

    class FifoWriter {
    public:
        FifoWriter() = default;
        ~FifoWriter();

        FifoWriter(const FifoWriter&) = delete;
        FifoWriter& operator=(const FifoWriter&) = delete;
        FifoWriter(FifoWriter&& other) noexcept;
        FifoWriter& operator=(FifoWriter&& other) noexcept;

        static FifoWriter open(const std::string& path,
                               std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
        bool write_exact(const void* src, size_t count);

        [[nodiscard]] int fd() const noexcept { return fd_; }
        [[nodiscard]] bool is_open() const noexcept { return fd_ >= 0; }
        void close();

    private:
        FifoWriter(int fd, std::string path);

        int fd_{-1};
        std::string path_{};
    };

}
