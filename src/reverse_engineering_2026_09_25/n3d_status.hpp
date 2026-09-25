#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace n3d::re {

enum class ReConfidence : std::uint8_t {
    Confirmed,
    StronglyInferred,
    Partial,
    Open
};

struct ReSubsystemStatus {
    std::string_view name;
    std::uint8_t minPercent;
    std::uint8_t maxPercent;
    ReConfidence confidence;
    std::string_view note;
};

// Working RE snapshot for 2026-09-25.
// Percentages are coverage estimates for the currently inspected Win16/Nite3W
// behavior, not proof of original-source equivalence.
inline constexpr std::array<ReSubsystemStatus, 10> kReSubsystemStatus{{
    {"renderer-static", 80, 85, ReConfidence::Partial,
     "Core raycasting/render data flow is mapped; pixel-perfect parity remains open."},
    {"player-movement-collision", 80, 90, ReConfidence::Partial,
     "Movement commit addresses and major collision branches are known; edge/sliding parity still needs runtime verification."},
    {"use-interactions", 75, 90, ReConfidence::Partial,
     "InputUse is confirmed as 0x0200; complete door/switch/teleport/push/special side effects are not yet closed."},
    {"guard-runtime-ai", 75, 90, ReConfidence::Partial,
     "Record layout, state range and strategies are mapped; several state-specific transitions remain open."},
    {"object-runtime", 70, 85, ReConfidence::Partial,
     "28-byte OBJECT layout and major fields are mapped; full per-class runtime behavior remains incomplete."},
    {"hud-uif-menu", 60, 80, ReConfidence::Partial,
     "Major sources and draw paths are identified; exact UIF indices/redraw policy/right-panel purpose remain incomplete."},
    {"img-seqdef-animation", 65, 80, ReConfidence::Partial,
     "Loading/addressing is stronger than semantic mapping of every animation token."},
    {"sound-event-mapping", 65, 85, ReConfidence::Partial,
     "SND container handling is strong; exact event/enemy mapping still has gaps."},
    {"mfc-runtime-init-memory", 45, 65, ReConfidence::Partial,
     "Framework/object lifetime and ownership remain less completely reconstructed than gameplay data paths."},
    {"bsf-registration-integrity", 35, 50, ReConfidence::Open,
     "Currently the weakest major subsystem; registration/integrity semantics are not closed."}
}};

inline constexpr bool isClosed(ReConfidence c) {
    return c == ReConfidence::Confirmed;
}

} // namespace n3d::re
