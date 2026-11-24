#!/usr/bin/env python3
"""Fit Bubu's global evaluation scale from labeled self-play positions.

Input CSV columns: score_cp,result. Scores are White's centipawns and result
is White's score (1.0 win, 0.5 draw, 0.0 loss). The script reports a proposed
params::eval_scale_percent; it does not edit source files automatically.
"""

import argparse
import csv
import math


def load_rows(path):
    rows = []
    with open(path, newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            score = float(row["score_cp"])
            result = float(row["result"])
            if not 0.0 <= result <= 1.0:
                raise ValueError("result values must be between 0 and 1")
            rows.append((score, result))
    if not rows:
        raise ValueError("input contains no labeled positions")
    return rows


def fit_scale(rows, iterations):
    scale = 1.0
    log10 = math.log(10.0)
    for step in range(iterations):
        gradient = 0.0
        for score_cp, result in rows:
            exponent = max(-40.0, min(40.0, -log10 * scale * score_cp / 400.0))
            expected = 1.0 / (1.0 + math.exp(exponent))
            gradient += (expected - result) * log10 * score_cp / 400.0
        gradient /= len(rows)
        learning_rate = 0.5 / math.sqrt(1.0 + step / 100.0)
        scale = max(0.05, min(5.0, scale - learning_rate * gradient))
    return round(scale * 100)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", help="CSV with score_cp,result columns")
    parser.add_argument("--iterations", type=int, default=1000)
    args = parser.parse_args()
    rows = load_rows(args.csv)
    proposed = fit_scale(rows, max(1, args.iterations))
    print(f"positions: {len(rows)}")
    print(f"proposed params::eval_scale_percent = {proposed}")
    print("Validate the parameter with a fixed-depth bench and SPRT before adopting it.")


if __name__ == "__main__":
    main()
