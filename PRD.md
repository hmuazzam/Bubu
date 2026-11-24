# PRD: Bubu Chess Engine 🐱

> Status: Draft v1.0 | Owner: (nama kamu) | Bahasa dokumen: Indonesia + istilah teknis Inggris
> Dokumen ini adalah **single source of truth** untuk manusia dan AI Agent.

---

## 1. Ringkasan

**Bubu** adalah chess engine (UCI) open source buatan Indonesia, ditulis dari nol dalam **C++20**. Maskot: kucing abu-abu (terinspirasi kucing pemilik proyek). Proyek ini dirancang serius dan jangka panjang.

**Arah strategis:**
1. **MVP (v0.1):** engine klasik (tanpa neural network) dengan algoritma search modern, target **>= 2300 Elo** (skala CCRL-style, diukur lewat turnamen engine vs engine).
2. **Pasca-MVP:** donasi server, NNUE, target **3000+ Elo**.
3. **Jangka panjang:** rating list (CCRL), Lichess bot, turnamen (WCCC/ICGA dan lainnya; syarat dicek langsung ke situs penyelenggara).

## 2. Latar Belakang dan Masalah

- Belum ada engine catur Indonesia yang dikenal di komunitas engine.
- Pemilik proyek adalah *vibe coder* dengan laptop **4 GB RAM**: pengembangan harus ringan, terstruktur, dan **dapat diverifikasi otomatis** karena sebagian besar kode ditulis AI.
- Risiko utama: kode AI yang tampak benar tapi punya bug tersembunyi, dan engine dianggap *derivative* (salinan engine lain).

## 3. Tujuan dan Non-Tujuan

### Tujuan (MVP)
| ID | Tujuan |
|----|--------|
| G1 | Move generator 100% benar (lulus perft). |
| G2 | Engine kompatibel UCI penuh dengan GUI umum (Cute Chess, Arena, Lichess-bot). |
| G3 | Kekuatan >= 2300 Elo, dibuktikan turnamen minimal 1.000 game vs engine referensi. |
| G4 | Setiap perubahan algoritma terbukti lewat SPRT/bench, bukan perasaan. |
| G5 | Kode orisinal, terdokumentasi sumber idenya, siap diaudit untuk turnamen. |
| G6 | Rilis publik v0.1.0 (binary Windows + Linux) dan bot Lichess. |

### Non-Tujuan (MVP)
- NNUE / neural network (masuk Fase pasca-MVP).
- GUI sendiri, opening book besar, endgame tablebase (Syzygy) , multi-threading (Lazy SMP) .
- Mendukung varian catur (Chess960 boleh ditunda).

## 4. Pengguna

| Persona | Kebutuhan |
|---------|-----------|
| Pemilik/dev (vibe coder) | Alur kerja jelas, tes otomatis, bug cepat ketahuan. |
| Pemain/komunitas catur Indonesia | Bisa main melawan Bubu di Lichess atau GUI lokal. |
| Pengguna GUI/turnamen engine | Binary UCI stabil, rating terukur. |
| Calon donatur/kontributor | Repo rapi, progres Elo transparan, cerita menarik. |

## 5. Metrik Keberhasilan

| Metrik | Target MVP |
|--------|------------|
| Perft | 100% cocok pada seluruh posisi uji di `tests/perft.epd` |
| Langkah ilegal / crash dalam 1.000 game | 0 |
| Kekuatan | >= 2300 Elo (lihat Bagian 10) |
| Kecepatan | >= 1 Mnps (juta node/detik) pada 1 thread (indikatif, tergantung CPU) |
| Bench | Deterministik: node count identik antar run |
| CI | Hijau (build + perft + bench + sanitizer) |

> **Catatan jujur soal Elo:** engine dengan fitur Fase 3 di bawah umumnya melampaui 2300. Angka 2300 dipakai sebagai *gate minimum*, bukan batas atas. Skala Elo engine bukan Elo manusia (Lichess/Chess.com); ukur selalu relatif terhadap engine referensi.

---

## 6. Tech Stack

