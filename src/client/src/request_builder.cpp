#include "request_builder.hpp"

#include "protocol/Protocol.hpp"
#include "protocol_frame.hpp"
#include "protocol_payload.hpp"

namespace client {

bool build_empty_request(RequestAction action, std::uint32_t client_pid,
    std::vector<unsigned char>& frame) {
    if (action == RequestAction::ListTopics) {
        frame = Protocol::Request::serialize_list(client_pid);
        return true;
    }
    const std::vector<unsigned char> payload;
    return build_request_frame(static_cast<std::uint8_t>(action), client_pid, payload, frame);
}

bool build_single_string_request(RequestAction action, std::uint32_t client_pid,
    const std::string& value, std::vector<unsigned char>& frame) {
    if (action == RequestAction::CreateTopic) {
        frame = Protocol::Request::serialize_create(client_pid, value);
        return true;
    }
    if (action == RequestAction::ClientInfo) {
        frame = Protocol::Request::serialize_info(client_pid, value);
        return true;
    }
    if (action == RequestAction::ProducerConnect) {
        frame = Protocol::Request::serialize_connect(client_pid, value);
        return true;
    }
    std::vector<unsigned char> payload;
    if (!encode_single_string_payload(value, payload))
        return false;
    return build_request_frame(static_cast<std::uint8_t>(action), client_pid, payload, frame);
}

bool build_two_string_request(RequestAction action, std::uint32_t client_pid,
    const std::string& first, const std::string& second, std::vector<unsigned char>& frame) {
    if (action == RequestAction::Publish) {
        frame = Protocol::Request::serialize_produce("", first, second);
        return true;
    }
    std::vector<unsigned char> payload;
    if (!encode_two_string_payload(first, second, payload))
        return false;
    return build_request_frame(static_cast<std::uint8_t>(action), client_pid, payload, frame);
}

bool build_publish_request(const std::string& topic, const std::string& key,
    const std::string& value, std::vector<unsigned char>& frame) {
    frame = Protocol::Request::serialize_produce(topic, key, value);
    return true;
}

bool build_subscriber_request(std::uint32_t client_pid, const std::string& topic,
    const std::string& subscriber, const std::string& prefix, bool has_offset,
    std::uint32_t offset, std::vector<unsigned char>& frame) {
    std::uint32_t requested_offset = has_offset ? offset : Protocol::OFFSET_UNSET;
    frame = Protocol::Request::serialize_subscribe(client_pid, subscriber, topic, prefix, requested_offset);
    return true;
}

bool build_commit_request(std::uint32_t client_pid, const std::string& subscriber,
    std::uint32_t offset, std::vector<unsigned char>& frame) {
    (void)client_pid;
    frame = Protocol::Request::serialize_ack(subscriber, offset);
    return true;
}

bool build_disconnect_request(std::uint32_t client_pid, const std::string& subscriber,
    std::vector<unsigned char>& frame) {
    (void)client_pid;
    frame = Protocol::Request::serialize_disconnect(subscriber);
    return true;
}

} // namespace client
