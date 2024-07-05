#!/usr/bin/env python3

###################
# 上交所逐笔数据说明 #
###################

# 逐笔委托中，仅以下字段是有效字段：
#   交易所代码
#   自然日
#   时间
#   交易所委托号
#   委托类型
#   委托代码
#   委托价格
#   委托数量
#
# 对于交易价格：交易价格为实际价格的 10000 倍；
# 对于委托类型：A 为新增委托，D 为撤单。视所有委托为限价委托。无其他有效值；
# 对于委托代码：B 为买，S 为卖。无其他有效值；
# 对于所有的撤单（委托类型为 D）：委托代码、委托价格和委托数量与原委托相同。
#
# 逐笔成交中，仅以下字段是有效字段：
#   交易所代码
#   自然日
#   时间
#   成交价格
#   成交数量
#   叫卖序号
#   叫买序号
#
# 逐笔成交中叫卖序号/叫买序号对应逐笔委托中的交易所委托号

###################
# 深交所逐笔数据说明 #
###################

# 逐笔委托中，仅以下字段是有效字段：
#   交易所代码
#   自然日
#   时间
#   交易所委托号
#   委托类型
#   委托代码
#   委托价格
#   委托数量
#
# 对于交易价格：交易价格为实际价格的 10000 倍；
# 对于委托类型：0 为限价委托，1 为对方最优价（市价）委托，U 为本方最优价委托。无其他有效值；
# 对于委托代码：B 为买，S 为卖。无其他有效值；
# 对于所有的撤单（委托类型为 D）：委托代码、委托价格和委托数量与原委托相同。
#
# 深交所逐笔委托中不含有撤单信息。撤单信息在逐步笔成交中。
#
# 逐笔成交中，仅以下字段是有效字段：
#   交易所代码
#   自然日
#   时间
#   成交代码
#   成交价格
#   成交数量
#   叫卖序号
#   叫买序号
#
# 对于成交代码：0 为成交，C 为撤单。无其他有效值；
# 对于所有的撤单（成交代码为 C）：委托价格均为 0。

#######################
# 标准 Tick 数据格式说明 #
#######################

# 每份标准 Tick 数据为某股票某天的所有逐笔委托和逐笔成交数据。
# 可通过一份标准 Tick 数据撮合出某股某天的逐笔行情并验证。
#
# 标准 Tick 数据含有以下字段：
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
# 对于 time：是原始逐笔数据中自然日和时间的组合值。

import argparse
import datetime
import logging
import os

import numpy as np
import pandas as pd

from concurrent.futures import ProcessPoolExecutor, as_completed


def list_symbols(input_folder):
    for symbol in os.listdir(input_folder):
        yield symbol


def check_exchange(symbol):
    if symbol.endswith(".SH"):
        return "SSE"
    elif symbol.endswith(".SZ"):
        return "SZSE"
    else:
        return ""


def convert(input_folder, output_folder, skip_converted = False):
    converted_list = []
    no_data_list = []
    bad_data_list = []

    with ProcessPoolExecutor() as executor:
        future_to_symbol = {
            executor.submit(do_convert, input_folder, symbol_folder, output_folder, skip_converted):
                symbol_folder for symbol_folder in list_symbols(input_folder)
        }

        for future in as_completed(future_to_symbol):
            symbol_folder = future_to_symbol[future]
            try:
                converted = future.result()
            except Exception as e:
                logging.error(f"Failed to convert symbol {symbol_folder}: {e}")
                bad_data_list.append(symbol_folder)
            else:
                if converted:
                    converted_list.append(symbol_folder)
                else:
                    no_data_list.append(symbol_folder)

    return converted_list, no_data_list, bad_data_list