| Komponen | Pilihan | Alasan |
|----------|---------|--------|
| Bahasa | **C++20** | Cepat, referensi AI melimpah, tanpa dependency |
| Compiler | **g++ >= 11** atau **clang >= 14** | Ringan untuk RAM 4 GB |
| Build | **Makefile** (CMake opsional nanti) | Sederhana, mudah di-debug AI |
| Dependency runtime | **Tidak ada** (hanya standard library) | Portabel, aman untuk turnamen |
| Flag rilis | `-O3 -DNDEBUG -march=x86-64-v2` (varian `v3` / popcnt, bmi2 dipisah) | Kompatibilitas luas |
| Flag debug | `-O1 -g -fsanitize=address,undefined` | Tangkap bug memori |
| Version control | **Git + GitHub** | Wajib sejak hari 1 |
| CI | **GitHub Actions** | Build, perft, bench otomatis |
| Test engine vs engine | **Cute Chess** (`cutechess-cli`) atau **fastchess** | SPRT dan gauntlet |
| Script bantu | **Python 3** (tuning, analisis hasil) | Hanya untuk tooling, bukan engine |
| Lichess bot | **lichess-bot** (proyek komunitas) | Menjembatani UCI ke Lichess |
| Pasca-MVP | Trainer NNUE (mis. **bullet**), **OpenBench**, GPU sewaan | Fase 4 |
| Lisensi | **GPL-3.0** (default, ubah jika ada alasan) | Norma komunitas engine |

**Batasan perangkat dev (laptop 4 GB RAM):** `make -j2`, Hash default 16 MB (maks uji 64 MB), `Threads=1`, concurrency turnamen 1-2, tutup browser saat compile/tes. Tes berat dijalankan di CI atau mesin lain.

---

## 7. Arsitektur dan Struktur Repo

```
bubu/
  src/
    types.h          # Square, Piece, Color, Move, Score, konstanta
    bitboard.h/.cpp  # operasi bitboard, tabel attack, magic bitboards
    position.h/.cpp  # state papan, FEN, make/unmake, Zobrist, repetisi
    movegen.h/.cpp   # generator legal (staged: capture, quiet, evasion)
    movepick.h/.cpp  # move ordering bertahap
    eval.h/.cpp      # evaluasi tapered (mg/eg)
    search.h/.cpp    # PVS, ID, qsearch, pruning/reduction
    tt.h/.cpp        # transposition table
    timeman.h/.cpp   # manajemen waktu
    uci.h/.cpp       # protokol UCI
    bench.cpp        # perintah bench
    main.cpp
  tests/
    perft.epd        # posisi + hasil perft
    tactics.epd      # posisi taktik sanity check
    run_perft.sh
  tools/
    sprt.sh          # wrapper cutechess-cli
    texel_tune.py    # tuning evaluasi
    openings/        # book pembuka acak (EPD/PGN) untuk tes
  docs/
    DESIGN.md        # keputusan desain
    SOURCES.md       # daftar konsep + sumber ide (anti-derivative)
    ELO_LOG.md       # log hasil tes tiap versi
  assets/            # logo Bubu
  .github/workflows/ci.yml
  AGENTS.md          # aturan untuk AI Agent (isi = ringkasan Bagian 12)
  Makefile
  README.md
  LICENSE
```

**Prinsip desain:**
- Satu modul = satu tanggung jawab; antarmuka kecil dan jelas.
- Tidak ada global mutable state tersembunyi; `Searcher` memegang state per-thread (siap untuk Lazy SMP kelak).
- Semua parameter yang bisa di-tune diletakkan di satu tempat (`params.h`/konstanta eksplisit) agar mudah di-tuning.
- Tidak ada alokasi dinamis di hot path search.

---

## 8. Kebutuhan Fungsional

### 8.1 Board dan Move Generation
| ID | Requirement |
|----|-------------|
| F-BRD-1 | Representasi bitboard 12 piece + array mailbox untuk lookup cepat. |
| F-BRD-2 | Parser dan serializer FEN yang valid, menolak FEN rusak. |
| F-BRD-3 | `make_move` / `unmake_move` inkremental (castling rights, ep, 50-move, Zobrist key, checkers/pins). |
| F-BRD-4 | Zobrist hashing 64-bit (PRNG deterministik dengan seed tetap). |
| F-GEN-1 | Generator **legal** lengkap: castling, en passant, promosi (4 jenis), pinned pieces, check evasion. |
| F-GEN-2 | **Magic bitboards** (atau PEXT di build `bmi2`) untuk sliding pieces. |
| F-GEN-3 | Mode generator terpisah: captures/promosi, quiet, evasions (dasar staged move picker dan qsearch). |
| F-GEN-4 | Perintah CLI `perft <depth>` dan `perft divide <depth>`. |

