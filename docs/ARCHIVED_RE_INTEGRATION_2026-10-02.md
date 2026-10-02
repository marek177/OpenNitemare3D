# Archived research reconciliation for the C# implementation

This pass uses accessible archived-conversation retrieval plus the preserved
cross-session ledger in Nitemare3d-reversed (d2be7fb09d6b1b6e0404517b603b306c8f1096db).
It is not an exhaustive export of every archived chat, or a new original-EXE audit.

## Sources consulted

- `docs/CROSS_SESSION_MASTER_INDEX.md`: accumulated findings and superseded claims.
- `docs/ALL_THREADS_CONSOLIDATION_2026-09-22.md`: canonical conversation consolidation.
- `docs/CHAT_RECOVERY_2026-09-26.md`: recovery limits and evidence policy.
- `docs/USE_INTERACTION_RE.md`: seg3:98ED..9AA5 USE edge latch; seg3:1A22
  and DS:009A cardinal target table `[-64,+1,+1,+64,+64,-1,-1,-64]`.
- `src/game/InventoryRuntime.hpp` and
  `analysis/nite3w_user_sav_story_flags_2026-09-23.md`: SECRET panel red ID-card bit 0.

## Implemented from the preserved findings

`RecoveredUseRuntime` translates rising-edge USE and the octant-to-cardinal-cell
table into C#. Player converts its modern radian angle to a N-first octant and
uses that helper, dispatching to both Tile.OnUse and entities in the selected cell.
The angle adapter is a modern integration choice, not a recovered original angle
conversion. The target helper checks each coordinate before indexing, avoiding
row wrapping at the 64x64 boundary.

HiddenPanel now checks the recovered red card credential and marks itself opened
before calling its neighbor. The one-time latch is a modern recursion fix: two
linked panels previously called each other without a stopping condition.
Existing instant wall removal remains a simplified animation implementation.

The earlier inventory/MAP/DEMO/health translations are detailed in
CPP_TO_CSHARP_2026-10-02.md. USE regression tests cover all eight octants, repeated
held input, release/repress, map edges and invalid octants. Full SFML validation
of the player adapter and panel animation remains open.

## Reconciled historical claims

| Topic | Accepted preserved finding | Superseded or open |
| --- | --- | --- |
| OBJECT | 28 bytes, capacity 350 | Old 80-byte hypothesis |
| GUARD | 26 bytes, capacity 100 | Old 98-byte hypothesis |
| Visible span | 20 bytes, capacity 50 | Old 52-byte hypothesis |
| Renderer | MAP boundary/VEC/span pipeline | Existing DDA renderer is not original-equivalent |
| E1M3 / E1M11 | Shared first-plane geometry fingerprint | Second-plane bytes differ; not identical maps |
| GUARD state 0x13 | Timer/step subset exists in canonical research | Full modern movement/occupancy integration pending |
| Projectiles | 8 x 42-byte pool and documented hit/projection subsets | Complete weapon/guard damage integration pending |

Archived assistant summaries alone do not establish new VERIFIED_EXE evidence.
Returned historical coverage percentages, function counts and plans are not code
specifications. Findings must retain platform/version and uncertainty boundaries.
