# 3 November 2026 — byte-exact frame and DAC comparison utility

This update implements `tools/n3d_frame_diff.py` and its isolated Python `unittest` regression tests. It does **not** add a rendering algorithm, an emulator capture backend or a validated gameplay parity result.

## Inputs and outputs

- Expected and actual indexed video memory snapshots: each exactly **320×200 = 64,000 bytes**.
- Expected and actual DAC snapshots: each exactly **256×3 = 768 bytes** in raw RGB6 component order.
- JSON result: independent pixel and DAC mismatch counts, first mismatch offset, frame (x,y) or palette (index, channel), SHA-256 fingerprints, size validation, RGB6-range diagnostics and `zero_diff`.
- A `parity_verified=false` field is intentionally **always** returned: a byte comparator cannot establish frame-state alignment, capture provenance or tick-exact execution.

## Reproduce

```bash
python3 tools/n3d_frame_diff.py \
  --expected-frame origin_frame.bin --expected-dac origin_dac.bin \
  --actual-frame remake_frame.bin --actual-dac remake_dac.bin \
  --json-out frame_report.json
python3 -m unittest discover -s tests -p 'test_n3d_frame_diff.py' -v
```

Exit codes: **0** byte-identical frame+DAC, **1** mismatch, **2** invalid size or input/output failure. The origin and remake filenames are examples; the tool does not decide which capture is genuine.

## Required original-vs-remake acceptance protocol

1. Capture original EXE version and SHA-256, map, input stream, seed, tick/frame index and relevant object/player state.
2. Capture exactly the corresponding remake state and indexed framebuffer+DAC (no palette normalization).
3. Run this tool and investigate the **first** divergence; repeat over a deterministic sequence.
4. Attribute whether mismatch originates in scheduler, object state, projection/clipping, material selection or palette updates.
5. Claim 1:1 pixel parity only for the specific matched captures that pass and were verified as synchronized. A single synthetic zero-diff test does not prove global 1:1 parity.

## Current limitation
`OpenNitemare3D` is a clean-room bootstrap, not a complete 1:1 engine. Renderer, scheduler, projectiles, DEMO, SAVE/LOAD first frame, GUARD bosses and Win16 runtime still need original evidence and integration. Research status is in [Nitemare3d-reversed](https://github.com/marek177/Nitemare3d-reversed/blob/main/docs/RE_NOV03_FUNCTION_CLOSURE_AND_TOOLING.md).
