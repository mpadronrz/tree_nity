#include "request_builder.hpp"

#include "protocol_frame.hpp"
#include "protocol_payload.hpp"

namespace client {

bool build_empty_request(RequestAction action, std::uint32_t client_pid,
    std::vector<unsigned char>& frame) {
    const std::vector<unsigned char> payload;
    return build_request_frame(static_cast<std::uint8_t>(action), client_pid, payload, frame);
}

bool build_single_string_request(RequestAction action, std::uint32_t client_pid,
    const std::string& value, std::vector<unsigned char>& frame) {
    std::vector<unsigned char> payload;
    if (!encode_single_string_payload(value, payload))
        return false;
    return build_request_frame(static_cast<std::uint8_t>(action), client_pid, payload, frame);
}

bool build_two_string_request(RequestAction action, std::uint32_t client_pid,
    const std::string& first, const std::string& second, std::vector<unsigned char>& frame) {
    std::vector<unsigned char> payload;
    if (!encode_two_string_payload(first, second, payload))
        return false;
    return build_request_frame(static_cast<std::uint8_t>(action), client_pid, payload, frame);
}

} // namespace client
