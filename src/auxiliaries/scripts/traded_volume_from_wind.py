#!/usr/bin/env python3

import argparse
import logging

import pandas as pd

from sqlalchemy import create_engine


def connect_to_sql(host, user, password, database):
    return create_engine(f"mysql+pymysql://{user}:{password}@{host}/{database}")


def traded_volume_from_wind(date, engine):
    return pd.read_sql(
        f"SELECT S_INFO_WINDCODE, S_DQ_VOLUME FROM `AShareEODPrices` WHERE TRADE_DT = '{date}'",
        engine
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Convert history tick data to std_tick.")

    parser.add_argument(
        "--host",
        type = str,
        default = "127.0.0.1:3306",
        help = "host of MySQL server",
    )

    parser.add_argument(
        "--user",
        type = str,
        default = "root",
        help = "user of MySQL server",
    )

    parser.add_argument(
        "--password",
        type = str,
        default = "123456",
        help = "password of MySQL server",
    )

    parser.add_argument(
        "--database",
        type = str,
        default = "wind",
        help = "database to read data from",
    )

    parser.add_argument(
        "-o",
        "--output-file",
        type = str,
        default = "./traded_volume.csv",
        help = "file to save traded volume data",
    )

    parser.add_argument(
        "-d",
        "--date",
        type = int,
        help = "traded volume data date"
    )

    args = parser.parse_args()

    logging.basicConfig(
        level = logging.INFO,
        format = '[%(asctime)s] [%(levelname)s] %(message)s',
        datefmt = '%Y-%m-%d %H:%M:%S'
    )

    engine = connect_to_sql(args.host, args.user, args.password, args.database)


    if args.date:
        df = traded_volume_from_wind(args.date, engine)
        df["S_DQ_VOLUME"] = df["S_DQ_VOLUME"] * 100
        df["S_DQ_VOLUME"] = df["S_DQ_VOLUME"].astype(int)
        df.to_csv(args.output_file, index = False)
    else:
        logging.info("Please specify date")
