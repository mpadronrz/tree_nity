#include <string>
#include <cstdint>

struct Message
{
	uint32_t	offset;
	std::string	key;
	std::string	value;
};
