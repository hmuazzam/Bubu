#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
    echo "Usage: $0 <bubu-engine> <openings.pgn> <name=/path/to/engine>..." >&2
    exit 2
fi

bubu=$1
openings=$2
shift 2

if ! command -v cutechess-cli >/dev/null 2>&1; then
    echo "cutechess-cli is required for the gauntlet." >&2
    exit 127
fi
if [[ ! -e $bubu || ! -e $openings ]]; then
    echo "Engine or openings file does not exist." >&2
    exit 2
fi

engine_args=(-engine name=Bubu cmd="$bubu" proto=uci)
for opponent in "$@"; do
    if [[ $opponent != *=* ]]; then
        echo "Opponent must be NAME=/path/to/engine: $opponent" >&2
        exit 2
    fi
    name=${opponent%%=*}
    path=${opponent#*=}
    if [[ ! -e $path ]]; then
        echo "Opponent executable does not exist: $path" >&2
        exit 2
    fi
    engine_args+=(-engine "name=$name" "cmd=$path" proto=uci)
done

mkdir -p results
cutechess-cli "${engine_args[@]}" \
    -each tc=10+0.1 option.Hash=16 \
    -rounds 500 -games 2 -repeat -concurrency 2 \
    -openings file="$openings" format=pgn order=random \
    -pgnout results/gauntlet.pgn
