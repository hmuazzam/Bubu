#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
    echo "Usage: $0 <candidate-engine> <baseline-engine> <openings.pgn> [games-per-round]" >&2
    exit 2
fi

candidate=$1
baseline=$2
openings=$3
games_per_round=${4:-2}

if ! command -v cutechess-cli >/dev/null 2>&1; then
    echo "cutechess-cli is required for SPRT." >&2
    exit 127
fi
for file in "$candidate" "$baseline" "$openings"; do
    if [[ ! -e $file ]]; then
        echo "Required path does not exist: $file" >&2
        exit 2
    fi
done

mkdir -p results
cutechess-cli \
    -engine name=Bubu cmd="$candidate" proto=uci \
    -engine name=Baseline cmd="$baseline" proto=uci \
    -each tc=10+0.1 option.Hash=16 \
    -games "$games_per_round" -rounds 100000 -repeat -concurrency 2 \
    -openings file="$openings" format=pgn order=random \
    -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
    -pgnout results/sprt.pgn
