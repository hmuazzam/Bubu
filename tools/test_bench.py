#!/usr/bin/env python3
"""Check that the fixed-position, single-thread benchmark has stable nodes."""

import re
import subprocess
import sys

DEFAULT_NODES = 42654
DISABLED_NODES = 47626


def run(engine):
    result = subprocess.run([engine, "bench"], check=True, capture_output=True, text=True)
    match = re.search(r"\bnodes (\d+)\b", result.stdout)
    if not match:
        raise RuntimeError(f"benchmark did not print a node count: {result.stdout}")
    return int(match.group(1)), result.stdout.strip()


def run_uci_bench(engine, enabled):
    process = subprocess.Popen(
        [engine], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, bufsize=1
    )
    try:
        process.stdin.write("uci\n")
        process.stdin.flush()
        for line in process.stdout:
            if line.strip() == "uciok":
                break
        else:
            raise RuntimeError("engine exited before completing UCI handshake")

        process.stdin.write(f"setoption name KingSafetyV2 value {str(enabled).lower()}\n")
        process.stdin.write("bench\n")
        process.stdin.flush()
        for line in process.stdout:
            match = re.search(r"\bnodes (\d+)\b", line)
            if match:
                nodes = int(match.group(1))
                process.stdin.write("quit\n")
                process.stdin.flush()
                process.wait(timeout=2)
                return nodes, line.strip()
        raise RuntimeError("UCI bench did not print a node count")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def main():
    engine = sys.argv[1] if len(sys.argv) > 1 else "./bubu"
    first_nodes, first_output = run(engine)
    second_nodes, second_output = run(engine)
    if first_nodes != second_nodes:
        raise RuntimeError(f"node count changed: {first_nodes} != {second_nodes}")
    if first_nodes != DEFAULT_NODES:
        raise RuntimeError(f"default KingSafetyV2 bench changed: {first_nodes} != {DEFAULT_NODES}")

    disabled_first, disabled_output = run_uci_bench(engine, False)
    disabled_second, disabled_repeat = run_uci_bench(engine, False)
    if disabled_first != disabled_second or disabled_first != DISABLED_NODES:
        raise RuntimeError(
            "KingSafetyV2=false bench changed: "
            f"{disabled_first}, {disabled_second}; expected {DISABLED_NODES}"
        )
    print(
        f"deterministic bench: default {first_nodes} nodes "
        f"({first_output}; {second_output}); false {disabled_first} nodes "
        f"({disabled_output}; {disabled_repeat})"
    )


if __name__ == "__main__":
    main()
