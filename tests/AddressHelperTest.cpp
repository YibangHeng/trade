#include <catch.hpp>
#include <thread>

#include "utilities/AddressHelper.hpp"

TEST_CASE("IPv4 address parsing", "[AddressHelper]")
{
    SECTION("Basic IPv4 address parsing")
    {
        const std::string ipv4_address = "127.0.0.1:2233";

        /// Extract multicast address and port from address string.
        const auto [address, port] = trade::utilities::AddressHelper::extract_address(ipv4_address);

        CHECK(address == "127.0.0.1");
        CHECK(port == 2233);
    }

    SECTION("Wrong format IPv4 address parsing")
    {
        const std::string ipv4_address = "127.0.0.1";

        /// Extract multicast address and port from address string.
        const auto [address, port] = trade::utilities::AddressHelper::extract_address(ipv4_address);

        CHECK(address.empty());
        CHECK(port == 0);
    }
}
