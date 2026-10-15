// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "GlobalAppearanceViewModel.g.h"
#include "ViewModelHelpers.h"
#include "Utils.h"
#include <array>
#include <vector>

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct GlobalAppearanceViewModel : GlobalAppearanceViewModelT<GlobalAppearanceViewModel>, ViewModelHelper<GlobalAppearanceViewModel>
    {
    public:
        GlobalAppearanceViewModel(Model::GlobalAppSettings globalSettings, Model::WindowSettings windowSettings);

        // DON'T YOU DARE ADD A `WINRT_CALLBACK(PropertyChanged` TO A CLASS DERIVED FROM ViewModelHelper. Do this instead:
        using ViewModelHelper<GlobalAppearanceViewModel>::PropertyChanged;

        WINRT_PROPERTY(Windows::Foundation::Collections::IObservableVector<Model::Theme>, ThemeList, nullptr);
        GETSET_BINDABLE_ENUM_SETTING(NewTabPosition, Model::NewTabPosition, _WindowSettings.NewTabPosition);
        GETSET_BINDABLE_ENUM_SETTING(TabWidthMode, winrt::Microsoft::UI::Xaml::Controls::TabViewWidthMode, _WindowSettings.TabWidthMode);

    public:
        winrt::Windows::Foundation::IInspectable CurrentTheme();
        void CurrentTheme(const winrt::Windows::Foundation::IInspectable& tag);
        static winrt::hstring ThemeNameConverter(const Model::Theme& theme);

        bool InvertedDisableAnimations();
        void InvertedDisableAnimations(bool value);

        void ShowTitlebarToggled(const winrt::Windows::Foundation::IInspectable& sender, const winrt::Windows::UI::Xaml::RoutedEventArgs& args);

        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, AlwaysShowTabs);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, ShowTabsFullscreen);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, ShowTabsInTitlebar);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, UseAcrylicInTabRow);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, ShowTitleInTitlebar);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, AlwaysOnTop);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, AutoHideWindow);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, AlwaysShowNotificationIcon);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, MinimizeToNotificationArea);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, ShowAdminShield);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_WindowSettings, EnableUnfocusedAcrylic);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELBrightness);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELContrast);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELBlackLevel);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELGamma);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELPanelGrid);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, IMECandidateOverlay);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, IMECandidateGlow);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELPhosphorRed);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELPhosphorGreen);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELPhosphorBlue);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELBloomIntensity);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELBloomThreshold);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELBloomRadius);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELBloomSoftness);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELColorMode);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELHue);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELSaturation);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELGlowHue);
        PERMANENT_OBSERVABLE_PROJECTED_SETTING(_GlobalSettings, TFELGlowSaturation);
        float TFELGlowMix() const { return _GlobalSettings.TFELGlowMix(); }
        bool HasTFELGlowMix() const { return _GlobalSettings.HasTFELGlowMix(); }
        void TFELGlowMix(float value)
        {
            if (TFELGlowMix() != value)
            {
                _GlobalSettings.TFELGlowMix(value);
                _NotifyChanges(L"HasTFELGlowMix", L"TFELGlowMix", L"TFELIndependentGlow");
            }
        }
        bool TFELIndependentGlow() const { return TFELGlowMix() > 0.0f; }
        void TFELIndependentGlow(bool value) { TFELGlowMix(value ? 1.0f : 0.0f); }
        Windows::Foundation::Collections::IObservableVector<Windows::Foundation::IInspectable> TFELPresetNames() const;
        void ApplyTFELPreset(int32_t index);
        int32_t CreateTFELPreset(const hstring& name);
        hstring ExportTFELPreset(int32_t index) const;
        int32_t ImportTFELPreset(const hstring& json, const hstring& name);
        void RenameTFELPreset(int32_t index, const hstring& name);
        void DeleteTFELPreset(int32_t index);
        void UpdateTFELPreset(int32_t index);
        hstring TFELCurrentPresetStatus() const;
        int32_t TFELModeIndex() const { return TFELColorMode() == L"native" ? 1 : TFELColorMode() == L"native-tinted" ? 2 : TFELColorMode() == L"native-hybrid" ? 3 : 0; }
        void TFELModeIndex(int32_t value)
        {
            if (value >= 0 && value <= 3) TFELColorMode(value == 1 ? L"native" : value == 2 ? L"native-tinted" : value == 3 ? L"native-hybrid" : L"monochrome");
        }

    private:
        Model::GlobalAppSettings _GlobalSettings;
        Model::WindowSettings _WindowSettings;
        winrt::Windows::Foundation::IInspectable _currentTheme;

        void _UpdateThemeList();
        struct UserTFELPreset
        {
            hstring name;
            std::array<float, 16> values;
            uint32_t storageIndex{};
            hstring mode{ L"monochrome" };
        };
        std::vector<UserTFELPreset> _userTFELPresets;
        void _LoadTFELPresets();
        std::array<float, 16> _CurrentTFELValues() const;
        hstring _ValidateTFELName(const hstring& name, int32_t excludedIndex = -1) const;
        size_t _UserTFELIndex(int32_t index) const;
    };
};

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(GlobalAppearanceViewModel);
}