### 8.2 UCI
| ID | Requirement |
|----|-------------|
| F-UCI-1 | Perintah: `uci`, `isready`, `ucinewgame`, `position (startpos/fen) moves ...`, `go`, `stop`, `quit`, `setoption`. |
| F-UCI-2 | `go` mendukung: `depth`, `nodes`, `movetime`, `wtime/btime/winc/binc/movestogo`, `infinite`. |
| F-UCI-3 | Output `info depth seldepth score cp/mate nodes nps time hashfull pv` tiap iterasi. |
| F-UCI-4 | Opsi: `Hash` (MB, default 16, min 1, maks 1024), `Threads` (default 1, dikunci 1 di MVP), `Clear Hash`. |
| F-UCI-5 | Search berjalan di thread terpisah agar `stop` responsif. |
| F-UCI-6 | Perintah non-standar: `bench [depth]`, `d` (cetak papan), `eval`. |

### 8.3 Search (algoritma terbaik untuk skala 2000-2800 tanpa NN)
| ID | Fitur | Catatan |
|----|-------|---------|
| F-SRC-1 | Negamax + **Principal Variation Search (PVS)** | Dasar |
| F-SRC-2 | **Iterative deepening** + **aspiration windows** | Window kecil, widening bertahap |
| F-SRC-3 | **Quiescence search** (captures, promosi, evasion saat skak), pruning SEE-negatif dan delta pruning | Hindari horizon effect |
| F-SRC-4 | **Transposition table** (Zobrist, bucket/replacement scheme sederhana, aging generasi) | Skor mate disesuaikan jarak ke root |
| F-SRC-5 | **Move ordering bertahap**: TT move, capture bagus (SEE >= 0, MVV-LVA), killer 2 slot, countermove, **history heuristic** (butterfly), quiet lainnya, capture buruk | Kunci kekuatan |
| F-SRC-6 | **Null move pruning** (R adaptif depth, hindari zugzwang: butuh non-pawn material) | |
| F-SRC-7 | **Late Move Reductions (LMR)** dengan tabel log(depth)*log(movecount), disesuaikan PV/history/improving | |
| F-SRC-8 | **Reverse futility pruning** (static null move) dan **futility pruning** | |
| F-SRC-9 | **Late Move Pruning** (move count pruning) pada depth rendah | |
| F-SRC-10 | **Check extension**; **Internal Iterative Reduction** saat tidak ada TT move | |
| F-SRC-11 | **SEE** (static exchange evaluation) untuk ordering dan pruning | |
| F-SRC-12 | Deteksi remis: repetisi (termasuk dalam search), aturan 50 langkah, insufficient material | |
| F-SRC-13 | Mate distance pruning | |
| F-SRC-14 | **Time management**: soft/hard limit, alokasi berdasarkan sisa waktu + increment, perpanjangan saat skor turun / best move berubah | |

> Fitur Lazy SMP, singular extensions, probcut, countermove history dan lainnya = **kandidat pasca-MVP**, ditambah hanya bila terbukti lewat SPRT.

### 8.4 Evaluasi (klasik, tapered)
| ID | Requirement |
|----|-------------|
| F-EVL-1 | Material + **piece-square tables** (mg & eg) dengan interpolasi fase permainan. |
| F-EVL-2 | Pawn structure: doubled, isolated, backward, passed pawn (bonus tergantung rank), connected/phalanx. |
| F-EVL-3 | Mobility per piece, bishop pair, rook di open/semi-open file, rook di rank 7. |
| F-EVL-4 | King safety: pawn shield, serangan di king zone, penalti open file dekat king. |
| F-EVL-5 | Tempo bonus; skala endgame sederhana (mis. dorong remis untuk material minim). |
| F-EVL-6 | **Semua parameter ditune dengan Texel tuning dari data self-play Bubu sendiri** (bukan menyalin tabel engine lain). |
| F-EVL-7 | Pawn hash table (opsional, hanya jika terbukti lebih cepat/kuat). |

