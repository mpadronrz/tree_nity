#include "protocol_io.hpp"

#include "protocol_frame.hpp"
#include "protocol/Protocol.hpp"

namespace client {
namespace {

// Traduce el resultado genérico de read_exact al vocabulario de la capa de protocolo.
ResponseReadStatus status_from_fifo_result(FifoReadStatus status) {
    if (status == FifoReadStatus::EndOfFile)
        return ResponseReadStatus::EndOfFile;
    if (status == FifoReadStatus::PartialEndOfFile)
        return ResponseReadStatus::PartialEndOfFile;
    return ResponseReadStatus::Error;
}

} // namespace

bool write_request_frame(int file_descriptor, const std::vector<unsigned char>& frame, int& error_number) {
    if (frame.size() < kRequestHeaderSize) {
        error_number = 0;
        return false;
    }
    return write_all(file_descriptor, frame.data(), frame.size(), error_number);
}

ResponseReadResult read_response_frame(int file_descriptor, ResponseFrame& response) {
    unsigned char header_bytes[kResponseHeaderSize] = {};
    ResponseReadResult result = {};
    result.status = ResponseReadStatus::Complete;
    result.error_number = 0;
    response.code = static_cast<Protocol::StatusCode>(0);
    response.payload.clear();

    const FifoReadResult header_read = read_exact(file_descriptor, header_bytes, kResponseHeaderSize);
    if (header_read.status != FifoReadStatus::Complete) {
        result.status = status_from_fifo_result(header_read.status);
        result.error_number = header_read.error_number;
        return result;
    }

    ResponseHeader header = {};
    if (!decode_response_header(header_bytes, kResponseHeaderSize, header)) {
        result.status = ResponseReadStatus::InvalidFrame;
        return result;
    }

    const std::size_t payload_size = static_cast<std::size_t>(header.total_length);
    response.code = static_cast<Protocol::StatusCode>(header.code);
    response.payload.resize(payload_size);
    if (payload_size == 0U)
        return result;

    const FifoReadResult payload_read = read_exact(file_descriptor, response.payload.data(), payload_size);
    if (payload_read.status != FifoReadStatus::Complete) {
        response.payload.clear();
        result.status = status_from_fifo_result(payload_read.status);
        result.error_number = payload_read.error_number;
    }
    return result;
}

} // namespace client
