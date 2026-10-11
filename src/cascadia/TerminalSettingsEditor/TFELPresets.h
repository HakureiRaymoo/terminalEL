// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once
#include <array>
#include <cstddef>
#include <string_view>

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    // Immutable v1 snapshots. Never replace these values when tuning defaults.
    // Order matches the TFEL preview transport: tone, bloom, RGB, color, glow.
    struct TFELPreset
    {
        std::wstring_view name;
        std::array<float, 16> values;
    };

    inline constexpr std::array TFELBuiltinPresets{
        TFELPreset{ L"Foundation v1 (approved)", { 2.0f, 1.0f, 0.0f, 1.0f, 0.6f, 0.30f, 8.0f, 0.10f, 1.0f, 0.82f, 0.04f, 0.0f, 1.0f, 18.0f, 1.0f, 1.0f } },
        TFELPreset{ L"Classic amber v1", { 1.0f, 1.0f, 0.0f, 1.0f, 0.6f, 0.30f, 8.0f, 0.10f, 1.0f, 0.68f, 0.12f, 0.0f, 1.0f, 18.0f, 1.0f, 0.0f } },
        TFELPreset{ L"Soft gold v1", { 1.2f, 1.0f, 0.0f, 1.0f, 0.3f, 0.35f, 6.0f, 0.10f, 1.0f, 0.82f, 0.04f, 0.0f, 0.8f, 25.0f, 0.9f, 0.7f } },
        TFELPreset{ L"Green phosphor v1", { 1.3f, 1.0f, 0.0f, 1.0f, 0.4f, 0.30f, 8.0f, 0.10f, 0.25f, 1.0f, 0.18f, 0.0f, 1.0f, 145.0f, 0.85f, 0.6f } },
    };

    inline constexpr std::array<std::wstring_view, 16> TFELPresetKeys{
        L"terminalEL.brightness", L"terminalEL.contrast", L"terminalEL.blackLevel", L"terminalEL.gamma",
        L"terminalEL.bloomIntensity", L"terminalEL.bloomThreshold", L"terminalEL.bloomRadius", L"terminalEL.bloomSoftness",
        L"terminalEL.phosphorRed", L"terminalEL.phosphorGreen", L"terminalEL.phosphorBlue",
        L"terminalEL.hue", L"terminalEL.saturation", L"terminalEL.glowHue", L"terminalEL.glowSaturation", L"terminalEL.glowMix"
    };

    inline constexpr std::array<std::wstring_view, 16> TFELPreviewProperties{
        L"TFELBrightness", L"TFELContrast", L"TFELBlackLevel", L"TFELGamma",
        L"TFELBloomIntensity", L"TFELBloomThreshold", L"TFELBloomRadius", L"TFELBloomSoftness",
        L"TFELPhosphorRed", L"TFELPhosphorGreen", L"TFELPhosphorBlue",
        L"TFELHue", L"TFELSaturation", L"TFELGlowHue", L"TFELGlowSaturation", L"TFELGlowMix"
    };

    constexpr bool IsTFELPreviewProperty(std::wstring_view name) noexcept
    {
        for (const auto property : TFELPreviewProperties)
        {
            if (name == property) return true;
        }
        return false;
    }

    // Validate JSON doubles before narrowing to float. Inclusive bounds also
    // reject NaN and infinities through the ordered comparisons.
    inline constexpr std::array<double, 16> TFELPresetMinimum{
        0, 0, 0, 0.5, 0, 0, 0, 0, 0, 0, 0, -180, 0, 0, 0, 0
    };
    inline constexpr std::array<double, 16> TFELPresetMaximum{
        2, 2, 0.5, 2.5, 2, 1, 16, 0.5, 1, 1, 1, 180, 2, 360, 1, 1
    };

    constexpr bool IsValidTFELPresetValue(std::size_t index, double value) noexcept
    {
        return index < TFELPresetMinimum.size() &&
               value >= TFELPresetMinimum[index] && value <= TFELPresetMaximum[index];
    }
}