### 8.5 Tooling dan Kualitas
| ID | Requirement |
|----|-------------|
| F-TST-1 | `make test`: perft suite, tactics suite, unit test FEN/make-unmake. |
| F-TST-2 | `bench`: search posisi tetap pada depth tetap, mencetak `nodes` dan `nps`; node count jadi **signature** (berubah hanya jika perubahan disengaja dan dicatat di commit). |
| F-TST-3 | CI: build (g++ & clang), perft, bench, build sanitizer. |
| F-TST-4 | `tools/sprt.sh`: SPRT otomatis (mis. `elo0=0 elo1=5, alpha=beta=0.05`, 10+0.1, opening acak). |

---

## 9. Kebutuhan Non-Fungsional

- **Korektness > kecepatan > fitur.** Jangan optimasi sebelum perft hijau.
- **Determinisme:** `bench` identik antar run (Threads=1).
- **Memori:** engine berjalan baik dengan Hash 16 MB; tidak ada leak (diperiksa ASan).
- **Portabilitas:** Windows (MinGW), Linux; tanpa dependency eksternal.
- **Keterbacaan:** fungsi pendek, nama jelas, komentar menjelaskan *kenapa* (bukan sekadar *apa*).
- **Orisinalitas:** tidak menyalin kode engine lain; tiap konsep dicatat di `docs/SOURCES.md`.

---

## 10. Strategi Pengukuran Kekuatan

1. **Tidak ada klaim Elo tanpa data.** Semua angka masuk `docs/ELO_LOG.md` (versi, jumlah game, W/D/L, TC, engine lawan, hasil Elo +/- error).
2. **Anchor/engine referensi:** pilih 4-6 engine open source dengan rating CCRL yang diketahui di sekitar 2000-2700 (cek rating terbaru di situs CCRL). Opsi tambahan: Stockfish dengan `UCI_LimitStrength` + `UCI_Elo` (rentang 1320+; skala kira-kira dikalibrasi ke CCRL, tapi hanya sebagai perkiraan kasar).
3. **Gauntlet:** Bubu vs tiap referensi, minimal 500 game per lawan, 10+0.1, Hash 16 MB, 1 thread, book opening acak, `-concurrency 2` di laptop.
4. **Regresi:** setiap perubahan search/eval lewat SPRT vs versi sebelumnya (bukan hanya vs engine luar).
5. **Gate MVP:** hasil gauntlet agregat (Elo terhitung dengan Ordo/BayesElo/ratingstool) >= **2300** dengan error margin yang dilaporkan.

---

## 11. Roadmap, Milestone, dan Acceptance Criteria

> Estimasi untuk 1 orang + AI Agent, ~10-15 jam/minggu. Urutan **tidak boleh dilompati**.

### M0 - Setup (Minggu 1)
- [ ] Repo, struktur folder, `Makefile` (target: `release`, `debug`, `test`, `bench`), `.gitignore`, LICENSE, README (visi + logo placeholder).
- [ ] CI GitHub Actions: build g++/clang.
- [ ] `docs/SOURCES.md`, `docs/DESIGN.md`, `docs/ELO_LOG.md` kosong berstruktur.
- **Done:** `make` sukses, CI hijau, `./bubu` jalan (cetak nama).

### M1 - Board dan FEN (Minggu 1-2)
- Tipe dasar, bitboard util, struct `Position`, parser FEN, Zobrist, `make/unmake`.
- **Done:** round-trip FEN untuk >= 50 posisi; hash inkremental == hash dihitung ulang (assert di debug).

### M2 - Move Generation dan Perft (Minggu 2-3)
- Attack tables, magic bitboards, generator legal, `perft`.
- **Done:** semua posisi berikut cocok (verifikasi ulang terhadap Chess Programming Wiki "Perft Results"):

| Posisi | Depth | Nodes |
|--------|-------|-------|
| Startpos | 5 | 4.865.609 |
| Kiwipete `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -` | 4 | 4.085.603 |
| Pos 3 `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -` | 5 | 674.624 |
| Pos 4 `r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1` | 4 | 422.333 |
| Pos 5 `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8` | 4 | 2.103.487 |
| Pos 6 `r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10` | 4 | 3.894.594 |

  Tambahan: lulus perft pada >= 100 posisi dari suite perft publik. Lulus ASan/UBSan.

