#pragma once

#include <netinet/ip.h>
#include <netinet/udp.h>

namespace trade::utilities
{

class UdpPayLoadGetter
{
public:
    std::tuple<const u_char*, size_t> operator()(const u_char* packet) const
    {
        const auto ip_header = reinterpret_cast<const ip*>(packet + 14); // 14 bytes for Ethernet header.

        /// If not a UDP packet.
        if (ip_header->ip_p != IPPROTO_UDP) [[unlikely]]
            return std::make_tuple(nullptr, 0);

        const auto udp_header  = reinterpret_cast<const udphdr*>(packet + 14 + ip_header->ip_hl * 4);

        const auto udp_payload = packet + 14 + ip_header->ip_hl * 4 + sizeof(udphdr);
        const auto length      = ntohs(udp_header->uh_ulen) - sizeof(udphdr);

        return std::make_tuple(udp_payload, length);
    }
};

} // namespace trade::utilities
