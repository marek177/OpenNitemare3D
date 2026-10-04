#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace n3d::original_renderer {

// Instruction-backed constants recovered from NITE3W.EXE V1.10. These describe
// the original Win16 renderer; they do not claim that the current verification
// Raycaster already implements the recovered pipeline.

// Framebuffer / viewport -----------------------------------------------------
inline constexpr std::size_t kFramebufferWidth = 320;
inline constexpr std::size_t kFramebufferHeight = 200;
inline constexpr std::size_t kFramebufferBytes =
    kFramebufferWidth * kFramebufferHeight;

inline constexpr int kViewportXMin = 8;
inline constexpr int kViewportXMax = 311;
inline constexpr int kViewportYMin = 4;
inline constexpr int kViewportYMax = 155;
inline constexpr std::size_t kViewportWidth = 304;
inline constexpr std::size_t kViewportHeight = 152;
inline constexpr int kViewportCenterX = 160;
inline constexpr int kViewportCenterY = 80;
inline constexpr int kViewportCenterYQ4 = 1280;

inline constexpr std::uint8_t kDefaultFloorPaletteIndex = 0x0C;
inline constexpr std::uint8_t kDefaultCeilingPaletteIndex = 0x11;
inline constexpr std::uint8_t kTransparentSpriteIndex = 0x29;

// MAP -> VEC ----------------------------------------------------------------
inline constexpr std::size_t kMapWidth = 64;
inline constexpr std::size_t kMapHeight = 64;
inline constexpr int kWorldUnitsPerTile = 64;
inline constexpr std::size_t kVectorRecordBytes = 0x1C;
inline constexpr std::size_t kVectorCapacity = 1000;
inline constexpr std::size_t kOrientationListCapacity = 333;
inline constexpr std::size_t kFarPointerBytes = 4;

// Four VECLIST arrays. Each contains far pointers to VEC records.
inline constexpr std::array<std::uint16_t, 4> kVectorListCounts = {
    0x697A, 0x697C, 0x697E, 0x6980
};
inline constexpr std::array<std::uint16_t, 4> kVectorListBases = {
    0x6982, 0x6EB6, 0x73EA, 0x791E
};
inline constexpr std::size_t kVectorListBytes =
    kOrientationListCapacity * kFarPointerBytes; // 0x534

// Orientation used by the recovered MAP-boundary constructor.
enum class VectorOrientation : std::uint8_t {
    Top = 0,    // (x,y)       -> (x+64,y)
    Bottom = 1, // (x,y+64)    -> (x+64,y+64)
    Right = 2,  // (x+64,y)    -> (x+64,y+64)
    Left = 3,   // (x,y)       -> (x,y+64)
};

// VEC record field offsets. Fields whose final high-level semantic names remain
// partial are intentionally described structurally rather than over-named.
inline constexpr std::size_t kVecWallIdOffset = 0x00;
inline constexpr std::size_t kVecTextureOffsetLikeOffset = 0x01; // PARTIAL
inline constexpr std::size_t kVecAnimationAuxOffset = 0x02;      // PARTIAL
inline constexpr std::size_t kVecAnimationFrameOffset = 0x03;    // STRONG
inline constexpr std::size_t kVecTextureSetOffset = 0x04;        // STRONG
inline constexpr std::size_t kVecFlagsOffset = 0x05;
inline constexpr std::size_t kVecRenderClassOffset = 0x06;
inline constexpr std::size_t kVecOrientationOffset = 0x07;
inline constexpr std::size_t kVecTimerOffset = 0x08;              // PARTIAL
inline constexpr std::size_t kVecX1Offset = 0x0C;
inline constexpr std::size_t kVecY1Offset = 0x0E;
inline constexpr std::size_t kVecX2Offset = 0x10;
inline constexpr std::size_t kVecY2Offset = 0x12;
inline constexpr std::size_t kVecScreenX1Offset = 0x14;
inline constexpr std::size_t kVecProjectedY1Q4Offset = 0x16;
inline constexpr std::size_t kVecScreenX2Offset = 0x18;
inline constexpr std::size_t kVecProjectedY2Q4Offset = 0x1A;