### M3 - UCI dan Game Pertama (Minggu 3-4)
- UCI penuh (F-UCI-*), search sederhana (negamax depth tetap, eval material) hanya sebagai kerangka.
- **Done:** Bubu main 100 game vs random-mover di Cute Chess: 0 langkah ilegal, 0 crash, `stop` responsif.

### M4 - Search Inti (Minggu 4-6) -> ~1900-2200 Elo
- PVS, ID, qsearch, TT, move ordering bertahap, SEE, deteksi remis, time management, eval PST tapered.
- **Done:** `bench` stabil; SPRT menunjukkan tiap komponen (TT, ordering, qsearch) memberi gain positif; gauntlet awal tercatat di ELO_LOG.

### M5 - Pruning/Reduction dan Eval Kaya (Minggu 6-9) -> >= 2300 Elo (MVP)
- Aspiration, NMP, LMR, RFP, futility, LMP, check extension, IIR, mate distance pruning.
- Eval: pawn structure, mobility, king safety, passed pawns, bishop pair, rook files.
- **Done:** tiap fitur lolos SPRT (atau dibuang); gauntlet resmi >= 2300 sesuai Bagian 10.

### M6 - Tuning, Rilis, dan Publik (Minggu 9-11)
- Generator data self-play (`tools/` + mode engine), **Texel tuning** parameter eval (+ tuning konstanta search kasar secara manual/SPSA ringan).
- Rilis **v0.1.0** (binary Windows/Linux, README final, logo kucing abu), Lichess BOT account, pengajuan ke CCRL.
- **Done:** release GitHub, Bubu bisa dimainkan di Lichess, ELO_LOG berisi hasil final.

### M7 - Pasca-MVP (di luar scope PRD ini, rencana awal)
- Lazy SMP, `datagen` skala besar, NNUE (arsitektur kecil -> besar), OpenBench, donasi (Trakteer/Saweria/GitHub Sponsors), target 3000+ Elo, lalu turnamen.

---

## 12. Daftar Task Atomik untuk AI Agent (urutan eksekusi)

Format: `T-<milestone>-<no>`. Satu task = satu sesi = satu commit. Selesai hanya jika **kriteria Done terpenuhi dan tes hijau**.

| Task | Deskripsi | Verifikasi |
|------|-----------|-----------|
| T0-1 | Scaffold repo + Makefile + CI | `make`, CI hijau |
| T1-1 | `types.h`, `bitboard.h/.cpp` (util, popcount, lsb, shift) | unit test |
| T1-2 | `Position` + FEN parse/serialize | round-trip 50 FEN |
| T1-3 | Zobrist + `make/unmake` | hash inkremental == recompute |
| T2-1 | Attack tables non-slider (knight, king, pawn) | unit test |
| T2-2 | Magic bitboards (slider) | cocok dengan generator naif untuk semua kotak/occupancy sampel |
| T2-3 | Generator pseudo-legal -> legal (pin, check, evasion) | perft tabel di M2 |
| T2-4 | `perft` / `perft divide` CLI + `tests/perft.epd` runner | `make test` hijau |
| T3-1 | UCI loop + `position` + `go depth` | main game di Cute Chess |
| T3-2 | Search thread + `stop` + time (`movetime`, clock) | respons < 50 ms |
| T4-1 | Negamax + alpha-beta + ID + eval material | bench |
| T4-2 | Quiescence | SPRT vs T4-1 |
| T4-3 | TT | SPRT |
| T4-4 | Move picker (TT, MVV-LVA, killer, history) | SPRT |
| T4-5 | SEE + capture ordering/prune | SPRT |
| T4-6 | PVS + draw detection + mate scores | SPRT |
| T4-7 | Eval tapered PST (parameter awal sederhana) | SPRT |
| T4-8 | Time management matang | tes flag/lost-on-time = 0 |
| T5-1 | Aspiration windows | SPRT |
| T5-2 | Null move pruning | SPRT |
| T5-3 | LMR | SPRT |
| T5-4 | RFP + futility + LMP | SPRT |
| T5-5 | Check extension + IIR + mate distance pruning | SPRT |
| T5-6 | Eval: pawn structure | SPRT |
| T5-7 | Eval: mobility + bishop pair + rook files | SPRT |
| T5-8 | Eval: king safety | SPRT |
| T5-9 | Gauntlet vs engine referensi, catat Elo | ELO_LOG |
| T6-1 | Mode data generation (self-play) | file posisi + hasil |
| T6-2 | `texel_tune.py` dan terapkan parameter | SPRT vs sebelum tuning |
| T6-3 | Rilis v0.1.0 + README + Lichess bot | release publik |

