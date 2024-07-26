#include <catch.hpp>
#include <thread>

#include "utilities/UdpPayloadGetter.hpp"

TEST_CASE("Udp payload parsing", "[UdpPayloadGetter]")
{
    const auto packet = reinterpret_cast<const u_char*>(
        "\x00\x1a\xa0\xbb\xcc\xdd\x00\x1a\xa0\xbb\xcc\xee\x08\x00" /// Ethernet header.
        "\x45\x00\x00\x3c\x1c\x46\x40\x00\x40\x11\xb7\xc0\xc0\xa8\x00\x68"
        "\xc0\xa8\x00\x01\x00\x35\x00\x35\x00\x23\x27\x10\x61\x62\x63\x64"
        "\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74"
        "\x75\x76\x77\x78\x79\x7a\x00" /// Payload: "abcdefghijklmnopqrstuvwxyz\0".
    );

    SECTION("Parsing for payload and length with robust version")
    {
        const auto [payload, udp_payload_length] = trade::utilities::UdpPayloadGetter()(packet);

        CHECK(payload != nullptr);
        CHECK(std::strcmp(reinterpret_cast<const char*>(payload), "abcdefghijklmnopqrstuvwxyz") == 0);
        CHECK(udp_payload_length == 27);
    }

    SECTION("Parsing for payload and length with faster version")
    {
        size_t udp_payload_length;

        const auto payload = trade::utilities::UdpPayloadGetter()(packet, 69, udp_payload_length);

        CHECK(payload != nullptr);
        CHECK(std::strcmp(reinterpret_cast<const char*>(payload), "abcdefghijklmnopqrstuvwxyz") == 0);
        CHECK(udp_payload_length == 27);
    }
}
