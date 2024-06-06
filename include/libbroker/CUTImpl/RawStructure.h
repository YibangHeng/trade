#pragma once

#include "visibility.h"

/// Also check ./SZSEHpfTick.lua.

namespace trade::broker
{

union EndianTest
{
    uint32_t i;
    char c[4];
};

constexpr EndianTest endianTest = {0x01020304};
static_assert(
    endianTest.c[0] == 0x04 && endianTest.c[1] == 0x03 && endianTest.c[2] == 0x02 && endianTest.c[3] == 0x01,
    "Endian check failed. The program must be compiled under little-endian system"
);

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

static_assert(offsetof(SSEHpfPackageHead, m_seq_num) == 0);
static_assert(offsetof(SSEHpfPackageHead, m_reserved) == 4);
static_assert(offsetof(SSEHpfPackageHead, m_msg_type) == 8);
static_assert(offsetof(SSEHpfPackageHead, m_msg_len) == 9);
static_assert(offsetof(SSEHpfPackageHead, m_exchange_id) == 11);
static_assert(offsetof(SSEHpfPackageHead, m_data_year) == 12);
static_assert(offsetof(SSEHpfPackageHead, m_data_month) == 14);
static_assert(offsetof(SSEHpfPackageHead, m_data_day) == 15);
static_assert(offsetof(SSEHpfPackageHead, m_send_time) == 16);
static_assert(offsetof(SSEHpfPackageHead, m_category_id) == 20);
static_assert(offsetof(SSEHpfPackageHead, m_msg_seq_id) == 21);
static_assert(offsetof(SSEHpfPackageHead, m_seq_lost_flag) == 25);
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

static_assert(offsetof(SSEHpfOrderTick, m_head) == 0);
static_assert(offsetof(SSEHpfOrderTick, m_order_index) == 26);
static_assert(offsetof(SSEHpfOrderTick, m_channel_id) == 30);
static_assert(offsetof(SSEHpfOrderTick, m_symbol_id) == 34);
static_assert(offsetof(SSEHpfOrderTick, m_order_time) == 43);
static_assert(offsetof(SSEHpfOrderTick, m_order_type) == 47);
static_assert(offsetof(SSEHpfOrderTick, m_order_no) == 48);
static_assert(offsetof(SSEHpfOrderTick, m_order_price) == 56);
static_assert(offsetof(SSEHpfOrderTick, m_balance) == 60);
static_assert(offsetof(SSEHpfOrderTick, m_reserved1) == 68);
static_assert(offsetof(SSEHpfOrderTick, m_side_flag) == 83);
static_assert(offsetof(SSEHpfOrderTick, m_biz_index) == 84);
static_assert(offsetof(SSEHpfOrderTick, m_reserved2) == 92);
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

static_assert(offsetof(SSEHpfTradeTick, m_head) == 0);
static_assert(offsetof(SSEHpfTradeTick, m_trade_seq_num) == 26);
static_assert(offsetof(SSEHpfTradeTick, m_channel_id) == 30);
static_assert(offsetof(SSEHpfTradeTick, m_symbol_id) == 34);
static_assert(offsetof(SSEHpfTradeTick, m_trade_time) == 43);
static_assert(offsetof(SSEHpfTradeTick, m_trade_price) == 47);
static_assert(offsetof(SSEHpfTradeTick, m_trade_volume) == 51);
static_assert(offsetof(SSEHpfTradeTick, m_trade_value) == 59);
static_assert(offsetof(SSEHpfTradeTick, m_seq_num_bid) == 67);
static_assert(offsetof(SSEHpfTradeTick, m_seq_num_ask) == 75);
static_assert(offsetof(SSEHpfTradeTick, m_side_flag) == 83);
static_assert(offsetof(SSEHpfTradeTick, m_biz_index) == 84);
static_assert(offsetof(SSEHpfTradeTick, m_reserved) == 92);
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

static_assert(offsetof(SZSEHpfPackageHead, m_sequence) == 0);
static_assert(offsetof(SZSEHpfPackageHead, m_tick1) == 4);
static_assert(offsetof(SZSEHpfPackageHead, m_tick2) == 6);
static_assert(offsetof(SZSEHpfPackageHead, m_message_type) == 8);
static_assert(offsetof(SZSEHpfPackageHead, m_security_type) == 9);
static_assert(offsetof(SZSEHpfPackageHead, m_sub_security_type) == 10);
static_assert(offsetof(SZSEHpfPackageHead, m_symbol) == 11);
static_assert(offsetof(SZSEHpfPackageHead, m_exchange_id) == 20);
static_assert(offsetof(SZSEHpfPackageHead, m_quote_update_time) == 21);
static_assert(offsetof(SZSEHpfPackageHead, m_channel_num) == 29);
static_assert(offsetof(SZSEHpfPackageHead, m_sequence_num) == 31);
static_assert(offsetof(SZSEHpfPackageHead, m_md_stream_id) == 39);
static_assert(sizeof(SZSEHpfPackageHead) == 43, "SZSEHpfPackageHead should be 43 bytes");

struct PUBLIC_API SZSEHpfOrderTick {
    SZSEHpfPackageHead m_header;
    uint32_t m_px;
    uint64_t m_qty;
    char m_side;
    char m_order_type;
    char m_reserved[7];
};

static_assert(offsetof(SZSEHpfOrderTick, m_header) == 0);
static_assert(offsetof(SZSEHpfOrderTick, m_px) == 43);
static_assert(offsetof(SZSEHpfOrderTick, m_qty) == 47);
static_assert(offsetof(SZSEHpfOrderTick, m_side) == 55);
static_assert(offsetof(SZSEHpfOrderTick, m_order_type) == 56);
static_assert(offsetof(SZSEHpfOrderTick, m_reserved) == 57);
static_assert(sizeof(SZSEHpfOrderTick) == 64, "SZSEHpfOrderTick should be 64 bytes");

struct PUBLIC_API SZSEHpfTradeTick {
    SZSEHpfPackageHead m_header;
    int64_t m_bid_app_seq_num;
    int64_t m_ask_app_seq_num;
    uint32_t m_exe_px;
    uint64_t m_exe_qty;
    char m_exe_type;
};

static_assert(offsetof(SZSEHpfTradeTick, m_header) == 0);
static_assert(offsetof(SZSEHpfTradeTick, m_bid_app_seq_num) == 43);
static_assert(offsetof(SZSEHpfTradeTick, m_ask_app_seq_num) == 51);
static_assert(offsetof(SZSEHpfTradeTick, m_exe_px) == 59);
static_assert(offsetof(SZSEHpfTradeTick, m_exe_qty) == 63);
static_assert(offsetof(SZSEHpfTradeTick, m_exe_type) == 71);
static_assert(sizeof(SZSEHpfTradeTick) == 72, "SZSEHpfTradeTick should be 72 bytes");

constexpr size_t max_sse_udp_size  = std::max(sizeof(SSEHpfOrderTick), sizeof(SSEHpfTradeTick));
constexpr size_t max_szse_udp_size = std::max(sizeof(SZSEHpfOrderTick), sizeof(SZSEHpfTradeTick));
constexpr size_t max_udp_size      = std::max(max_sse_udp_size, max_szse_udp_size);

#pragma pack(pop)

} // namespace trade::broker
