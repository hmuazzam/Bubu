# Catatan Desain

## Status scaffold

Repo memulai dengan program C++20 minimal agar alur build dapat diverifikasi
sebelum modul catur ditambahkan. `src/main.cpp` belum memuat aturan catur,
UCI, atau search.

## Prinsip

- Ikuti urutan task di Bagian 12 `PRD.md`.
- Utamakan korektness dan antarmuka modul yang kecil.
- Hindari state global mutable tersembunyi dan alokasi dinamis di hot path.
- Jaga build ringan dengan `make -j2`.

## Build

Makefile menyediakan build rilis, build ASan/UBSan, smoke test, dan target
bench. Target bench saat ini hanya penanda scaffold; belum ada node count.
