# DOS v2 DEMO reader: evidence-bounded regression (2026-10-26)

Provenance: [original runtime report](https://github.com/marek177/Nitemare3d-reversed/blob/codex/re-function-evidence-20261025/docs/evidence/2026-10-25/RNG_DEMO_EOF_report.md), [PR #69](https://github.com/marek177/Nitemare3d-reversed/pull/69), and [read-only verifier](https://github.com/marek177/Nitemare3d-reversed/blob/codex/re-function-evidence-20261025/tools/verify_rng_demo_ledger.py).

## First concrete implementation divergence

At OpenNitemare3D/main @ 1668039e5, `src/formats/DemoFile.cpp` decoded a record as one byte event, two bytes input mask and one padding byte. The original hash-pinned DEMO.1 verifier decodes each record as little-endian `<HHI` (16-bit key, 16-bit flags, 32-bit render counter), following a six-byte `<HHH` header. This is a reproducible *format* divergence, visible from the very first original record `key=0x48E0, flags=0x0002, counter=8`. The decoder now uses the evidenced layout.

## Bounded model and tests

`DemoReaderState::applyDue` models one due application and the subsequent read result: 8 bytes when the next record is prefetched; zero on attempted prefetch beyond the last record. The final pending record and applied key/flags remain unchanged by EOF. The reader does not encode mode termination, BIOS interrupt behavior, or caller-specific scheduling. The older `dispatchOneDueRecordAtGeneration` remains explicitly a reconstruction primitive rather than a completed engine scheduler.

`demo_eof_regression_test` uses invented fixtures (none of the commercial DEMO payload). It checks 16-bit key/flag positions, the three header words, first delayed application, successful next-record prefetch, zero-byte first EOF, retained last input and prefetched record, and the 275-record count/ordering invariant. The EOF key 0x74E0 and flags 0x0090 are *recorded original case* values; this synthetic test only shows that the interface can preserve those values, **not** that a remake executed the original schedule.

Original ledger summary: 734 events, 229 completed RNG calls, 275 applied inputs, first EOF at observed render counter 1074, baseline RNG seed 1, final RNG state 0xD0643564. 72 RNG observations matched a separate interrupted run. This repository lacks the private ordered 734-event raw trace and original DEMO bytes; tests do not assert that these 229 RNG calls or all 275 original keys/times have been matched against a remake. Run the independent verifier with its private inputs for that stronger evidence gate; the current test does not replace it.

## Not implemented / open

- Gameplay scheduler order, nested callbacks, interrupts, RNG call order and a paired 229-return trace.
- The 32-event original EOF-to-`06B0:039C` control-flow continuation is **reported evidence**, not a verified control-flow path of this bootstrap. The local API returns to its C++ caller; no equivalent FAR stack frame or SP +12 is claimed.
- Any repeated post-EOF read behavior and eventual game-mode end beyond the separately measured local reader return.
- Whole-game, DEMO, renderer and DOS/Win16 parity.

Reproduce:
```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
