# 22–24 October 2026 — clean-room implementation bridge

This milestone adds conservative byte-level tooling that can be used with original or candidate game data without inventing undocumented field positions.

## Implemented
- `src/parity/BinaryParity.hpp`: binary first-difference reporting, strict file-size check, lossless raw snapshot serialization with explicit byte patching.
- `tests/binary_parity_test.cpp`: deterministic smoke tests for raw round trips, patching, length mismatch, first mismatch and indexed 320×200 framebuffer plus 768-byte DAC gate.
- `CMakeLists.txt`: CTest registration for the new module.

## Evidence boundary
The generic raw-byte snapshot is **not** a validated DOS/Windows USER.SAV parser or a converter. It cannot safely edit game state fields until versioned layouts are independently validated. The exact-frame gate only checks supplied matched buffers; it does not capture or synchronize the original emulator against the remake.

## Known observations to implement next
- DOS2 Ctrl FIRE 0x0080, Alt 0x0100 and Space USE 0x0200 (see reversed project V085/V086 original run).
- Original gameplay traces establish certain E1M1 rotation/movement and projectile lifecycle samples, not global behavior.
- Wall projection U stages passed bounded sampled-original checks; the active preview in the separate RE repository still uses diagnostic DDA.
- Save/load first frame, deterministic DEMO scheduler, projectile stale-depth, GUARD bosses and full indexed/DAC parity remain open.

See [research repository](https://github.com/marek177/Nitemare3d-reversed/blob/main/docs/RE_ALL_RUNTIME_STATIC_20261022_SK.md) for the source ledger and evidence grades.

## Reproduce
```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