---

## 13. Strategi Testing

| Lapisan | Alat | Kapan |
|---------|------|-------|
| Unit test | test runner sederhana (assert/CTest opsional) | tiap commit |
| Perft | `make test` | tiap perubahan board/movegen |
| Bench signature | `./bubu bench` | tiap commit; perubahan node count harus dijelaskan di pesan commit (`Bench: <nodes>`) |
| Sanitizer | build ASan+UBSan | sebelum merge dan nightly CI |
| Fungsional (UCI) | Cute Chess game vs engine lain | tiap milestone |
| Kekuatan | SPRT/gauntlet | tiap perubahan search/eval |

**Aturan SPRT default:** 10+0.1, Hash 16 MB, 1 thread, opening acak (book netral), bounds `[0, 5]` untuk gain atau `[-3, 1]` untuk simplifikasi, alpha=beta=0.05.

---

## 14. Risiko dan Mitigasi

| Risiko | Dampak | Mitigasi |
|--------|--------|----------|
| Bug halus dari kode AI | Sangat tinggi | Perft, bench signature, SPRT, sanitizer, review manual tiap modul |
| Dianggap derivative | Tinggi | Tulis dari konsep, SOURCES.md, parameter di-tune sendiri, tidak menyalin tabel/kode |
| Laptop 4 GB kewalahan | Sedang | `-j2`, Hash kecil, CI untuk tes berat, training di cloud |
| Scope creep | Tinggi | Non-tujuan jelas, fitur baru hanya via SPRT |
| Hilang motivasi | Tinggi | Milestone kecil, rilis publik cepat, ELO_LOG sebagai progres visible |
| Nama "Bubu" bentrok | Rendah-sedang | Cek CCRL, GitHub, Lichess sebelum rilis; siapkan nama cadangan |
| Hasil tes tidak akurat (noise) | Sedang | Jumlah game cukup, opening acak, laporkan error margin |

---

## 15. Open Questions (butuh keputusan pemilik)

1. Lisensi final: GPL-3.0 atau MIT?
2. Sistem operasi laptop: Windows (MinGW/MSYS2/WSL2) atau Linux?
3. Nama final dan cek ketersediaan "Bubu"; siapa pembuat logo (kucing abu mirip Mittens: hati-hati hak cipta jika meniru karakter spesifik; sebaiknya buat ilustrasi orisinal kucing sendiri).
4. Engine referensi mana yang dipakai (tentukan setelah cek daftar CCRL terbaru).
5. Platform donasi pilihan.

---

## 16. Aturan Kerja AI Agent (ringkasan untuk `AGENTS.md`)

1. Baca `PRD.md` dan `docs/` sebelum menulis kode; kerjakan **satu task** (Bagian 12) per sesi.
2. Jangan lompat milestone. Jangan menambah fitur di luar PRD tanpa persetujuan.
3. **Tulis kode orisinal** dari konsep. Dilarang menyalin kode/tabel dari engine lain. Catat konsep dan sumber di `docs/SOURCES.md`.
4. Setiap task: tulis tes -> implementasi -> jalankan `make test` dan `bench` -> commit.
5. Pesan commit mengikuti Conventional Commits dan menyertakan `Bench: <nodes>` bila search/eval berubah.
6. Jangan ubah perilaku engine secara diam-diam: perubahan search/eval harus menyertakan hasil SPRT atau alasan jelas.
7. Jelaskan kode non-trivial dalam komentar dan ringkasan akhir agar pemilik (vibe coder) paham.
8. Hormati batasan perangkat: `make -j2`, Hash kecil saat tes lokal.
9. Jika ragu atau tes gagal berulang, **berhenti dan laporkan**, jangan menebak atau menonaktifkan tes.
