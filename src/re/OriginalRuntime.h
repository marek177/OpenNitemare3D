#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace n3d::re {

// Clean-room constants synchronized with direct Nitemare 3D executable/data analysis.
// NITE3W-specific addresses/strides are reference facts and must not be assumed to
// apply unchanged to DOS builds without independent confirmation.
struct OriginalRuntime final {
    // MAP / world geometry.
    static constexpr int MapWidth = 64;
    static constexpr int MapHeight = 64;
    static constexpr int MapCellBytes = 2;
    static constexpr int MapHeaderBytes = 514;
    static constexpr int MapLevelBytes = MapWidth * MapHeight * MapCellBytes;
    static constexpr int WorldUnitsPerTile = 64;
    static constexpr int TileCenterOffset = 32;

    // Player.
    static constexpr int PlayerMaxHealth = 100;
    static constexpr int PlayerCollisionHalfExtent = 27;

    // Input mask recovered from DEMO/runtime handling.
    static constexpr std::uint16_t InputForward  = 0x0002;
    static constexpr std::uint16_t InputBackward = 0x0004;
    static constexpr std::uint16_t InputTurnA    = 0x0008;
    static constexpr std::uint16_t InputTurnB    = 0x0010;
    static constexpr std::uint16_t InputFast     = 0x0020;
    static constexpr std::uint16_t InputFineStep = 0x0040;
    static constexpr std::uint16_t InputFire     = 0x0080;
    static constexpr std::uint16_t InputStrafe   = 0x0100;
    static constexpr std::uint16_t InputUse      = 0x0200;

    // Fixed Win16 runtime capacities / strides recovered from NITE3W.
    static constexpr int MaxDoors = 64;
    static constexpr int DoorRuntimeStride = 22;
    static constexpr int MaxPanels = 32;
    static constexpr int PanelRuntimeStride = 22;
    static constexpr int MaxPushables = 12;
    static constexpr int PushRuntimeStride = 6;
    static constexpr int MaxObjects = 350;
    static constexpr int ObjectRuntimeStride = 28;
    static constexpr int MaxGuards = 100;
    static constexpr int GuardRuntimeStride = 26;
    static constexpr int MaxVectors = 1000;
    static constexpr int VectorRuntimeStride = 28;
    static constexpr int VectorListOrientations = 4;
    static constexpr int VectorListEntriesPerOrientation = 333;
    static constexpr int MaxVisibleWallSpans = 50;
    static constexpr int VisibleWallSpanStride = 20;
    static constexpr int MaxProjectedSprites = 100;
    static constexpr int ProjectedSpriteStride = 18;

    // Indexed framebuffer / normal 3-D viewport.
    static constexpr int FramebufferWidth = 320;
    static constexpr int FramebufferHeight = 200;
    static constexpr int ViewportX = 8;
    static constexpr int ViewportY = 4;
    static constexpr int ViewportWidth = 304;
    static constexpr int ViewportHeight = 152;
    static constexpr int ViewportCenterX = 160;
    static constexpr int ViewportCenterY = 80;
    static constexpr std::uint8_t TransparentPaletteIndex = 0x29;
    static constexpr double RecoveredHorizontalFovDegrees = 80.99;

    // NITE3W 1.10 HUD / automap reference addresses and dimensions.
    static constexpr std::uint16_t HudDispatcherSegment = 0x0003;
    static constexpr std::uint16_t HudDispatcherOffset = 0xA3B6;
    static constexpr std::uint16_t AutomapDispatcherSegment = 0x0003;
    static constexpr std::uint16_t AutomapDispatcherOffset = 0xB1A4;
    static constexpr std::uint16_t PlayerTileXGlobal = 0x4BF2;
    static constexpr std::uint16_t PlayerTileYGlobal = 0x4BF4;
    static constexpr std::uint16_t PlayerWorldXGlobal = 0x4BF6;
    static constexpr std::uint16_t PlayerWorldYGlobal = 0x4BF8;
    static constexpr std::uint16_t PlayerHealthGlobal = 0x4C1D;
    static constexpr std::uint16_t EnemyLocatorEnergyGlobal = 0x4C42;
    static constexpr std::uint16_t MapClarityEnergyGlobal = 0x4C43;
    static constexpr int AutomapWidth = 64;
    static constexpr int AutomapHeight = 64;
    static constexpr int AutomapBytes = 4096;
    static constexpr int AutomapViewportWidth = 62;
    static constexpr int AutomapViewportHeight = 36;
    static constexpr int AutomapScreenX = 256;
    static constexpr int AutomapScreenY = 162;

