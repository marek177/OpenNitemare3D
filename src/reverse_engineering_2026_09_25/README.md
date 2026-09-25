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
- current MAP.1-3 placement table.

Placement totals: E1 375, E2 501, E3 390, playable total 1266; E1M11 demo adds 50.

The score baseline 145775 (147775 with Penelope penalties excluded) is analytical only. The true maximum score remains open because score is added through the damage path.

This directory is intentionally not wired into the production build yet. It is a reference/integration source until remaining UNKNOWN/INFERRED branches are closed.
