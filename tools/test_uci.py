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
        if not any("option name KingSafetyV2 type check default true" in line
                   for line in uci_lines):
            raise RuntimeError("KingSafetyV2 UCI option is missing or not enabled by default")
        if not any("option name Move Overhead type spin default 10 min 0 max 5000" in line
                   for line in uci_lines):
            raise RuntimeError("Move Overhead UCI option is missing or has wrong bounds")
        send(process, "isready")
        read_until(output, lambda line: line == "readyok", 2.0)
        safety_fen = (
            "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R w KQkq - 0 1"
        )
        castled_fen = (
            "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R4RK1 w kq - 0 1"
        )
        send(process, "setoption name KingSafetyV2 value false")
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
            raise RuntimeError("KingSafetyV2=false did not restore the baseline evaluation")
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

        send(process, "setoption name Move Overhead value 10")
        clock_test_positions = {
            "white": {
                "command": "position startpos moves e2e4 e7e5",
                "fen": "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2",
            },
            "black": {
                "command": "position startpos moves e2e4 e7e5 g1f3",
                "fen": "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",
            },
        }
        legal_clock_moves = {}
        for side, position in clock_test_positions.items():
            legal_result = subprocess.run(
                [engine, "perft", "divide", "1", *shlex.split(position["fen"])],
                check=True, capture_output=True, text=True
            )
            legal_clock_moves[side] = {
                row.split(":", 1)[0]
                for row in legal_result.stdout.splitlines()
                if ":" in row
            }

        worst_clock_ms = 0.0
        clock_searches = 0
        for remaining in (50, 100, 150, 200, 300, 500):
            for increment in (0, 50):
                for side in ("white", "black"):
                    send(process, clock_test_positions[side]["command"])
                    go = (f"go wtime {remaining} btime {remaining} "
                          f"winc {increment} binc {increment} movestogo 20")
                    sent = time.monotonic()
                    send(process, go)
                    lines = read_until(
                        output, lambda line: line.startswith("bestmove "), 2.0
                    )
                    elapsed_ms = (time.monotonic() - sent) * 1000
                    worst_clock_ms = max(worst_clock_ms, elapsed_ms)
                    if elapsed_ms >= remaining:
                        raise RuntimeError(
                            f"{side} clock {remaining} ms + {increment} ms returned "
                            f"bestmove after {elapsed_ms:.1f} ms"
                        )
                    bestmove_line = next(
                        line for line in reversed(lines)
                        if line.startswith("bestmove ")
                    )
                    bestmove = bestmove_line.split()[1]
                    if bestmove not in legal_clock_moves[side]:
                        raise RuntimeError(
                            f"{side} clock search returned illegal move {bestmove}"
                        )
                    completed_pvs = []
                    for line in lines:
                        fields = line.split()
                        if (len(fields) >= 7 and fields[0] == "info" and
                                fields[1] == "depth" and fields[3] == "seldepth" and
                                "score" in fields and "pv" in fields):
                            pv_index = fields.index("pv")
                            if pv_index + 1 < len(fields):
                                completed_pvs.append(
                                    (int(fields[2]), fields[pv_index + 1])
                                )
                    if not any(depth >= 2 for depth, _ in completed_pvs):
                        raise RuntimeError(
                            f"{side} clock {remaining} ms + {increment} ms "
                            f"returned {bestmove} without a completed depth-2 info line; "
                            f"output: {lines}"
                        )
                    last_completed_pv = max(completed_pvs)[1]
                    if bestmove != last_completed_pv:
                        raise RuntimeError(
                            f"{side} clock {remaining} ms + {increment} ms returned "
                            f"{bestmove}, but latest completed PV starts with "
                            f"{last_completed_pv}"
                        )
                    clock_searches += 1

        send(process, "bench 3")
        lines = read_until(output, lambda line: "bench depth 3 nodes" in line, 10.0)
        if not any("nodes 0 " not in line for line in lines):
            raise RuntimeError("UCI bench did not search any nodes")
        print(
            "UCI, legal/tactical search, stop, and small clocks: ok "
            f"(stop {latency_ms:.1f} ms; worst small-clock response "
            f"{worst_clock_ms:.1f} ms; {clock_searches} timed searches "
            "completed at least depth 2)"
        )
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
