#!/usr/bin/env python3

#######################
# 标准 Tick 数据格式说明 #
#######################

# 每份标准 Tick 数据为某股票某天的所有逐笔委托和逐笔成交数据。
# 可通过一份标准 Tick 数据撮合出某股某天的逐笔行情并验证。
#
# 标准 Tick 数据含有以下字段：
#   index
#   symbol
#   ask_unique_id
#   bid_unique_id
#   order_type
#   price
#   quantity
#   time
#
# 对于 ask_unique_id/bid_unique_id：对应原始逐笔数据中的交易所委托号，且可通过两者值确定委托的交易方向；
# 对于 order_type：仅含 L、M、B、C 和 T 三类，分别表示限价委托、对方最优价（市价）委托、本方最优价委托、撤单和成交；
# 对于 time：自然日和时间的组合值。

import argparse
import datetime
import logging
import os

import numpy as np
import pandas as pd

from enum import Enum


class ErrorType(Enum):
    NoError = 0
    NoFile = 1
    NoData = 2
    BadData = 3


def list_files(input_folder):
    for symbol in os.listdir(input_folder):
        yield symbol


def convert(input_folder, output_folder):
    converted_list = []
    no_data_list = []
    bad_data_list = []

    error_type = do_convert(input_folder, output_folder)

    match error_type:
        case ErrorType.NoError:
            converted_list.append(input_folder)
        case ErrorType.NoData:
            no_data_list.append(input_folder)
        case ErrorType.BadData:
            bad_data_list.append(input_folder)
        case _:
            pass

    return converted_list, no_data_list, bad_data_list


def do_convert(input_folder, output_folder):
    if not os.path.isdir(input_folder):
        return ErrorType.NoFile

    sse_tick_file = os.path.join(input_folder, "sse-tick.csv")
    szse_order_file = os.path.join(input_folder, "szse-order-tick.csv")
    szse_trade_file = os.path.join(input_folder, "szse-trade-tick.csv")

    try:
        sse_std_ticks = convert_sse(sse_tick_file, output_folder)
        szse_std_ticks = convert_szse(szse_order_file, szse_trade_file, output_folder)
        calculate_quantity(sse_std_ticks, szse_std_ticks, output_folder)
    except Exception as e:
        logging.error(f"Failed to convert: {e}")
        return ErrorType.BadData


def convert_sse(tick_file, output_folder):
    logging.info(f"Loading sse data from file {tick_file}")

    std_tick = pd.read_csv(tick_file, low_memory = False)

    if std_tick is None:
        logging.warning(f"No data for file {tick_file}, skipped")
        return None

    # del std_tick["time"]

    std_tick.rename(
        columns = {
            "symbol_id"    : "symbol",
            "sell_order_no": "ask_unique_id",
            "buy_order_no" : "bid_unique_id",
            "tick_type"    : "order_type",
            "order_price"  : "price",
            "qty"          : "quantity",
            "tick_time"    : "time",
        },
        inplace = True,
    )

    std_tick["index"] = std_tick["tick_index"]
    std_tick["symbol"] = std_tick["symbol"].apply(lambda x: f"{x:06d}")
    std_tick["order_type"] = std_tick["order_type"].map({"A": "L", "D": "C", "T": "T"})
    std_tick["time"] = (std_tick["data_year"].astype(int).astype(str)
                        + std_tick["data_month"].astype(int).astype(str).str.zfill(2)
                        + std_tick["data_day"].astype(int).astype(str).str.zfill(2)
                        + std_tick["time"].astype(str).str.zfill(8))
    std_tick["price"] = std_tick["price"].apply(lambda x: float(x) / 1000)
    std_tick["quantity"] = std_tick["quantity"].apply(lambda x: int(int(x) / 1000))

    std_tick.dropna(subset = ["order_type"], inplace = True)

    std_tick = std_tick[
        [
            "index",
            "symbol",
            "ask_unique_id",
            "bid_unique_id",
            "order_type",
            "price",
            "quantity",
            "time",
        ]
    ]

    grouped = std_tick.groupby("symbol")

    for symbol, group in grouped:
        group.to_csv(os.path.join(output_folder, f"{symbol}-std-tick.csv"), index = False)
        logging.info(f"Saved sse's std tick in file {os.path.join(output_folder, f'{symbol}-std-tick.csv')}")

    return std_tick


def convert_szse(order_file, trade_file, output_folder):
    logging.info(f"Loading szse order data from file {order_file}")
    od = convert_szse_od(os.path.join(order_file))

    logging.info(f"Loading szse trade data from file {trade_file}")
    td = convert_szse_td(os.path.join(trade_file))

    if od is None or td is None:
        logging.warning(f"No data for file {order_file} or {trade_file}, skipped")
        return None

    std_tick = join_to_std_tick(od, td)

    grouped = std_tick.groupby("symbol")

    for symbol, group in grouped:
        group.to_csv(os.path.join(output_folder, f"{symbol}-std-tick.csv"), index = False)
        logging.info(f"Saved szse's std tick in file {os.path.join(output_folder, f'{symbol}-std-tick.csv')}")

    return std_tick


