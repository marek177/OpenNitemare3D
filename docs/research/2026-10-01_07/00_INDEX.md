# Nitemare 3D Reverse Engineering — Weekly Research Pack

**Period:** October 1–7, 2026  
**Language:** English  
**Scope:** New discoveries, corrected assumptions, build-specific data, runtime evidence plans, and remaining gaps toward 1:1 behavioral/timing/rendering parity.

## Documents

1. [01 — Weekly Summary of New Findings](01_WEEKLY_SUMMARY.md)
2. [02 — Technical Appendix: Addresses, Data and Algorithms](02_TECHNICAL_APPENDIX_ADDRESSES_DATA.md)
3. [03 — Runtime Evidence and Experiments](03_RUNTIME_EVIDENCE_AND_EXPERIMENTS.md)
4. [04 — Open Gaps Toward 1:1 Parity](04_OPEN_GAPS_TO_1TO1.md)

## Confidence legend

- **CONFIRMED** — directly supported by disassembly/dumps or agreement between multiple artifacts.
- **STRONG** — very strongly supported statically, but a targeted runtime proof is still missing.
- **CORRECTION** — a new finding invalidates or refines an older hypothesis.
- **OPEN** — behavior/order is not yet closed by a 1:1 runtime test.

## Important rule

Addresses in these documents are **build-specific** unless explicitly stated otherwise. Do not reuse offsets between DOS/Win16 versions without fingerprinting the exact executable and mapping the corresponding symbols/functions.
