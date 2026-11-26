# Agent Instructions

These rules define the project's agent workflow. The README and documents in
`docs/` describe the current project scope and design.

1. Read this file and the relevant documents in `docs/` before changing code.
   Work on one user-authorized task at a time.
2. Do not expand the requested scope or add unrelated features without approval.
3. Write original implementations from general concepts. Do not copy engine
   code or evaluation tables. Record concepts and their sources in
   `docs/SOURCES.md`.
4. Prefer tests before implementation. Run `make test` and `make bench` when
   relevant, then commit with a Conventional Commit message.
5. Include `Bench: <nodes>` in commits that change search or evaluation.
6. Search and evaluation changes need SPRT results or a clear reason why SPRT
   could not be run.
7. Explain non-trivial code and finish with a beginner-friendly summary.
8. Keep local work within the machine's limits: use `make -j2`, a small hash,
   and one search thread.
9. Do not weaken tests to make them pass. If a failure remains after reasonable
   attempts, stop and report what was tried.
10. Never invent perft, benchmark, Elo, or tournament results. Report only
    measurements that were actually run.
