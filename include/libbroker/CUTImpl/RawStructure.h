#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "visibility.h"

/// Also check ./SZSEHpfTick.lua.

namespace trade::broker
{

union EndianTest
{
    uint32_t i;
    char c[4];
};

static_assert(std::endian::native == std::endian::little, "Endian check failed. The program must be compiled under little-endian system");

/// For using memcpy().
#pragma pack(push, 1)

struct MessageTypeViewer {
    uint8_t m_reserved[8];
    uint8_t m_message_type;
};

static_assert(offsetof(MessageTypeViewer, m_message_type) == 8);
static_assert(sizeof(MessageTypeViewer) == 9, "MessageTypeViewer should be 9 bytes");

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

/// SSE has deprecated this struct.
struct [[deprecated]] PUBLIC_API SSEHpfOrderTick {
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

/// SSE has deprecated this struct.
struct [[deprecated]] PUBLIC_API SSEHpfTradeTick {
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

struct PUBLIC_API SSEHpfTick {
    SSEHpfPackageHead m_head;
    uint32_t m_tick_index;
    uint32_t m_channel_id;
    char m_symbol_id[9];
    uint8_t m_secu_type;
    uint8_t m_sub_secu_type;
    uint32_t m_tick_time;
    char m_tick_type;
    uint64_t m_buy_order_no;
    uint64_t m_sell_order_no;
    uint32_t m_order_price;
    uint64_t m_qty;
    uint64_t m_trade_money;
    char m_side_flag;
    uint8_t m_instrument_status;
    char m_reserved[8];
};

static_assert(offsetof(SSEHpfTick, m_head) == 0);
static_assert(offsetof(SSEHpfTick, m_tick_index) == 26);
static_assert(offsetof(SSEHpfTick, m_channel_id) == 30);
static_assert(offsetof(SSEHpfTick, m_symbol_id) == 34);
static_assert(offsetof(SSEHpfTick, m_secu_type) == 43);
static_assert(offsetof(SSEHpfTick, m_sub_secu_type) == 44);
static_assert(offsetof(SSEHpfTick, m_tick_time) == 45);
static_assert(offsetof(SSEHpfTick, m_tick_type) == 49);
static_assert(offsetof(SSEHpfTick, m_buy_order_no) == 50);
static_assert(offsetof(SSEHpfTick, m_sell_order_no) == 58);
static_assert(offsetof(SSEHpfTick, m_order_price) == 66);
static_assert(offsetof(SSEHpfTick, m_qty) == 70);
static_assert(offsetof(SSEHpfTick, m_trade_money) == 78);
static_assert(offsetof(SSEHpfTick, m_side_flag) == 86);
static_assert(offsetof(SSEHpfTick, m_instrument_status) == 87);
static_assert(offsetof(SSEHpfTick, m_reserved) == 88);
static_assert(sizeof(SSEHpfTick) == 96, "SSEHpfTick should be 96 bytes");

struct SSEPriceQuantityPair {
    uint32_t resv;
    uint32_t m_px;
    uint64_t m_qty;
};

static_assert(offsetof(SSEPriceQuantityPair, resv) == 0);
static_assert(offsetof(SSEPriceQuantityPair, m_px) == 4);
static_assert(offsetof(SSEPriceQuantityPair, m_qty) == 8);
static_assert(sizeof(SSEPriceQuantityPair) == 16, "SSEPriceQuantityPair should be 16 bytes");

struct PUBLIC_API SSEHpfL2Snap {
    SSEHpfPackageHead m_head;
    uint32_t m_update_time;
    char m_symbol_id[9];
    uint8_t m_secu_type;
    uint8_t m_update_type;
    uint8_t m_reserved;
    uint32_t m_prev_close;
    uint32_t m_open_price;
    uint32_t m_day_high;
    uint32_t m_day_low;
    uint32_t m_last_price;
    uint32_t m_close_price;
    uint8_t m_instrument_status;
    uint8_t m_trading_status;
    uint16_t m_reserved2;
    uint32_t m_trade_number;
    uint64_t m_trade_volume;
    uint64_t m_trade_value;
    uint64_t m_total_qty_bid;
    uint32_t m_weighted_avg_px_bid;
    uint64_t m_total_qty_ask;
    uint32_t m_weighted_avg_px_ask;
    uint32_t m_yield_to_maturity;
    uint8_t m_depth_bid;
    uint8_t m_depth_ask;
    SSEPriceQuantityPair m_bid_px[10];
    SSEPriceQuantityPair m_ask_px[10];
};

static_assert(offsetof(SSEHpfL2Snap, m_head) == 0);
static_assert(offsetof(SSEHpfL2Snap, m_update_time) == 26);
static_assert(offsetof(SSEHpfL2Snap, m_symbol_id) == 30);
static_assert(offsetof(SSEHpfL2Snap, m_secu_type) == 39);
static_assert(offsetof(SSEHpfL2Snap, m_update_type) == 40);
static_assert(offsetof(SSEHpfL2Snap, m_reserved) == 41);
static_assert(offsetof(SSEHpfL2Snap, m_prev_close) == 42);
static_assert(offsetof(SSEHpfL2Snap, m_open_price) == 46);
static_assert(offsetof(SSEHpfL2Snap, m_day_high) == 50);
static_assert(offsetof(SSEHpfL2Snap, m_day_low) == 54);
static_assert(offsetof(SSEHpfL2Snap, m_last_price) == 58);
static_assert(offsetof(SSEHpfL2Snap, m_close_price) == 62);
static_assert(offsetof(SSEHpfL2Snap, m_instrument_status) == 66);
static_assert(offsetof(SSEHpfL2Snap, m_trading_status) == 67);
static_assert(offsetof(SSEHpfL2Snap, m_reserved2) == 68);
static_assert(offsetof(SSEHpfL2Snap, m_trade_number) == 70);
static_assert(offsetof(SSEHpfL2Snap, m_trade_volume) == 74);
static_assert(offsetof(SSEHpfL2Snap, m_trade_value) == 82);
static_assert(offsetof(SSEHpfL2Snap, m_total_qty_bid) == 90);
static_assert(offsetof(SSEHpfL2Snap, m_weighted_avg_px_bid) == 98);
static_assert(offsetof(SSEHpfL2Snap, m_total_qty_ask) == 102);
static_assert(offsetof(SSEHpfL2Snap, m_weighted_avg_px_ask) == 110);
static_assert(offsetof(SSEHpfL2Snap, m_yield_to_maturity) == 114);
static_assert(offsetof(SSEHpfL2Snap, m_depth_bid) == 118);
static_assert(offsetof(SSEHpfL2Snap, m_depth_ask) == 119);
static_assert(offsetof(SSEHpfL2Snap, m_bid_px) == 120);
static_assert(offsetof(SSEHpfL2Snap, m_ask_px) == 280);
static_assert(sizeof(SSEHpfL2Snap) == 440, "SSEHpfL2Snap should be 440 bytes");

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

struct SZSEPriceQuantityPair {
    uint32_t m_price;
    uint64_t m_qty;
};

static_assert(offsetof(SZSEPriceQuantityPair, m_price) == 0);
static_assert(offsetof(SZSEPriceQuantityPair, m_qty) == 4);
static_assert(sizeof(SZSEPriceQuantityPair) == 12);

struct PUBLIC_API SZSEHpfL2Snap {
    SZSEHpfPackageHead m_header;
    uint8_t m_trading_phase_code;
    int64_t m_trades_num;
    uint64_t m_total_quantity_trade;
    uint64_t m_total_value_trade;
    uint32_t m_previous_close_price;
    uint32_t m_last_price;
    uint32_t m_open_price;
    uint32_t m_day_high;
    uint32_t m_day_low;
    uint32_t m_today_close_price;
    uint32_t m_total_bid_weighted_avg_px;
    uint64_t m_total_bid_qty;
    uint32_t m_total_ask_weighted_avg_px;
    uint64_t m_total_ask_qty;
    uint32_t m_lpv;
    uint32_t m_iopv;
    uint32_t m_upper_limit_price;
    uint32_t m_lower_limit_price;
    uint32_t m_open_interest;
    SZSEPriceQuantityPair m_bid_unit[10];
    SZSEPriceQuantityPair m_ask_unit[10];
};

static_assert(offsetof(SZSEHpfL2Snap, m_header) == 0);
static_assert(offsetof(SZSEHpfL2Snap, m_trading_phase_code) == 43);
static_assert(offsetof(SZSEHpfL2Snap, m_trades_num) == 44);
static_assert(offsetof(SZSEHpfL2Snap, m_total_quantity_trade) == 52);
static_assert(offsetof(SZSEHpfL2Snap, m_total_value_trade) == 60);
static_assert(offsetof(SZSEHpfL2Snap, m_previous_close_price) == 68);
static_assert(offsetof(SZSEHpfL2Snap, m_last_price) == 72);
static_assert(offsetof(SZSEHpfL2Snap, m_open_price) == 76);
static_assert(offsetof(SZSEHpfL2Snap, m_day_high) == 80);
static_assert(offsetof(SZSEHpfL2Snap, m_day_low) == 84);
static_assert(offsetof(SZSEHpfL2Snap, m_today_close_price) == 88);
static_assert(offsetof(SZSEHpfL2Snap, m_total_bid_weighted_avg_px) == 92);
static_assert(offsetof(SZSEHpfL2Snap, m_total_bid_qty) == 96);
static_assert(offsetof(SZSEHpfL2Snap, m_total_ask_weighted_avg_px) == 104);
static_assert(offsetof(SZSEHpfL2Snap, m_total_ask_qty) == 108);
static_assert(offsetof(SZSEHpfL2Snap, m_lpv) == 116);
static_assert(offsetof(SZSEHpfL2Snap, m_iopv) == 120);
static_assert(offsetof(SZSEHpfL2Snap, m_upper_limit_price) == 124);
static_assert(offsetof(SZSEHpfL2Snap, m_lower_limit_price) == 128);
static_assert(offsetof(SZSEHpfL2Snap, m_open_interest) == 132);
static_assert(offsetof(SZSEHpfL2Snap, m_bid_unit) == 136);
static_assert(offsetof(SZSEHpfL2Snap, m_ask_unit) == 256);
static_assert(sizeof(SZSEHpfL2Snap) == 376, "SZSEL2Snap should be 376 bytes");

/// See 东方证券极速行情输出协议.
constexpr uint8_t sse_hpf_tick_type        = 36;
constexpr uint8_t sse_hpf_l2_snap_type     = 39;
constexpr uint8_t szse_hpf_order_tick_type = 23;
constexpr uint8_t szse_hpf_trade_tick_type = 24;
constexpr uint8_t szse_hpf_l2_snap_type    = 21;

constexpr size_t message_type_viewer_size  = sizeof(MessageTypeViewer);

constexpr size_t sse_hpf_tick_size         = sizeof(SSEHpfTick);
constexpr size_t sse_hpf_l2_snap_size      = sizeof(SSEHpfL2Snap);

constexpr size_t szse_hpf_order_tick_size  = sizeof(SZSEHpfOrderTick);
constexpr size_t szse_hpf_trade_tick_size  = sizeof(SZSEHpfTradeTick);
constexpr size_t szse_hpf_l2_snap_size     = sizeof(SZSEHpfL2Snap);

constexpr size_t max_sse_udp_size          = std::max({sse_hpf_tick_size, sse_hpf_l2_snap_size});
constexpr size_t max_szse_udp_size         = std::max({szse_hpf_order_tick_size, szse_hpf_trade_tick_size, szse_hpf_l2_snap_size});
constexpr size_t max_udp_size              = std::max(max_sse_udp_size, max_szse_udp_size);

#pragma pack(pop)

} // namespace trade::broker
