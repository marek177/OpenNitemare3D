#pragma once

#define UI_PLAYERFACE 815
#define UI_PLAYERFACE_X 3
#define UI_PLAYERFACE_Y 162


/*
    some versions of the game add new sprites made from 3D models,
    this also adds new rotations for some entities, which is *real* fun to implement,
    since the game has hard-coded offsets and they're all shifted in newer versions of the game :)

    thanks David!
*/

#ifdef NEW_SPRITES
#define UI_FACE_START 699
#define UI_PLASMAGUN 826
#define UI_MAGICWAND 701
#define UI_REVOLVER 702
#define UI_AUTOPLASMAGUN 703
#define UI_KEY_START 806
#else
#define UI_FACE_START 699
#define UI_PLASMAGUN 700
#define UI_MAGICWAND 701
#define UI_REVOLVER 702
#define UI_AUTOPLASMAGUN 703
#define UI_KEY_START 806

#endif

#define UI_WEAPON_X 146
#define UI_WEAPON_Y 113