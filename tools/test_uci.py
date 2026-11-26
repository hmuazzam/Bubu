#!/usr/bin/env python3
"""Small standard-library protocol and stop-responsiveness smoke test."""

import queue
import shlex
import subprocess
import sys
import threading
import time


def read_until(output, predicate, timeout):
    lines = []
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            line = output.get(timeout=max(0.001, deadline - time.monotonic()))
        except queue.Empty:
            break
        lines.append(line)
        if predicate(line):
            return lines
    raise RuntimeError(f"timed out waiting for engine output; received: {lines}")


def send(process, command):
    process.stdin.write(command + "\n")
    process.stdin.flush()


def main():
    engine = sys.argv[1] if len(sys.argv) > 1 else "./bubu"
    process = subprocess.Popen(
        [engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, bufsize=1
    )
    output = queue.Queue()

    def collect_output():
        for line in process.stdout:
            output.put(line.strip())

    reader = threading.Thread(target=collect_output, daemon=True)
    reader.start()
    try:
        send(process, "uci")
        uci_lines = read_until(output, lambda line: line == "uciok", 2.0)
        if not any("option name KingSafetyV2 type check default false" in line
                   for line in uci_lines):
            raise RuntimeError("KingSafetyV2 UCI option is missing or not disabled by default")
        send(process, "isready")
        read_until(output, lambda line: line == "readyok", 2.0)
        safety_fen = (
            "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R w KQkq - 0 1"
        )
        castled_fen = (
            "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R4RK1 w kq - 0 1"
        )
        send(process, "position fen " + safety_fen)
        send(process, "eval")
        eval_off_line = read_until(
            output, lambda line: line.startswith("info score cp "), 2.0
        )[-1]
        eval_off = int(eval_off_line.rsplit(" ", 1)[1])
        send(process, "setoption name KingSafetyV2 value true")
        send(process, "eval")
        eval_on_line = read_until(
            output, lambda line: line.startswith("info score cp "), 2.0
        )[-1]
        eval_on = int(eval_on_line.rsplit(" ", 1)[1])
        send(process, "position fen " + castled_fen)
        send(process, "eval")
        castled_line = read_until(
            output, lambda line: line.startswith("info score cp "), 2.0
        )[-1]
        if int(castled_line.rsplit(" ", 1)[1]) <= eval_on:
            raise RuntimeError("KingSafetyV2 did not prefer the castled evaluation")
        send(process, "setoption name KingSafetyV2 value false")
        send(process, "position fen " + safety_fen)
        send(process, "eval")
        eval_reset_line = read_until(
            output, lambda line: line.startswith("info score cp "), 2.0
        )[-1]
        if int(eval_reset_line.rsplit(" ", 1)[1]) != eval_off:
            raise RuntimeError("KingSafetyV2=false did not restore the default evaluation")
        send(process, "setoption name Hash value 1")
        send(process, "position startpos")
        send(process, "go depth 4")
        lines = read_until(output, lambda line: line.startswith("bestmove "), 10.0)
        bestmove = lines[-1].split()[1]
        start_moves = {
            "a2a3", "a2a4", "b2b3", "b2b4", "c2c3", "c2c4", "d2d3", "d2d4",
            "e2e3", "e2e4", "f2f3", "f2f4", "g2g3", "g2g4", "h2h3", "h2h4",
            "b1a3", "b1c3", "g1f3", "g1h3",
        }
        if bestmove not in start_moves:
            raise RuntimeError(f"search returned an illegal start-position move: {bestmove}")

        with open("tests/tactics.epd", encoding="utf-8") as suite:
            for line in suite:
                if not line.strip() or line.lstrip().startswith("#"):
                    continue
                depth, expected, fen = line.strip().split(maxsplit=2)
                legal_result = subprocess.run(
                    [engine, "perft", "divide", "1", *shlex.split(fen)],
                    check=True, capture_output=True, text=True
                )
                legal_moves = {
                    row.split(":", 1)[0]
                    for row in legal_result.stdout.splitlines()
                    if ":" in row
                }
                send(process, "position fen " + fen)
                send(process, "go depth " + depth)
                lines = read_until(output, lambda line: line.startswith("bestmove "), 10.0)
                bestmove = lines[-1].split()[1]
                if bestmove not in legal_moves:
                    raise RuntimeError(f"search returned illegal move {bestmove} for {fen}")
                if expected != "-" and bestmove != expected:
                    raise RuntimeError(f"expected {expected}, got {bestmove} for {fen}")

        send(process, "setoption name Clear Hash")
        send(process, "position startpos")
        for limit in ("nodes 1000", "movetime 50",
                      "wtime 1000 btime 1000 winc 0 binc 0 movestogo 10"):
            send(process, "go " + limit)
            read_until(output, lambda line: line.startswith("bestmove "), 5.0)

        send(process, "go infinite")
        read_until(output, lambda line: line.startswith("info depth "), 3.0)
        sent = time.monotonic()
        send(process, "stop")
        lines = read_until(output, lambda line: line.startswith("bestmove "), 1.0)
        latency_ms = (time.monotonic() - sent) * 1000
        if latency_ms >= 50:
            raise RuntimeError(f"stop took {latency_ms:.1f} ms (limit: <50 ms)")
        send(process, "bench 3")
        lines = read_until(output, lambda line: "bench depth 3 nodes" in line, 10.0)
        if not any("nodes 0 " not in line for line in lines):
            raise RuntimeError("UCI bench did not search any nodes")
        print(f"UCI, legal/tactical search, and stop: ok ({latency_ms:.1f} ms)")
    finally:
        if process.poll() is None:
            send(process, "quit")
            try:
                process.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        reader.join(timeout=1.0)


if __name__ == "__main__":
    main()