def do_convert(input_folder, symbol_folder, output_folder, skip_converted):
    if not os.path.isdir(os.path.join(input_folder, symbol_folder)):
        return False

    if skip_converted and os.path.exists(os.path.join(output_folder, f"{symbol_folder}-std-tick.csv")):
        logging.info(f"Skipping converted symbol {symbol_folder}")
        return False

    exchange = check_exchange(symbol_folder)

    if exchange == "SSE":
        return convert_sse(input_folder, symbol_folder, output_folder)
    elif exchange == "SZSE":
        return convert_szse(input_folder, symbol_folder, output_folder)
    else:
        logging.error(f"Unknown exchange for symbol {symbol_folder}")
        return False


def convert_sse(input_folder, symbol_folder, output_folder):
    logging.info(f"Loading {symbol_folder}'s data from folder {os.path.join(input_folder, symbol_folder)}")

    od = convert_sse_od(os.path.join(input_folder, symbol_folder))
    td = convert_sse_td(os.path.join(input_folder, symbol_folder))

    if od is None or td is None:
        logging.warning(f"No data for symbol {symbol_folder}, skipped")
        return False

    std_tick = join_to_std_tick(od, td)

    std_tick.to_csv(os.path.join(output_folder, f"{symbol_folder}-std-tick.csv"), index = False)

    logging.info(f"Saved {symbol_folder}'s std tick in file {os.path.join(output_folder, symbol_folder)}-std-tick.csv")

    return True


