#pragma once

#include <regex>

namespace trade::utilities
{

class AddressHelper
{
public:
    /// Extract multicast address and port from address string.
    /// @param address The IPv4 address string in format %d.%d.%d.%d:%d.
    /// @return A tuple of (address, port).
    static std::tuple<std::string, uint16_t> extract_address(const std::string& address)
    {
        const std::regex re(R"(^(\d{1,3}(?:\.\d{1,3}){3}):(\d+)$)");
        std::smatch address_match;

        std::regex_search(address, address_match, re);

        if (address_match.size() == 3) {
            return std::make_tuple(std::string(address_match[1]), static_cast<uint16_t>(std::stoi(address_match[2])));
        }

        return std::make_tuple(std::string {}, static_cast<uint16_t>(0));
    }
};

} // namespace trade::utilities