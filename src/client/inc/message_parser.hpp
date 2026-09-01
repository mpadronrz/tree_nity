#ifndef CLIENT_MESSAGE_PARSER_HPP
#define CLIENT_MESSAGE_PARSER_HPP

#include <istream>
#include <string>
#include <vector>

namespace client {

// Representa el contenido que el productor enviará después al servidor.
struct Message {
    std::string key;
    std::string value;
};

// Devuelve todos los registros leídos o el primer error que impide producirlos.
struct MessageParseResult {
    bool ok;
    std::vector<Message> messages;
    std::string error;
};

// Lee registros de stdin escritos como key:body, uno por línea.
MessageParseResult parse_text_messages(std::istream& input);

// Lee registros binarios little-endian sin separadores entre ellos.
MessageParseResult parse_raw_messages(std::istream& input);

} // namespace client

#endif