    static constexpr int hudPortraitFrame(std::uint8_t health) noexcept {
        const int hp = std::min<int>(health, PlayerMaxHealth);
        return 13 + (hp + 9) / 10;
    }

    // Automap buffer is X-major: x*64+y, unlike MAP row-major cells.
    static constexpr int automapIndex(int cellX, int cellY) noexcept {
        return cellX * 64 + cellY;
    }

    static constexpr int automapOriginX(int playerCellX) noexcept {
        return std::clamp(playerCellX - 31, 0, 2);
    }

    static constexpr int automapOriginY(int playerCellY) noexcept {
        return std::clamp(playerCellY - 18, 0, 28);
    }

    static constexpr int automapNoisePointCount(std::uint8_t power) noexcept {
        if (power == 0 || power > 15) return 0;
        const int p = power;
        return 500 / (p * p * p);
    }

    // GUARD runtime.
    static constexpr int GuardInitialStrength = 255;
    static constexpr int GuardStateCount = 22;
    static constexpr std::uint8_t GuardPerceptionDecisionState = 0x07;
    static constexpr std::uint8_t GuardLethalContactState = 0x0B;
    static constexpr std::uint8_t GuardTimedDirectionalMoveState = 0x13;
    static constexpr std::uint8_t GuardPainState = 0x15;
    static constexpr int GuardResultOctantOffset = 0x12;
    static constexpr int GuardState13MoveXOffset = 0x13;
    static constexpr int GuardState13MoveYOffset = 0x14;
    static constexpr std::uint8_t DraculaPhase1Class = 0x11;
    static constexpr std::uint8_t DraculaBatPhase2Class = 0x14;

    // Renderer global arrays/counters recovered from NITE3W 1.10 DS.
    static constexpr std::uint16_t WallOwnerTableGlobal = 0x53FE;
    static constexpr std::uint16_t WallOcclusionTableGlobal = 0x58FE;
    static constexpr std::uint16_t RgbPaletteTableGlobal = 0x5B7E;
    static constexpr std::uint16_t VisibleSpanCountGlobal = 0x5E7E;
    static constexpr std::uint16_t VisibleSpanArrayGlobal = 0x5E88;
    static constexpr std::uint16_t ProjectedSpriteArrayGlobal = 0x6270;
    static constexpr std::uint16_t WinGBitmapHandleGlobal = 0x6978;
    static constexpr std::uint16_t VectorCountGlobal = 0x7E56;
    static constexpr std::uint16_t ObjectCountGlobal = 0x7E58;
    static constexpr std::uint16_t GuardCountGlobal = 0x7E5E;

    // IMG/UIF reference facts.
    static constexpr int ImgWallDirectoryOffset = 0x0000;
    static constexpr int ImgObjectDirectoryOffset = 0x0400;
    static constexpr int ImgDirectoryEntries = 256;
    static constexpr int ImgFrameHeaderBytes = 10;
    static constexpr int ImgFirstFrameStreamOffset = 0xBC00;
    static constexpr std::uint8_t HudImageObjectId = 0xFF;
    static constexpr int HudImageFrameCount = 29;
    static constexpr int UifDirectorySlots = 32;
    static constexpr int UifDirectoryEntryBytes = 6;

    static constexpr int imgPixelIndex(int x, int y, int height) noexcept {
        return x * height + y;
    }

    // Menu/save UI.
    static constexpr std::uint16_t MenuDispatcherSegment = 0x0004;
    static constexpr std::uint16_t MenuDispatcherOffset = 0x27DE;
    static constexpr int MenuActionFirst = 1;
    static constexpr int MenuActionLast = 40;
    static constexpr int MenuTableCount = 14;
    static constexpr int MenuItemCount = 80;
    static constexpr int SaveSlotCount = 10;
    static constexpr int SaveNameMaxChars = 40;
    static constexpr int SaveNameMaxPixels = 179;

    // DEMO stream.
    static constexpr int DemoHeaderBytes = 6;
    static constexpr int DemoRecordBytes = 8;

    // Win16 save/config facts.
    static constexpr int WindowsConfigSaveBytes = 20;
    static constexpr int WindowsUserSaveRecordBytes = 55015;

    static constexpr int NormalAmmoCap = 100;

    inline static constexpr std::array<int, 25> GuardScoreByObjectClass = {
        25, 75, 50, 100, 250, 150, 200, 100, 100, 0,
        150, 150, 200, -1000, 1000, 100, 200, 0, 25, 100,
        100, 250, 250, 200, 50
    };

    static constexpr int guardScore(std::uint8_t objectClass) noexcept {
        return objectClass >= 0x08 && objectClass <= 0x20
            ? GuardScoreByObjectClass[objectClass - 0x08]
            : 0;
    }
};

} // namespace n3d::re
