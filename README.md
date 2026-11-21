# Bubu Chess Engine

Bubu adalah proyek engine catur UCI open source dari Indonesia, ditulis dari
awal dengan C++20. Fokus pengembangan adalah kode yang mudah diperiksa,
korektness yang teruji, dan hasil kekuatan yang diukur secara terbuka.

## Status

Repo saat ini berada pada milestone M0 (setup). Program hanya menampilkan nama
Bubu; protokol UCI dan fitur engine akan dibuat pada task berikutnya sesuai
urutan di `PRD.md`.

## Build dan pemeriksaan

```sh
make -j2
./bubu
make test
make bench
make debug
```

Build rilis menghasilkan `./bubu`. Target `test` saat ini hanya memeriksa bahwa
scaffold dapat dijalankan. Target `bench` memberi tahu bahwa pengukuran node
belum tersedia sampai search dibuat. Keduanya akan berkembang seiring task di
roadmap.

## Identitas visual

Logo Bubu: placeholder. Maskot yang direncanakan adalah kucing abu-abu dengan
desain orisinal.

## Lisensi

GPL-3.0; lihat [`LICENSE`](LICENSE).
