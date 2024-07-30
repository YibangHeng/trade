#!/usr/bin/env python3

import argparse

import pandas as pd


def volume_checker(source_file, target_file):
    source = pd.read_csv(source_file)
    target = pd.read_csv(target_file)

    source = source.rename(columns = {source.columns[0]: "symbol", source.columns[1]: "quantity"})
    target = target.rename(columns = {target.columns[0]: "symbol", target.columns[1]: "quantity"})

    # Join two dataframes.
    joined = pd.merge(source, target, on = "symbol", how = "inner")

    joined = joined.rename(columns = {joined.columns[1]: "source", joined.columns[2]: "target"})

    # Only keep the symbols that quantity in source is greater than target.
    joined = joined[joined["source"] > joined["target"]]

    # Calculate the difference.
    joined["difference"] = joined["source"] - joined["target"]

    joined.reset_index(drop=True, inplace=True)

    print(joined.to_string())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Convert history tick data to std_tick.")

    parser.add_argument(
        "-s",
        "--source",
        type = str,
        default = "./source.csv",
        help = "source file",
    )

    parser.add_argument(
        "-t",
        "--target",
        type = str,
        default = "./target.csv",
        help = "target file",
    )

    args = parser.parse_args()

    volume_checker(args.source, args.target)
