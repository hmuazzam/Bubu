#!/usr/bin/env python3
"""Compare seven candidate first moves at a total search depth of seven plies."""

import subprocess
import sys


FEN = (
    "r1bqk2r/ppp1bppp/2np1n2/4p3/2B1P3/2NP1N2/PPP2PPP/"
    "R1BQK2R w KQkq - 0 6"
)
CANDIDATES = ("e1g1", "a2a3", "h2h3", "g2g4", "a2a4", "b2b4", "g2g3")


def read_until(process, marker):
    lines = []
    while True:
        line = process.stdout.readline()
        if not line:
            raise RuntimeError("engine exited before completing the UCI command")
        line = line.strip()
        lines.append(line)
        if line == marker or line.startswith(marker):
            return lines


def send(process, command):
    process.stdin.write(command + "\n")
    process.stdin.flush()


def root_score(process, enabled, move):
    send(process, "setoption name KingSafetyV2 value " + str(enabled).lower())
    send(process, "setoption name Clear Hash")
    send(process, "position fen " + FEN + " moves " + move)
    send(process, "go depth 6")
    lines = read_until(process, "bestmove ")
    depth_six = None
    for line in lines:
        fields = line.split()
        if len(fields) >= 6 and fields[0] == "info" and fields[1] == "depth":
            if int(fields[2]) == 6:
                score_index = fields.index("score")
                score_type = fields[score_index + 1]
                value = int(fields[score_index + 2])
                depth_six = value if score_type == "cp" else (30000 if value > 0 else -30000)
    if depth_six is None:
        raise RuntimeError("engine did not report a depth-6 score for " + move)
    # The child search reports the score for Black to move; negate it for White.
    return -depth_six


def main():
    engine = sys.argv[1] if len(sys.argv) > 1 else "./bubu"
    process = subprocess.Popen(
        [engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, bufsize=1
    )
    try:
        send(process, "uci")
        read_until(process, "uciok")
        for enabled in (False, True):
            scores = [(move, root_score(process, enabled, move))
                      for move in CANDIDATES]
            scores.sort(key=lambda item: item[1], reverse=True)
            print("KingSafetyV2=" + str(enabled).lower())
            for rank, (move, score) in enumerate(scores, start=1):
                print(f"{rank}. {move} {score:+d} cp")
    finally:
        if process.poll() is None:
            send(process, "quit")
            process.wait(timeout=2)


if __name__ == "__main__":
    main()
