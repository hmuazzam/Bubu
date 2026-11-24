#!/usr/bin/env python3
"""Generate labeled self-play positions for the small Texel scale tuner."""

import argparse
import csv
import queue
import re
import subprocess
import threading


FEN_PATTERN = re.compile(r"^[pnbrqkPNBRQK1-8/]+ [wb] [KQkq-]+ [-a-h1-8]+ \d+ \d+$")


def launch(engine):
    process = subprocess.Popen(
        [engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL, text=True, bufsize=1
    )
    output = queue.Queue()

    def collect():
        for line in process.stdout:
            output.put(line.strip())

    threading.Thread(target=collect, daemon=True).start()
    send(process, "uci")
    read_until(output, lambda line: line == "uciok")
    send(process, "setoption name Hash value 16")
    send(process, "ucinewgame")
    return process, output


def send(process, command):
    process.stdin.write(command + "\n")
    process.stdin.flush()


def read_until(output, predicate):
    while True:
        line = output.get(timeout=60)
        if predicate(line):
            return line


def current_fen(process, output, moves):
    suffix = " " + " ".join(moves) if moves else ""
    send(process, "position startpos moves" + suffix)
    send(process, "d")
    while True:
        line = output.get(timeout=10)
        if FEN_PATTERN.match(line):
            return line


def game_result(engine, fen):
    result = subprocess.run(
        [engine, "outcome", *fen.split()], check=True,
        capture_output=True, text=True
    )
    return result.stdout.strip()


def play_game(engine, depth, max_plies):
    process, output = launch(engine)
    moves = []
    rows = []
    seen = {}
    result = "1/2-1/2"
    try:
        for _ in range(max_plies):
            fen = current_fen(process, output, moves)
            result = game_result(engine, fen)
            if result != "*":
                break
            repetition_key = " ".join(fen.split()[:4])
            seen[repetition_key] = seen.get(repetition_key, 0) + 1
            if seen[repetition_key] >= 3:
                result = "1/2-1/2"
                break

            send(process, f"go depth {depth}")
            last_score = 0
            while True:
                line = output.get(timeout=60)
                if line.startswith("info "):
                    score = re.search(r" score cp (-?\d+)(?: |$)", line)
                    mate = re.search(r" score mate (-?\d+)(?: |$)", line)
                    if score:
                        last_score = int(score.group(1))
                    elif mate:
                        last_score = 30000 if int(mate.group(1)) > 0 else -30000
                if line.startswith("bestmove "):
                    bestmove = line.split()[1]
                    break
            if bestmove == "0000":
                result = game_result(engine, fen)
                break
            white_score = last_score if fen.split()[1] == "w" else -last_score
            rows.append((fen, white_score))
            moves.append(bestmove)
        else:
            result = "1/2-1/2"
    finally:
        send(process, "quit")
        process.wait(timeout=5)
    label = {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}.get(result, 0.5)
    return [(fen, score, label) for fen, score in rows], result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", help="path to the Bubu executable")
    parser.add_argument("output", help="output CSV path")
    parser.add_argument("--games", type=int, default=10)
    parser.add_argument("--depth", type=int, default=5)
    parser.add_argument("--max-plies", type=int, default=160)
    args = parser.parse_args()

    with open(args.output, "w", newline="", encoding="utf-8") as destination:
        writer = csv.writer(destination)
        writer.writerow(["fen", "score_cp", "result"])
        for game in range(args.games):
            rows, result = play_game(args.engine, args.depth, args.max_plies)
            writer.writerows(rows)
            print(f"game {game + 1}: {result}; positions {len(rows)}", flush=True)


if __name__ == "__main__":
    main()