def convert_szse_od(order_file):
    od = pd.read_csv(order_file, low_memory = False)

    if len(od) <= 2:
        return None

    # del od["time"]

    od.rename(
        columns = {
            "sequence"         : "index",
            "px"               : "price",
            "qty"              : "quantity",
            "quote_update_time": "time",
        },
        inplace = True,
    )

    od["symbol"] = od["symbol"].apply(lambda x: f"{x:06d}")
    od["ask_unique_id"] = np.where(od["side"] == 2, od["sequence_num"], 0)
    od["bid_unique_id"] = np.where(od["side"] == 1, od["sequence_num"], 0)
    od["order_type"] = od["order_type"].map({"2": "L", "1": "M", "U": "B"})
    od["price"] = od["price"].apply(lambda x: float(x) / 10000)
    od["quantity"] = od["quantity"].apply(lambda x: int(int(x) / 100))

    od = od[
        [
            "index",
            "symbol",
            "ask_unique_id",
            "bid_unique_id",
            "order_type",
            "price",
            "quantity",
            "time",
        ]
    ]

    logging.debug(f"\n{od}")

    return od


def convert_szse_td(trade_file):
    td = pd.read_csv(trade_file, low_memory = False)

    if len(td) <= 2:
        return None

    # del td["time"]

    td.rename(
        columns = {
            "sequence"         : "index",
            "ask_app_seq_num"  : "ask_unique_id",
            "bid_app_seq_num"  : "bid_unique_id",
            "exe_type"         : "order_type",
            "exe_px"           : "price",
            "exe_qty"          : "quantity",
            "quote_update_time": "time",
        },
        inplace = True,
    )

    td["symbol"] = td["symbol"].apply(lambda x: f"{x:06d}")
    td["order_type"] = td["order_type"].map({"F": "T", "4": "C"})
    td["price"] = td["price"].apply(lambda x: float(x) / 10000)
    td["quantity"] = td["quantity"].apply(lambda x: int(int(x) / 100))

    td = td[
        [
            "index",
            "symbol",
            "ask_unique_id",
            "bid_unique_id",
            "order_type",
            "price",
            "quantity",
            "time",
        ]
    ]

    logging.debug(f"\n{td}")

    return td


def join_to_std_tick(od, td):
    std_tick = pd.concat([od, td])

    # Sort by index.
    std_tick = std_tick.sort_values(
        by = "index",
        ascending = True,
        kind = "mergesort"
    )

    logging.debug(f"\n{std_tick}")

    return std_tick


def calculate_quantity(sse_std_tick, szse_std_tick, output_folder):
    sse_traded_tick = sse_std_tick[sse_std_tick["order_type"] == "T"]
    szse_traded_tick = szse_std_tick[szse_std_tick["order_type"] == "T"]

    traded_tick = pd.concat([sse_traded_tick, szse_traded_tick], ignore_index = True)

    traded_tick = traded_tick.groupby("symbol")["quantity"].sum().reset_index()

    traded_tick = traded_tick[["symbol", "quantity"]]

    traded_tick = traded_tick.sort_values(by = "symbol")

    # Remove all symbols that start with "1", "2" or "5".
    traded_tick = traded_tick[~traded_tick["symbol"].str.startswith(("1", "2", "5"))]

    output_path = os.path.join(output_folder, "quantity_from_std_ticks.csv")
    traded_tick.to_csv(output_path, index = False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Convert raw tick data to std_tick.")

    parser.add_argument(
        "-i",
        "--input-folder",
        type = str,
        default = ".",
        help = "folder containing raw tick files",
    )

    parser.add_argument(
        "-o",
        "--output-folder",
        type = str,
        default = "./std_tick",
        help = "folder to save std_tick files",
    )

    parser.add_argument(
        "-d",
        "--debug",
        action = "store_true",
        help = "enable debug log",
    )

    parser.add_argument(
        "-w",
        "--warnings-only",
        action = "store_true",
        help = "only show warning log",
    )

    args = parser.parse_args()

    if args.debug:
        logging_level = logging.DEBUG
    else:
        logging_level = logging.INFO

    if args.warnings_only:
        logging_level = logging.WARN

    logging.basicConfig(
        level = logging_level,
        format = '[%(asctime)s] [%(levelname)s] %(message)s',
        datefmt = '%Y-%m-%d %H:%M:%S'
    )

    now = datetime.datetime.now()

    if not os.path.exists(args.output_folder):
        os.mkdir(os.path.join(args.output_folder))

    (converted_list, no_data_list, bad_data_list) = convert(args.input_folder, args.output_folder)

    if len(no_data_list) > 0:
        logging.warning(f"{len(no_data_list)} symbols are not converted because of no data: {no_data_list}")

    if len(bad_data_list) > 0:
        logging.warning(f"{len(bad_data_list)} symbols are not converted because of bad data: {bad_data_list}")

    logging.info(f"Converted all symbols in {datetime.datetime.now() - now}")