def convert_sse_od(input_file):
    od = pd.read_csv(os.path.join(input_file, "逐笔委托.csv"), encoding = "gbk", low_memory = False)

    if len(od) <= 2:
        return None

    od = od[(od["委托代码"] == "B") | (od["委托代码"] == "S")]
    od["时间"] = od["时间"].apply(
        lambda x: "0" + str(int(x)) if len(str(int(x))) == 8 else str(int(x))
    )

    od.rename(
        columns = {
            "交易所代码"  : "symbol",
            "交易所委托号": "unique_id",
            "委托类型"    : "order_type",
            "委托代码"    : "side",
            "委托价格"    : "price",
            "委托数量"    : "quantity",
            "自然日"      : "date",
            "时间"        : "time",
        },
        inplace = True,
    )

    od["symbol"] = od["symbol"].apply(lambda x: f"{x:06d}")
    od["order_type"] = od["order_type"].map({"A": "L", "D": "C", })
    od["price"] = od["price"].apply(lambda x: float(x) / 10000)
    od["ask_unique_id"] = np.where(od["side"] == "S", od["unique_id"], 0)
    od["bid_unique_id"] = np.where(od["side"] == "B", od["unique_id"], 0)
    od["time"] = od.apply(lambda x: str(int(x["date"])) + str(x["time"]), axis = 1)

    od = od[
        [
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


def convert_sse_td(input_file):
    td = pd.read_csv(os.path.join(input_file, "逐笔成交.csv"), encoding = "gbk", low_memory = False)

    if len(td) <= 2:
        return None

    td = td[td["成交编号"] != 0]
    td["时间"] = td["时间"].apply(
        lambda x: "0" + str(int(x)) if len(str(int(x))) == 8 else str(int(x))
    )

    td.rename(
        columns = {
            "交易所代码": "symbol",
            "叫卖序号"  : "ask_unique_id",
            "叫买序号"  : "bid_unique_id",
            "成交价格"  : "price",
            "成交数量"  : "quantity",
            "自然日"    : "date",
            "时间"      : "time",
        },
        inplace = True,
    )

    td["symbol"] = td["symbol"].apply(lambda x: f"{x:06d}")
    td["order_type"] = "T"
    td["price"] = td["price"].apply(lambda x: float(x) / 10000)
    td["time"] = td.apply(lambda x: str(int(x["date"])) + str(x["time"]), axis = 1)

    td = td[
        [
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


def convert_szse(input_folder, symbol_folder, output_folder):
    logging.info(f"Loading {symbol_folder}'s data from folder {os.path.join(input_folder, symbol_folder)}")

    od = convert_szse_od(os.path.join(input_folder, symbol_folder))
    td = convert_szse_td(os.path.join(input_folder, symbol_folder))

    if od is None or td is None:
        logging.warning(f"No data for symbol {symbol_folder}, skipped")
        return False

    std_tick = join_to_std_tick(od, td)

    std_tick.to_csv(os.path.join(output_folder, f"{symbol_folder}-std-tick.csv"), index = False)

    logging.info(f"Saved {symbol_folder}'s std tick in file {os.path.join(output_folder, symbol_folder)}-std-tick.csv")

    return True


def convert_szse_od(input_file):
    od = pd.read_csv(os.path.join(input_file, "逐笔委托.csv"), encoding = "gbk", low_memory = False)

    if len(od) <= 2:
        return None

    od = od[(od["委托代码"] == "B") | (od["委托代码"] == "S")]
    od["时间"] = od["时间"].apply(
        lambda x: "0" + str(int(x)) if len(str(int(x))) == 8 else str(int(x))
    )

    od.rename(
        columns = {
            "交易所代码"  : "symbol",
            "交易所委托号": "unique_id",
            "委托类型"    : "order_type",
            "委托代码"    : "side",
            "委托价格"    : "price",
            "委托数量"    : "quantity",
            "自然日"      : "date",
            "时间"        : "time",
        },
        inplace = True,
    )

    od["symbol"] = od["symbol"].apply(lambda x: f"{x:06d}")
    od["order_type"] = od["order_type"].map({"0": "L", "1": "M", "U": "B"})
    od["price"] = od["price"].apply(lambda x: float(x) / 10000)
    od["ask_unique_id"] = np.where(od["side"] == "S", od["unique_id"], 0)
    od["bid_unique_id"] = np.where(od["side"] == "B", od["unique_id"], 0)
    od["time"] = od.apply(lambda x: str(int(x["date"])) + str(x["time"]), axis = 1)

    od = od[
        [
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


def convert_szse_td(input_file):
    td = pd.read_csv(os.path.join(input_file, "逐笔成交.csv"), encoding = "gbk", low_memory = False)

    if len(td) <= 2:
        return None

    td = td[td["成交编号"] != 0]
    td["时间"] = td["时间"].apply(
        lambda x: "0" + str(int(x)) if len(str(int(x))) == 8 else str(int(x))
    )

    td.rename(
        columns = {
            "交易所代码": "symbol",
            "叫卖序号"  : "ask_unique_id",
            "叫买序号"  : "bid_unique_id",
            "成交代码"  : "order_type",
            "成交价格"  : "price",
            "成交数量"  : "quantity",
            "自然日"    : "date",
            "时间"      : "time",
        },
        inplace = True,
    )

    td["symbol"] = td["symbol"].apply(lambda x: f"{x:06d}")
    td["order_type"] = td["order_type"].map({"0": "T", "C": "C"})
    td["price"] = td["price"].apply(lambda x: float(x) / 10000)
    td["time"] = td.apply(lambda x: str(int(x["date"])) + str(x["time"]), axis = 1)

    td = td[
        [
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

    # Sort by time.
    std_tick = std_tick.sort_values(by = "time", ascending = True, kind = "mergesort")

    logging.debug(f"\n{std_tick}")

    return std_tick


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Convert history tick data to std_tick.")

    parser.add_argument(
        "-i",
        "--input-folder",
        type = str,
        default = ".",
        help = "folder containing history tick files",
    )

    parser.add_argument(
        "-o",
        "--output-folder",
        type = str,
        default = "./std_tick",
        help = "folder to save std_tick files",
    )

    parser.add_argument(
        "-s",
        "--skip-converted",
        action = "store_true",
        help = "skip already converted files",
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

    converted_list, no_data_list, bad_data_list = convert(args.input_folder, args.output_folder, args.skip_converted)

    if len(no_data_list) > 0:
        logging.warning(f"{len(no_data_list)} symbols are not converted because of no data: {no_data_list}")

    if len(bad_data_list) > 0:
        logging.warning(f"{len(bad_data_list)} symbols are not converted because of bad data: {bad_data_list}")

    logging.info(f"Converted {len(converted_list)} symbols in {datetime.datetime.now() - now}")
