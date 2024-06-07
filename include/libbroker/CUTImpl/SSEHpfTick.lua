-- Wireshark Lua Plugin for SSE Hpf Tick Protocol.

-- Usage:
--   Copy this file into your wireshark plugins directory and
--   Analyze -> Reload Lua Plugins (Ctrl-Shift-L).

local p_SSEHpfTick = Proto("SSEHpfTick", "SSE Hpf Tick Protocol")

local f = p_SSEHpfTick.fields

-- SSEHpfPackageHead.
f.seq_num = ProtoField.uint32("SSEHpfTick.seq_num", "Seq Num", ftypes.uint32)
f.reserved_1 = ProtoField.uint32("SSEHpfTick.reserved", "Reserved", ftypes.uint32)
f.msg_type = ProtoField.uint8("SSEHpfTick.msg_type", "Msg Type", ftypes.uint8)
f.msg_len = ProtoField.uint16("SSEHpfTick.msg_len", "Msg Len", ftypes.uint16)
f.exchange_id = ProtoField.uint8("SSEHpfTick.exchange_id", "Exchange ID", ftypes.uint8)
f.data_year = ProtoField.uint16("SSEHpfTick.data_year", "Data Year", ftypes.uint16)
f.data_month = ProtoField.uint8("SSEHpfTick.data_month", "Data Month", ftypes.uint8)
f.data_day = ProtoField.uint8("SSEHpfTick.data_day", "Data Day", ftypes.uint8)
f.send_time = ProtoField.uint32("SSEHpfTick.send_time", "Send Time", ftypes.uint32)
f.category_id = ProtoField.uint8("SSEHpfTick.category_id", "Category ID", ftypes.uint8)
f.msg_seq_id = ProtoField.uint32("SSEHpfTick.msg_seq_id", "Msg Seq ID", ftypes.uint32)
f.seq_lost_flag = ProtoField.uint8("SSEHpfTick.seq_lost_flag", "Seq Lost Flag", ftypes.uint8)

-- SSEHpfTick (including Order and Trade).
f.tick_index = ProtoField.uint32("SSEHpfTick.tick_index", "Tick Index", ftypes.uint32)
f.channel_id = ProtoField.uint32("SSEHpfTick.channel_id", "Channel ID", ftypes.uint32)
f.symbol_id = ProtoField.string("SSEHpfTick.symbol_id", "Symbol ID")
f.secu_type = ProtoField.uint8("SSEHpfTick.secu_type", "Secu Type", ftypes.uint8)
f.sub_secu_type = ProtoField.uint8("SSEHpfTick.sub_secu_type", "Sub Secu Type", ftypes.uint8)
f.tick_time = ProtoField.uint32("SSEHpfTick.tick_time", "Tick Time", ftypes.uint32)
f.tick_type = ProtoField.string("SSEHpfTick.tick_type", "Tick Type", base.ASCII)
f.buy_order_no = ProtoField.uint64("SSEHpfTick.buy_order_no", "Buy Order No", ftypes.uint64)
f.sell_order_no = ProtoField.uint64("SSEHpfTick.sell_order_no", "Sell Order No", ftypes.uint64)
f.order_price = ProtoField.uint32("SSEHpfTick.order_price", "Order Price", ftypes.uint32)
f.qty = ProtoField.uint64("SSEHpfTick.qty", "Qty", ftypes.uint64)
f.trade_money = ProtoField.uint64("SSEHpfTick.trade_money", "Trade Money", ftypes.uint64)
f.side_flag = ProtoField.uint8("SSEHpfTick.side_flag", "Side Flag", ftypes.uint8)
f.instrument_status = ProtoField.uint8("SSEHpfTick.instrument_status", "Instrument Status", ftypes.uint8)
f.reserved_2 = ProtoField.string("SSEHpfTick.reserved", "Reserved")

function p_SSEHpfTick.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "SSEHpfTick"

    local subtree = tree:add(p_SSEHpfTick, buffer(), "SSE Hpf Tick Protocol")

    -- SSEHpfPackageHead.
    subtree:add_le(f.seq_num, buffer(0, 4))
    subtree:add_le(f.reserved_1, buffer(4, 4))
    subtree:add_le(f.msg_type, buffer(8, 1))
    subtree:add_le(f.msg_len, buffer(9, 2))
    subtree:add_le(f.exchange_id, buffer(11, 1))
    subtree:add_le(f.data_year, buffer(12, 2))
    subtree:add_le(f.data_month, buffer(14, 1))
    subtree:add_le(f.data_day, buffer(15, 1))
    subtree:add_le(f.send_time, buffer(16, 4))
    subtree:add_le(f.category_id, buffer(20, 1))
    subtree:add_le(f.msg_seq_id, buffer(21, 4))
    subtree:add_le(f.seq_lost_flag, buffer(25, 1))

    -- SSEHpfTick (including Order and Trade).
    sse_tick_subtree = subtree:add(p_SSEHpfTick, buffer(), "SSE Tick")
    sse_tick_subtree:add_le(f.tick_index, buffer(26, 4))
    sse_tick_subtree:add_le(f.channel_id, buffer(30, 4))
    sse_tick_subtree:add(f.symbol_id, buffer(34, 9))
    sse_tick_subtree:add_le(f.secu_type, buffer(43, 1))
    sse_tick_subtree:add_le(f.sub_secu_type, buffer(44, 1))
    sse_tick_subtree:add_le(f.tick_time, buffer(45, 4))
    sse_tick_subtree:add(f.tick_type, buffer(49, 1))
    sse_tick_subtree:add_le(f.buy_order_no, buffer(50, 8))
    sse_tick_subtree:add_le(f.sell_order_no, buffer(58, 8))
    sse_tick_subtree:add_le(f.order_price, buffer(66, 4))
    sse_tick_subtree:add_le(f.qty, buffer(70, 8))
    sse_tick_subtree:add_le(f.trade_money, buffer(78, 8))
    sse_tick_subtree:add_le(f.side_flag, buffer(86, 1))
    sse_tick_subtree:add_le(f.instrument_status, buffer(87, 1))
    sse_tick_subtree:add(f.reserved_2, buffer(88, 8))
end

local udp_port = 37126
DissectorTable.get("udp.port"):add(udp_port, p_SSEHpfTick)
