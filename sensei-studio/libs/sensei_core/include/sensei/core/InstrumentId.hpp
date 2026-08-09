#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sensei::core {

// Stable instrument/preset identity. Core stores IDs + metadata only — no DSP.
enum class InstrumentId : std::uint8_t
{
    WarmKeys = 0,       // chords.warm_keys
    DeepBass = 1,       // bass.deep_bass
    BrightPluck = 2,    // melody.bright_pluck
    StudioKitBasic = 3, // drums.studio_kit_basic
};

inline constexpr std::size_t kInstrumentCount = 4;

struct InstrumentInfo
{
    InstrumentId id = InstrumentId::WarmKeys;
    const char* stableId = "";
    const char* displayName = "";
    const char* shortFact = "";
    bool isDrumKit = false;
};

[[nodiscard]] inline constexpr InstrumentInfo instrumentInfo(InstrumentId id) noexcept
{
    switch (id)
    {
        case InstrumentId::DeepBass:
            return { id, "bass.deep_bass", "Deep Bass",
                     "Deep Bass gives the low end its own role.", false };
        case InstrumentId::BrightPluck:
            return { id, "melody.bright_pluck", "Bright Pluck",
                     "Bright Pluck works well for short melodic ideas.", false };
        case InstrumentId::StudioKitBasic:
            return { id, "drums.studio_kit_basic", "Studio Kit",
                     "Studio Kit keeps kick, snare, and hats in distinct lanes.", true };
        case InstrumentId::WarmKeys:
        default:
            return { id, "chords.warm_keys", "Warm Keys",
                     "Warm Keys leave space for sustained chords.", false };
    }
}

[[nodiscard]] inline constexpr bool isValidInstrumentId(InstrumentId id) noexcept
{
    return static_cast<std::uint8_t>(id) < static_cast<std::uint8_t>(kInstrumentCount);
}

[[nodiscard]] inline InstrumentId instrumentIdFromStable(std::string_view stable) noexcept
{
    if (stable == "bass.deep_bass")
        return InstrumentId::DeepBass;
    if (stable == "melody.bright_pluck")
        return InstrumentId::BrightPluck;
    if (stable == "drums.studio_kit_basic")
        return InstrumentId::StudioKitBasic;
    return InstrumentId::WarmKeys;
}

} // namespace sensei::core