inline constexpr std::uint8_t kVecFlagActive = 0x01;
inline constexpr std::uint8_t kVecFlagSpecial04 = 0x04; // semantic TODO
inline constexpr std::uint8_t kVecFlagSpecial08 = 0x08; // semantic TODO
inline constexpr std::uint8_t kVecFlagMaskedLike = 0x10; // PARTIAL
inline constexpr std::uint8_t kVecFlagTextureUFlip = 0x20;

// Visibility / wall spans ---------------------------------------------------
inline constexpr std::uint16_t kColumnOwnerBase = 0x53FE;
inline constexpr std::size_t kColumnOwnerEntries = 320;
inline constexpr std::size_t kColumnOwnerEntryBytes = 4;

inline constexpr std::uint16_t kWallOcclusionBase = 0x58FE;
inline constexpr std::size_t kWallOcclusionEntries = 320;
inline constexpr std::size_t kWallOcclusionEntryBytes = 2;

inline constexpr std::uint16_t kWallSpanCountGlobal = 0x5E7E;
inline constexpr std::uint16_t kWallSpanBase = 0x5E88;
inline constexpr std::size_t kWallSpanRecordBytes = 0x14;
inline constexpr std::size_t kWallSpanCapacity = 50;

// Sprite/object projection --------------------------------------------------
inline constexpr std::uint16_t kProjectedSpriteBase = 0x6270;
inline constexpr std::size_t kProjectedSpriteRecordBytes = 0x12;
inline constexpr std::size_t kProjectedSpriteCapacity = 100;



enum class SpriteSpatialEvidence : std::uint8_t {
    ExecutableLayout,
    BehavioralOnly,
};

// Queue capacity/record size are executable-backed. Vertical anchoring and
// directional object selection observed in gameplay are intentionally not
// encoded as renderer math until the selector/writer path is recovered.
inline constexpr SpriteSpatialEvidence kSpriteQueueEvidence =
    SpriteSpatialEvidence::ExecutableLayout;
inline constexpr SpriteSpatialEvidence kDirectionalSpriteSelectionEvidence =
    SpriteSpatialEvidence::BehavioralOnly;

// Function offsets/names used by the current Ghidra/IDA renderer audit.
// These are evidence anchors, not API entry points in the reconstructed engine.
inline constexpr std::uint16_t kFnOwnerToSpan = 0x6266;
inline constexpr std::uint16_t kFnWallRaster = 0x66B0;
inline constexpr std::uint16_t kFnTextureU = 0x6422;
inline constexpr std::uint16_t kFnWallColumnBlitter = 0x3E44;
inline constexpr std::uint16_t kFnSpriteQueueConsume = 0x6914;

// Remaining high-value renderer unknowns are intentionally represented as
// audit anchors instead of guessed semantics.
inline constexpr std::uint16_t kUnknownDynamicWallGlobal = 0x7E60;
inline constexpr std::uint8_t kSpecialRenderClass2 = 0x02;
inline constexpr std::uint8_t kSpecialRenderClass3F = 0x3F;
inline constexpr std::uint8_t kSpecialRenderClass40 = 0x40;

// Audited WinG wall-column source path uses 64 vertical texture samples.
inline constexpr std::size_t kTextureColumnSamples = 64;

// Projection ---------------------------------------------------------------
inline constexpr std::uint16_t kNearClipAnchor = 0x4000;

// A pinhole interpretation of the recovered normal-viewport projection term
// gives an approximately 81-degree horizontal FOV. This is intentionally NOT
// exposed as an exact original 'FOV=81' setting because no such literal named
// parameter has been found in the executable.

enum class VisibilityModel {
    MapBoundariesToVectorsThenSortedListsToColumnOwners,
};

inline constexpr VisibilityModel kVisibilityModel =
    VisibilityModel::MapBoundariesToVectorsThenSortedListsToColumnOwners;

static_assert(kFramebufferBytes == 64000);
static_assert(kViewportWidth == 304);
static_assert(kViewportHeight == 152);
static_assert(kVectorListBytes == 0x534);
static_assert(kColumnOwnerBase + kColumnOwnerEntries * kColumnOwnerEntryBytes ==
              kWallOcclusionBase);

} // namespace n3d::original_renderer
