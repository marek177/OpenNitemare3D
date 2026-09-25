# Reverse-engineered Nite3W core findings (2026-09-25)

This isolated module mirrors currently confirmed reverse-engineering findings without replacing existing OpenNitemare3D gameplay code.

Included:
- GUARD/OBJECT/projectile/DEMO record layouts;
- runtime capacities and strides;
- recovered LCG;
- corrected score table;
- damage formula and class/weapon transforms;
- fire hazard damage;
- special GUARD categories;
- current MAP.1-3 placement table;
- machine-readable subsystem coverage/status table.

Placement totals: E1 375, E2 501, E3 390, playable total 1266; E1M11 demo adds 50.

The score baseline 145775 (147775 with Penelope penalties excluded) is analytical only. The true maximum score remains open because score is added through the damage path.

## Coverage tracking

Working subsystem coverage is kept separate from recovered behavior:

- `n3d_status.hpp` — machine-readable coverage/confidence table.
- `../../docs/NITE3W_RE_STATUS_2026_09_25.md` — human-readable status ledger.

The coverage values are working RE estimates, not proof of source-code equivalence. Production behavior should only be integrated after the relevant branch is confirmed by static/runtime evidence.

This directory is intentionally not wired into the production build yet. It is a reference/integration source until remaining UNKNOWN/INFERRED branches are closed.
