#include <catch.hpp>
#include <thread>

#include "utilities/AddressHelper.hpp"

TEST_CASE("Basic IPv4 address parsing", "[AddressHelper]")
{
    const std::string ipv4_address = "127.0.0.1:2233";

    const auto [address, port]     = trade::utilities::AddressHelper::extract_address(ipv4_address);

    CHECK(address == "127.0.0.1");
    CHECK(port == 2233);
}

TEST_CASE("Wrong format IPv4 address parsing", "[AddressHelper]")
{
    const std::string ipv4_address = "127.0.0.1";

    const auto [address, port]     = trade::utilities::AddressHelper::extract_address(ipv4_address);

    CHECK(address.empty());
    CHECK(port == 0);
}
