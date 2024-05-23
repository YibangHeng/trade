#pragma once

namespace trade::broker
{

/// For using memcpy().
#pragma pack(push, 1)

struct PriceQuantityPair {
    uint32_t m_price;
    uint64_t m_qty;
};

struct SZSEHpfPackageHead {
    uint32_t m_sequence;
    uint16_t m_tick1;
    uint16_t m_tick2;
    uint8_t m_message_type;
    uint8_t m_security_type;
    uint8_t m_sub_security_type;
    char m_symbol[9];
    uint8_t m_exchange_id;
    uint64_t m_quote_update_time;
    uint16_t m_channel_num;
    int64_t m_sequence_num;
    int32_t m_md_stream_id;
};

struct SZSEHpfOrderTick {
    SZSEHpfPackageHead m_header;
    uint32_t m_px;
    uint64_t m_qty;
    char m_side;
    char m_order_type;
    char m_reserved[7];
};

struct SZSEHpfTradeTick {
    SZSEHpfPackageHead m_header;
    int64_t m_bid_app_seq_num;
    int64_t m_ask_app_seq_num;
    uint32_t m_exe_px;
    uint64_t m_exe_qty;
    char m_exe_type;
};

#pragma pack(pop)

} // namespace trade::broker