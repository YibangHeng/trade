#pragma once

#include "visibility.h"

namespace trade::broker
{

/// For using memcpy().
#pragma pack(push, 1)

/// All structures below come from OST example code without any modification.

struct SSEHpfPackageHead {
    uint32_t m_seq_num;
    uint32_t m_reserved;
    uint8_t m_msg_type;
    uint16_t m_msg_len;
    uint8_t m_exchange_id;
    uint16_t m_data_year;
    uint8_t m_data_month;
    uint8_t m_data_day;
    uint32_t m_send_time;
    uint8_t m_category_id;
    uint32_t m_msg_seq_id;
    uint8_t m_seq_lost_flag;
};

static_assert(sizeof(SSEHpfPackageHead) == 26, "SSEHpfPackageHead should be 26 bytes");

struct PUBLIC_API SSEHpfOrderTick {
    SSEHpfPackageHead m_head;
    uint32_t m_order_index;
    uint32_t m_channel_id;
    char m_symbol_id[9];
    uint32_t m_order_time;
    char m_order_type;
    uint64_t m_order_no;
    uint32_t m_order_price;
    uint64_t m_balance;
    uint8_t m_reserved1[15];
    char m_side_flag;
    uint64_t m_biz_index;
    uint32_t m_reserved2;
};

static_assert(sizeof(SSEHpfOrderTick) == 96, "SSEHpfOrderTick should be 96 bytes");

struct PUBLIC_API SSEHpfTradeTick {
    SSEHpfPackageHead m_head;
    uint32_t m_trade_seq_num;
    uint32_t m_channel_id;
    char m_symbol_id[9];
    uint32_t m_trade_time;
    uint32_t m_trade_price;
    uint64_t m_trade_volume;
    uint64_t m_trade_value;
    uint64_t m_seq_num_bid;
    uint64_t m_seq_num_ask;
    char m_side_flag;
    uint64_t m_biz_index;
    uint32_t m_reserved;
};

static_assert(sizeof(SSEHpfTradeTick) == 96, "SSEHpfTradeTick should be 96 bytes");

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

static_assert(sizeof(SZSEHpfPackageHead) == 43, "SZSEHpfPackageHead should be 43 bytes");

struct PUBLIC_API SZSEHpfOrderTick {
    SZSEHpfPackageHead m_header;
    uint32_t m_px;
    uint64_t m_qty;
    char m_side;
    char m_order_type;
    char m_reserved[7];
};

static_assert(sizeof(SZSEHpfOrderTick) == 64, "SZSEHpfOrderTick should be 64 bytes");

struct PUBLIC_API SZSEHpfTradeTick {
    SZSEHpfPackageHead m_header;
    int64_t m_bid_app_seq_num;
    int64_t m_ask_app_seq_num;
    uint32_t m_exe_px;
    uint64_t m_exe_qty;
    char m_exe_type;
};

static_assert(sizeof(SZSEHpfTradeTick) == 72, "SZSEHpfTradeTick should be 72 bytes");

constexpr size_t max_sse_udp_size  = std::max(sizeof(SSEHpfOrderTick), sizeof(SSEHpfTradeTick));
constexpr size_t max_szse_udp_size = std::max(sizeof(SZSEHpfOrderTick), sizeof(SZSEHpfTradeTick));

#pragma pack(pop)

} // namespace trade::broker
