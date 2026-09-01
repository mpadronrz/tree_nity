#ifndef CLIENT_REQUEST_BUILDER_HPP
#define CLIENT_REQUEST_BUILDER_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace client {

// Asigna los códigos acordados a las operaciones recibidas por el FIFO principal del servidor.
enum class RequestAction : std::uint8_t {
    CreateTopic = 1,
    ListTopics = 2,
    ClientInfo = 3,
    ProducerConnect = 4,
    Publish = 5,
    SubscriberConnect = 6,
    SubscriberCommit = 7,
    Disconnect = 8
};

// Construye un request sin payload, usado inicialmente por la operación list.
bool build_empty_request(RequestAction action, std::uint32_t client_pid,
    std::vector<unsigned char>& frame);

// Construye un request cuyo payload es [longitud][cadena], usado por create e info.
bool build_single_string_request(RequestAction action, std::uint32_t client_pid,
    const std::string& value, std::vector<unsigned char>& frame);

// Construye un request con dos campos de cadena, usado por Publish para key y value.
bool build_two_string_request(RequestAction action, std::uint32_t client_pid,
    const std::string& first, const std::string& second, std::vector<unsigned char>& frame);

} // namespace client

#endif
