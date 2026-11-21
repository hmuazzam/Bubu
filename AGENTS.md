# Aturan untuk AI Agent

Aturan ini merangkum Bagian 16 `PRD.md`, yang tetap menjadi sumber kebenaran.

1. Baca `PRD.md` dan dokumen di `docs/` sebelum mengubah kode. Kerjakan satu
   task Bagian 12 per sesi dan ikuti urutannya.
2. Jangan melompati milestone atau menambah fitur di luar PRD tanpa persetujuan.
3. Tulis implementasi sendiri dari konsep umum. Jangan menyalin kode atau tabel
   evaluasi engine lain; catat konsep dan sumber idenya di `docs/SOURCES.md`.
4. Utamakan tes, lalu implementasi. Jalankan `make test` dan `make bench` untuk
   task yang relevan, dan commit dengan Conventional Commits.
5. Sertakan `Bench: <nodes>` pada pesan commit bila search atau evaluasi berubah.
6. Perubahan search/eval harus disertai hasil SPRT atau alasan yang jelas.
7. Jelaskan alasan untuk kode non-trivial dalam komentar dan ringkasan akhir
   yang mudah dipahami pemula.
8. Batasi kerja lokal sesuai perangkat: `make -j2`, Hash kecil, satu thread.
9. Jangan melemahkan tes demi hasil hijau. Jika kegagalan berulang tidak dapat
   diperbaiki, berhenti dan laporkan percobaan serta hasilnya.
10. Jangan mengarang hasil perft, bench, Elo, atau turnamen; laporkan hanya
    hasil yang benar-benar diperoleh.
