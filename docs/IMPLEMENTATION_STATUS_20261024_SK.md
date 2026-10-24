# OpenNitemare3D — implementačný stav (24. 10. 2026)

Toto je **clean-room bootstrap**. Čísla 99–99,5 % statická znalosť, 97–98 % behaviorálna znalosť a 89–92 % historický **odhad** celkovej parity z výskumných diskusií **nie sú meraním hotovosti tohto repozitára**.

## Potvrdené hranice
- Koncepčné rozhrania a testovacie utility ešte nepredstavujú kompletne implementovaný DOS/Win16 gameplay engine.
- Nenasadzuje sa legacy diagnostický DDA renderer ako náhrada originálneho algoritmu.
- Chýba preukázaný synchronizovaný 1:1 original-vs-remake framebuffer+DAC zero-diff test.
- USER.SAV snapshot/first-diff nástroje sú všeobecné; neznamenajú verifikovaný decoder pre všetky verzie.

## Nasledujúce akceptačné brány
1. Pripojiť pôvodnú VEC/owner/span pipeline k aktívnemu rendereru a preukázať 64 000 pixelov + 768 DAC zero-diff na identickom stave.
2. Reprodukovať OBJECT scheduler a slot lifecycle deterministicky, vrátane nepriamo vložených/odstránených objektov.
3. Preukázať Weapon/Projectile/DAMAGE/RNG tick-exact trace parity.
4. Pridať SAVE/LOAD prvý gameplay frame a stav cache.
5. DEMO input a RNG prvá divergencia, samostatné DOS/Win16 prefixy.

Pozri [výskumný stav a percentá](https://github.com/marek177/Nitemare3d-reversed/blob/main/docs/RE_STATUS_AREAS_20261024_SK.md) a [implementačný bridge](OCT22_24_2026_IMPLEMENTATION_BRIDGE.md).

**Bez nových testovacích dôkazov neaktualizovať úspešnosť parity ani percentá implementácie.**
