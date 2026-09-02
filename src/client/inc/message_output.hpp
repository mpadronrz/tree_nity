#ifndef CLIENT_MESSAGE_OUTPUT_HPP
#define CLIENT_MESSAGE_OUTPUT_HPP

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace client {

struct Message {
    std::string key;
    std::string value;
};

std::string format_text_message(const Message& message);

bool write_raw_message(std::ostream& output, std::uint32_t offset, const Message& message);
void write_output(const std::vector<unsigned char>& payload);
void write_error(const std::vector<unsigned char>& payload);

}

#endif
