#!/usr/bin/env python3
"""Check that the fixed-position, single-thread benchmark has stable nodes."""

import re
import subprocess
import sys


def run(engine):
    result = subprocess.run([engine, "bench"], check=True, capture_output=True, text=True)
    match = re.search(r"\bnodes (\d+)\b", result.stdout)
    if not match:
        raise RuntimeError(f"benchmark did not print a node count: {result.stdout}")
    return int(match.group(1)), result.stdout.strip()


def main():
    engine = sys.argv[1] if len(sys.argv) > 1 else "./bubu"
    first_nodes, first_output = run(engine)
    second_nodes, second_output = run(engine)
    if first_nodes != second_nodes:
        raise RuntimeError(f"node count changed: {first_nodes} != {second_nodes}")
    print(f"deterministic bench: {first_nodes} nodes ({first_output}; {second_output})")


if __name__ == "__main__":
    main()
