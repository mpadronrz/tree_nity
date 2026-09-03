#include "ipc.hpp"

#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <thread>
#include <utility>

namespace ipc {
    bool is_reader_active(const std::string& path) {
        int fd = ::open(path.c_str(), O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            ::close(fd);
            return true;
        }
        return false;
    }

    FifoReader::FifoReader(int fd, std::string path)
        : fd_(fd), path_(std::move(path)) {}

    FifoReader::~FifoReader() {
        close();
    }

    FifoReader::FifoReader(FifoReader&& other) noexcept
        : fd_(other.fd_), path_(std::move(other.path_)) {
        other.fd_ = -1;
        other.path_.clear();
    }

    FifoReader& FifoReader::operator=(FifoReader&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            path_ = std::move(other.path_);
            other.fd_ = -1;
            other.path_.clear();
        }
        return *this;
    }

    FifoReader FifoReader::create(const std::string& path, bool read_write) {
        ::unlink(path.c_str());

        if (::mkfifo(path.c_str(), 0666) == -1 && errno != EEXIST) {
            return FifoReader(-1, "");
        }

        int flags = read_write ? O_RDWR : (O_RDONLY | O_NONBLOCK);
        int fd = ::open(path.c_str(), flags);
        if (fd < 0) {
            ::unlink(path.c_str());
            return FifoReader(-1, "");
        }

        if (!read_write) {
            int current_flags = ::fcntl(fd, F_GETFL, 0);
            ::fcntl(fd, F_SETFL, current_flags & ~O_NONBLOCK);
        }

        return FifoReader(fd, path);
    }

    bool FifoReader::read_exact(void* dest, size_t count, int timeout_ms) {
        if (fd_ < 0) return false;
        size_t total = 0;
        auto* ptr = static_cast<uint8_t*>(dest);

        while (total < count) {
            if (timeout_ms >= 0) {
                struct pollfd pfd{};
                pfd.fd = fd_;
                pfd.events = POLLIN;

                int ret = ::poll(&pfd, 1, timeout_ms);
                if (ret == 0) {
                    return false;
                }
                if (ret < 0) {
                    if (errno == EINTR) continue;
                    return false;
                }
            }

            ssize_t n = ::read(fd_, ptr + total, count - total);
            if (n < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (n == 0) {
                return false;
            }
            total += static_cast<size_t>(n);
        }
        return true;
    }

    void FifoReader::close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
        if (!path_.empty()) {
            ::unlink(path_.c_str());
            path_.clear();
        }
    }

    FifoWriter::FifoWriter(int fd, std::string path)
        : fd_(fd), path_(std::move(path)) {}

    FifoWriter::~FifoWriter() {
        close();
    }

    FifoWriter::FifoWriter(FifoWriter&& other) noexcept
        : fd_(other.fd_), path_(std::move(other.path_)) {
        other.fd_ = -1;
        other.path_.clear();
    }

    FifoWriter& FifoWriter::operator=(FifoWriter&& other) noexcept {
        if (this != &other) {
            close();
            fd_ = other.fd_;
            path_ = std::move(other.path_);
            other.fd_ = -1;
            other.path_.clear();
        }
        return *this;
    }

    FifoWriter FifoWriter::open(const std::string& path, std::chrono::milliseconds timeout) {
        auto start = std::chrono::steady_clock::now();

        while (true) {
            int fd = ::open(path.c_str(), O_WRONLY | O_NONBLOCK);
            if (fd >= 0) {
                int flags = ::fcntl(fd, F_GETFL, 0);
                ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
                return FifoWriter(fd, path);
            }

            if (errno != ENXIO && errno != ENOENT) {
                return FifoWriter(-1, "");
            }

            if (std::chrono::steady_clock::now() - start >= timeout) {
                return FifoWriter(-1, "");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    bool FifoWriter::write_exact(const void* src, size_t count) {
        if (fd_ < 0) return false;
        size_t total = 0;
        const auto* ptr = static_cast<const uint8_t*>(src);

        while (total < count) {
            ssize_t n = ::write(fd_, ptr + total, count - total);
            if (n < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            total += static_cast<size_t>(n);
        }
        return true;
    }

    void FifoWriter::close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
        path_.clear();
    }

}
