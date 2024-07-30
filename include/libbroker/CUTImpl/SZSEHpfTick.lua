-- Wireshark Lua Plugin for SZSE Hpf Tick Protocol.

-- Usage:
--   Copy this file into your wireshark plugins directory and
--   Analyze -> Reload Lua Plugins (Ctrl-Shift-L).

local p_SZSEHpfTick = Proto("SZSEHpfTick", "SZSE Hpf Tick Protocol")

local f = p_SZSEHpfTick.fields

-- SZSEHpfPackageHead.
f.sequence = ProtoField.uint32("SZSEHpfTick.sequence", "Sequence", ftypes.uint32)
f.tick1 = ProtoField.uint16("SZSEHpfTick.tick1", "Tick1", ftypes.uint16)
f.tick2 = ProtoField.uint16("SZSEHpfTick.tick2", "Tick2", ftypes.uint16)
f.message_type = ProtoField.uint8("SZSEHpfTick.message_type", "Message Type", ftypes.uint8)
f.security_type = ProtoField.uint8("SZSEHpfTick.security_type", "Security Type", ftypes.uint8)
f.sub_security_type = ProtoField.uint8("SZSEHpfTick.sub_security_type", "Sub Security Type", ftypes.uint8)
f.symbol = ProtoField.string("SZSEHpfTick.symbol", "Symbol")
f.exchange_id = ProtoField.uint8("SZSEHpfTick.exchange_id", "Exchange ID", ftypes.uint8)
f.quote_update_time = ProtoField.uint64("SZSEHpfTick.quote_update_time", "Quote Update Time", ftypes.uint64)
f.channel_num = ProtoField.uint16("SZSEHpfTick.channel_num", "Channel Number", ftypes.uint16)
f.sequence_num = ProtoField.int64("SZSEHpfTick.sequence_num", "Sequence Number", ftypes.int64)
f.md_streaid = ProtoField.int32("SZSEHpfTick.md_streaid", "MD Stream ID", ftypes.int32)

-- SZSEHpfOrderTick.
f.px = ProtoField.uint32("SZSEHpfTick.px", "Price", ftypes.uint32)
f.qty = ProtoField.uint64("SZSEHpfTick.qty", "Quantity", ftypes.uint64)
f.side = ProtoField.char("SZSEHpfTick.side", "Side")
f.order_type = ProtoField.char("SZSEHpfTick.order_type", "Order Type")
f.reserved = ProtoField.bytes("SZSEHpfTick.reserved", "Reserved", base.NONE)

-- SZSEHpfTradeTick.
f.bid_app_seq_num = ProtoField.int64("SZSEHpfTick.bid_app_seq_num", "Bid App Seq Num", ftypes.int64)
f.ask_app_seq_num = ProtoField.int64("SZSEHpfTick.ask_app_seq_num", "Ask App Seq Num", ftypes.int64)
f.exe_px = ProtoField.uint32("SZSEHpfTick.exe_px", "Execution Price", ftypes.uint32)
f.exe_qty = ProtoField.uint64("SZSEHpfTick.exe_qty", "Execution Quantity", ftypes.uint64)
f.exe_type = ProtoField.char("SZSEHpfTick.exe_type", "Execution Type")

local odtd_dissectors = {
    [23] = function(buffer, pinfo, tree) -- 23 stands for 逐笔委托行情.
        tree:add_le(f.px, buffer(43, 4))
        tree:add_le(f.qty, buffer(47, 8))
        tree:add(f.side, buffer(55, 1))
        tree:add(f.order_type, buffer(56, 1))
        tree:add(f.reserved, buffer(57, 7))
    end,
    [24] = function(buffer, pinfo, tree) -- 24 stands for 逐笔成交行情.
        tree:add_le(f.bid_app_seq_num, buffer(43, 8))
        tree:add_le(f.ask_app_seq_num, buffer(51, 8))
        tree:add_le(f.exe_px, buffer(59, 4))
        tree:add_le(f.exe_qty, buffer(63, 8))
        tree:add(f.exe_type, buffer(71, 1))
    end,
}

function p_SZSEHpfTick.dissector(buffer, pinfo, tree)
    pinfo.cols.protocol = "SZSEHpfTick"

    local subtree = tree:add(p_SZSEHpfTick, buffer(), "SZSE Hpf Tick Protocol")

    -- SZSEHpfPackageHead.
    subtree:add_le(f.sequence, buffer(0, 4))
    subtree:add_le(f.tick1, buffer(4, 2))
    subtree:add_le(f.tick2, buffer(6, 2))
    subtree:add_le(f.message_type, buffer(8, 1))
    subtree:add_le(f.security_type, buffer(9, 1))
    subtree:add_le(f.sub_security_type, buffer(10, 1))
    subtree:add(f.symbol, buffer(11, 9))
    subtree:add_le(f.exchange_id, buffer(20, 1))
    subtree:add_le(f.quote_update_time, buffer(21, 8))
    subtree:add_le(f.channel_num, buffer(29, 2))
    subtree:add_le(f.sequence_num, buffer(31, 8))
    subtree:add_le(f.md_streaid, buffer(39, 4))

    local message_type = buffer(8, 1):le_uint()
    local dissector = odtd_dissectors[message_type]

    if dissector then
        local tick_data_subtree
        if message_type == 23 then
            tick_data_subtree = subtree:add(p_SZSEHpfTick, buffer(), "Order Tick")
        elseif message_type == 24 then
            tick_data_subtree = subtree:add(p_SZSEHpfTick, buffer(), "Trade Tick")
        end
        dissector(buffer, pinfo, tick_data_subtree)
    end
end

local udp_port = 38202
DissectorTable.get("udp.port"):add(udp_port, p_SZSEHpfTick)
