#!/usr/bin/env python3

import argparse
import logging

import pandas as pd


def calculate_quantity(sse_l2_snap_file, szse_l2_snap_file, output_file):
    sse_l2_snap = unify_sse_l2_snap(sse_l2_snap_file)
    szse_l2_snap = unify_szse_l2_snap(szse_l2_snap_file)

    sse_l2_snap = sse_l2_snap.groupby("symbol").max().reset_index()
    szse_l2_snap = szse_l2_snap.groupby("symbol").max().reset_index()

    sse_l2_snap["quantity"] = sse_l2_snap["quantity"].apply(lambda x: int(x / 1000))
    szse_l2_snap["quantity"] = szse_l2_snap["quantity"].apply(lambda x: int(x / 100))

    l2_snap = pd.concat([sse_l2_snap, szse_l2_snap], ignore_index = True)

    l2_snap["symbol"] = l2_snap["symbol"].apply(lambda x: str(x).zfill(6))

    # Remove all symbols that start with "1", "2" or "5".
    l2_snap = l2_snap[~l2_snap["symbol"].str.startswith(("1", "2", "5"))]

    # Remove all rows that quantity is 0.
    l2_snap = l2_snap[l2_snap["quantity"] > 0]

    l2_snap.sort_values(by = "symbol", inplace = True)

    l2_snap.to_csv(output_file, index = False)


def unify_sse_l2_snap(sse_l2_snap_file):
    logging.info(f"Loading sse l2 snap file: {sse_l2_snap_file}")
    sse_l2_snap = pd.read_csv(sse_l2_snap_file, low_memory = False)

    sse_l2_snap = sse_l2_snap[["symbol_id", "trade_volume"]]

    sse_l2_snap.rename(
        columns = {
            "symbol_id"   : "symbol",
            "trade_volume": "quantity",
        },
        inplace = True,
    )

    return sse_l2_snap


def unify_szse_l2_snap(szse_l2_snap_file):
    logging.info(f"Loading szse l2 snap file: {szse_l2_snap_file}")
    szse_l2_snap = pd.read_csv(szse_l2_snap_file, low_memory = False)

    szse_l2_snap = szse_l2_snap[["symbol", "total_quantity_trade"]]

    szse_l2_snap.rename(
        columns = {
            "symbol"              : "symbol",
            "total_quantity_trade": "quantity",
        },
        inplace = True,
    )

    return szse_l2_snap


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Convert history tick data to std_tick.")

    parser.add_argument(
        "--sse-l2-snap-file",
        type = str,
        default = "./sse-l2-snap.csv",
        help = "sse l2 snap file",
    )

    parser.add_argument(
        "--szse-l2-snap-file",
        type = str,
        default = "./szse-l2-snap.csv",
        help = "szse l2 snap file",
    )

    parser.add_argument(
        "-o",
        "--output-file",
        type = str,
        default = "./quantity_from_l2_snaps.csv",
        help = "file to save traded volume data",
    )

    args = parser.parse_args()

    logging.basicConfig(
        level = logging.INFO,
        format = '[%(asctime)s] [%(levelname)s] %(message)s',
        datefmt = '%Y-%m-%d %H:%M:%S'
    )

    calculate_quantity(args.sse_l2_snap_file, args.szse_l2_snap_file, args.output_file)
