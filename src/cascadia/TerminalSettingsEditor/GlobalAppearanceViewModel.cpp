// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "GlobalAppearanceViewModel.h"
#include "GlobalAppearanceViewModel.g.cpp"
#include "EnumEntry.h"
#include "TFELPresets.h"
#include <winrt/Windows.Data.Json.h>
#include <cmath>

using namespace winrt;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    constexpr std::wstring_view systemThemeName{ L"system" };
    static void validateTFELPresetValues(const std::array<float, 16>& values)
    {
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (!IsValidTFELPresetValue(i, values[i]))
                throw hresult_invalid_argument(RS_(L"TFELInvalidRange"));
        }
    }

    static std::array<float, 16> readTFELPresetValues(const Windows::Data::Json::JsonObject& settings)
    {
        const auto mode = settings.GetNamedString(L"terminalEL.colorMode");
        if (mode != L"monochrome" && mode != L"native" && mode != L"native-tinted" && mode != L"native-hybrid")
            throw hresult_invalid_argument(RS_(L"TFELUnsupportedVersion"));
        std::array<float, 16> values;
        for (size_t i = 0; i < values.size(); ++i)
        {
            const auto value = settings.GetNamedNumber(hstring{ TFELPresetKeys[i] });
            if (!IsValidTFELPresetValue(i, value))
                throw hresult_invalid_argument(RS_(L"TFELInvalidRange"));
            values[i] = static_cast<float>(value);
        }
        return values;
    }

    static Windows::Data::Json::JsonObject serializeTFELPreset(const hstring& name, const std::array<float, 16>& values, const hstring& mode = L"monochrome")
    {
        using namespace Windows::Data::Json;
        validateTFELPresetValues(values);
        JsonObject settings;
        for (size_t i = 0; i < values.size(); ++i)
        {
            settings.Insert(hstring{ TFELPresetKeys[i] }, JsonValue::CreateNumberValue(values[i]));
        }
        settings.Insert(L"terminalEL.colorMode", JsonValue::CreateStringValue(mode));
        JsonObject result;
        result.Insert(L"version", JsonValue::CreateNumberValue(1));
        result.Insert(L"renderer", JsonValue::CreateStringValue(mode == L"monochrome" ? L"tfel-v1" : L"native-el-v1"));
        result.Insert(L"name", JsonValue::CreateStringValue(name));
        result.Insert(L"settings", settings);
        return result;
    }

    void GlobalAppearanceViewModel::_LoadTFELPresets()
    {
        using namespace Windows::Data::Json;
        _userTFELPresets.clear();
        JsonArray presets;
        if (!JsonArray::TryParse(_GlobalSettings.TFELUserPresets(), presets))
        {
            return;
        }
        for (uint32_t storageIndex = 0; storageIndex < presets.Size(); ++storageIndex)
        {
            try
            {
                const auto obj = presets.GetAt(storageIndex).GetObject();
                if (obj.GetNamedNumber(L"version") != 1) continue;
                UserTFELPreset preset{ obj.GetNamedString(L"name"), {}, storageIndex };
                if (preset.name.empty()) continue;
                preset.values = readTFELPresetValues(obj.GetNamedObject(L"settings"));
                preset.mode = obj.GetNamedObject(L"settings").GetNamedString(L"terminalEL.colorMode");
                if (obj.GetNamedString(L"renderer") != (preset.mode == L"monochrome" ? L"tfel-v1" : L"native-el-v1")) continue;
                _userTFELPresets.push_back(std::move(preset));
            }
            CATCH_LOG();
        }
    }

    hstring GlobalAppearanceViewModel::_ValidateTFELName(const hstring& name, int32_t excludedIndex) const
    {
        std::wstring trimmed{ name };
        const auto start = trimmed.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos) throw hresult_invalid_argument(RS_(L"TFELNameRequired"));
        trimmed = trimmed.substr(start, trimmed.find_last_not_of(L" \t\r\n") - start + 1);
        if (trimmed.size() > 80) throw hresult_invalid_argument(RS_(L"TFELNameTooLong"));
        const hstring cleanName{ trimmed };
        for (const auto& preset : TFELBuiltinPresets)
        {
            if (til::compare_linguistic_insensitive(cleanName, hstring{ preset.name }) == 0) throw hresult_invalid_argument(RS_(L"TFELNameExists"));
        }
        for (size_t i = 0; i < _userTFELPresets.size(); ++i)
        {
            if (static_cast<int32_t>(i + TFELBuiltinPresets.size()) == excludedIndex) continue;
            const auto& preset = _userTFELPresets[i];
            if (til::compare_linguistic_insensitive(cleanName, preset.name) == 0) throw hresult_invalid_argument(RS_(L"TFELNameExists"));
        }
        return cleanName;
    }

    int32_t GlobalAppearanceViewModel::CreateTFELPreset(const hstring& name)
    {
        using namespace Windows::Data::Json;
        const auto cleanName = _ValidateTFELName(name);
        const auto values = _CurrentTFELValues();
        validateTFELPresetValues(values);
        if (TFELColorMode() != L"monochrome" && TFELColorMode() != L"native" && TFELColorMode() != L"native-tinted" && TFELColorMode() != L"native-hybrid") throw hresult_invalid_argument(RS_(L"TFELUnsupportedVersion"));
        // Append to the original array so unknown future versions are preserved.
        JsonArray presets;
        if (!JsonArray::TryParse(_GlobalSettings.TFELUserPresets(), presets))
            throw hresult_invalid_argument(RS_(L"TFELInvalidLibrary"));
        presets.Append(serializeTFELPreset(cleanName, values, TFELColorMode()));
        _GlobalSettings.TFELUserPresets(presets.Stringify());
        _userTFELPresets.push_back({ cleanName, values, presets.Size() - 1, TFELColorMode() });
        _NotifyChanges(L"TFELCurrentPresetStatus");
        return static_cast<int32_t>(TFELBuiltinPresets.size() + _userTFELPresets.size() - 1);
    }

    hstring GlobalAppearanceViewModel::ExportTFELPreset(int32_t index) const
    {
        if (index < 0) throw hresult_invalid_argument(RS_(L"TFELSelectPreset"));
        const auto i = static_cast<size_t>(index);
        if (i < TFELBuiltinPresets.size())
        {
            const auto& preset = TFELBuiltinPresets[i];
            return serializeTFELPreset(hstring{ preset.name }, preset.values).Stringify();
        }
        if (i - TFELBuiltinPresets.size() >= _userTFELPresets.size()) throw hresult_invalid_argument();
        const auto& preset = _userTFELPresets[i - TFELBuiltinPresets.size()];
        return serializeTFELPreset(preset.name, preset.values, preset.mode).Stringify();
    }

    size_t GlobalAppearanceViewModel::_UserTFELIndex(int32_t index) const
    {
        if (index < static_cast<int32_t>(TFELBuiltinPresets.size()))
            throw hresult_invalid_argument(RS_(L"TFELReadOnly"));
        const auto i = static_cast<size_t>(index) - TFELBuiltinPresets.size();
        if (i >= _userTFELPresets.size()) throw hresult_invalid_argument(RS_(L"TFELSelectPreset"));
        return i;
    }

    void GlobalAppearanceViewModel::RenameTFELPreset(int32_t index, const hstring& name)
    {
        using namespace Windows::Data::Json;
        const auto i = _UserTFELIndex(index);
        const auto cleanName = _ValidateTFELName(name, index);
        auto presets = JsonArray::Parse(_GlobalSettings.TFELUserPresets());
        const auto obj = presets.GetAt(_userTFELPresets[i].storageIndex).GetObject();
        obj.Insert(L"name", JsonValue::CreateStringValue(cleanName));
        _GlobalSettings.TFELUserPresets(presets.Stringify());
        _userTFELPresets[i].name = cleanName;
        _NotifyChanges(L"TFELCurrentPresetStatus");
    }

    void GlobalAppearanceViewModel::DeleteTFELPreset(int32_t index)
    {
        using namespace Windows::Data::Json;
        const auto i = _UserTFELIndex(index);
        auto presets = JsonArray::Parse(_GlobalSettings.TFELUserPresets());
        presets.RemoveAt(_userTFELPresets[i].storageIndex);
        _GlobalSettings.TFELUserPresets(presets.Stringify());
        _LoadTFELPresets();
        _NotifyChanges(L"TFELCurrentPresetStatus");
    }

    std::array<float, 16> GlobalAppearanceViewModel::_CurrentTFELValues() const
    {
        return { TFELBrightness(), TFELContrast(), TFELBlackLevel(), TFELGamma(),
                 TFELBloomIntensity(), TFELBloomThreshold(), TFELBloomRadius(), TFELBloomSoftness(),
                 TFELPhosphorRed(), TFELPhosphorGreen(), TFELPhosphorBlue(), TFELHue(), TFELSaturation(),
                 TFELGlowHue(), TFELGlowSaturation(), TFELGlowMix() };
    }

    hstring GlobalAppearanceViewModel::TFELCurrentPresetStatus() const
    {
        const auto values = _CurrentTFELValues();
        const auto matches = [&](const auto& preset) {
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (!std::isfinite(values[i]) || std::abs(values[i] - preset.values[i]) > 0.00001f) return false;
            }
            return true;
        };
        std::wstring names;
        const auto append = [&](const std::wstring_view name) {
            if (!names.empty()) names += L" / ";
            names += name;
        };
        for (const auto& preset : TFELBuiltinPresets) if (TFELColorMode() == L"monochrome" && matches(preset)) append(preset.name);
        for (const auto& preset : _userTFELPresets) if (TFELColorMode() == preset.mode && matches(preset)) append(preset.name);
        if (names.empty()) return RS_(L"TFELStatusCustom");
        return hstring{ std::wstring{ RS_(L"TFELStatusMatches") } + names };
    }

    void GlobalAppearanceViewModel::UpdateTFELPreset(int32_t index)
    {
        using namespace Windows::Data::Json;
        const auto i = _UserTFELIndex(index);
        const auto values = _CurrentTFELValues();
        validateTFELPresetValues(values);
        if (TFELColorMode() != L"monochrome" && TFELColorMode() != L"native" && TFELColorMode() != L"native-tinted" && TFELColorMode() != L"native-hybrid") throw hresult_invalid_argument(RS_(L"TFELUnsupportedVersion"));
        auto presets = JsonArray::Parse(_GlobalSettings.TFELUserPresets());
        const auto obj = presets.GetAt(_userTFELPresets[i].storageIndex).GetObject();
        const auto settings = obj.GetNamedObject(L"settings");
        settings.Insert(L"terminalEL.colorMode", JsonValue::CreateStringValue(TFELColorMode()));
        obj.Insert(L"renderer", JsonValue::CreateStringValue(TFELColorMode() == L"monochrome" ? L"tfel-v1" : L"native-el-v1"));
        // Update only known parameters, retaining unknown metadata and array entries.
        for (size_t n = 0; n < values.size(); ++n)
            settings.Insert(hstring{ TFELPresetKeys[n] }, JsonValue::CreateNumberValue(values[n]));
        _GlobalSettings.TFELUserPresets(presets.Stringify());
        _userTFELPresets[i].values = values;
        _userTFELPresets[i].mode = TFELColorMode();
        _NotifyChanges(L"TFELCurrentPresetStatus");
    }

    int32_t GlobalAppearanceViewModel::ImportTFELPreset(const hstring& json, const hstring& name)
    {
        using namespace Windows::Data::Json;
        if (json.size() > 65536) throw hresult_invalid_argument(RS_(L"TFELTextTooLarge"));
        const auto obj = JsonObject::Parse(json);
        if (obj.GetNamedNumber(L"version") != 1)
            throw hresult_invalid_argument(RS_(L"TFELUnsupportedVersion"));
        const auto cleanName = _ValidateTFELName(name.empty() ? obj.GetNamedString(L"name") : name);
        const auto values = readTFELPresetValues(obj.GetNamedObject(L"settings"));
        const auto mode = obj.GetNamedObject(L"settings").GetNamedString(L"terminalEL.colorMode");
        if (obj.GetNamedString(L"renderer") != (mode == L"monochrome" ? L"tfel-v1" : L"native-el-v1"))
            throw hresult_invalid_argument(RS_(L"TFELUnsupportedVersion"));
        // Validate completely before touching the library or current effect.
        auto presets = JsonArray::Parse(_GlobalSettings.TFELUserPresets());
        presets.Append(serializeTFELPreset(cleanName, values, mode));
        _GlobalSettings.TFELUserPresets(presets.Stringify());
        _userTFELPresets.push_back({ cleanName, values, presets.Size() - 1, mode });
        _NotifyChanges(L"TFELCurrentPresetStatus");
        return static_cast<int32_t>(TFELBuiltinPresets.size() + _userTFELPresets.size() - 1);
    }

    IObservableVector<Windows::Foundation::IInspectable> GlobalAppearanceViewModel::TFELPresetNames() const
    {
        // XAML ItemsSource consumes an object/bindable collection. A C++/WinRT
        // vector<hstring> does not provide the object-vector projection.
        auto names = single_threaded_observable_vector<Windows::Foundation::IInspectable>();
        for (const auto& preset : TFELBuiltinPresets)
        {
            names.Append(box_value(hstring{ preset.name }));
        }
        for (const auto& preset : _userTFELPresets)
        {
            names.Append(box_value(preset.name));
        }
        return names;
    }

    void GlobalAppearanceViewModel::ApplyTFELPreset(int32_t index)
    {
        if (index < 0 || static_cast<size_t>(index) >= TFELBuiltinPresets.size() + _userTFELPresets.size())
        {
            return;
        }
        const auto i = static_cast<size_t>(index);
        const auto& v = i < TFELBuiltinPresets.size() ? TFELBuiltinPresets[i].values : _userTFELPresets[i - TFELBuiltinPresets.size()].values;
        TFELBrightness(v[0]);
        TFELContrast(v[1]);
        TFELBlackLevel(v[2]);
        TFELGamma(v[3]);
        TFELBloomIntensity(v[4]);
        TFELBloomThreshold(v[5]);
        TFELBloomRadius(v[6]);
        TFELBloomSoftness(v[7]);
        TFELPhosphorRed(v[8]);
        TFELPhosphorGreen(v[9]);
        TFELPhosphorBlue(v[10]);
        TFELHue(v[11]);
        TFELSaturation(v[12]);
        TFELGlowHue(v[13]);
        TFELGlowSaturation(v[14]);
        TFELGlowMix(v[15]);
        TFELColorMode(i < TFELBuiltinPresets.size() ? hstring{ L"monochrome" } : _userTFELPresets[i - TFELBuiltinPresets.size()].mode);
    }

    constexpr std::wstring_view darkThemeName{ L"dark" };
    constexpr std::wstring_view lightThemeName{ L"light" };
    constexpr std::wstring_view legacySystemThemeName{ L"legacySystem" };
    constexpr std::wstring_view legacyDarkThemeName{ L"legacyDark" };
    constexpr std::wstring_view legacyLightThemeName{ L"legacyLight" };

    GlobalAppearanceViewModel::GlobalAppearanceViewModel(Model::GlobalAppSettings globalSettings, Model::WindowSettings windowSettings) :
        _GlobalSettings{ globalSettings },
        _WindowSettings{ windowSettings },
        _ThemeList{ single_threaded_observable_vector<Model::Theme>() }
    {
        INITIALIZE_BINDABLE_ENUM_SETTING(NewTabPosition, NewTabPosition, NewTabPosition, L"Globals_NewTabPosition", L"Content");
        INITIALIZE_BINDABLE_ENUM_SETTING(TabWidthMode, TabViewWidthMode, winrt::Microsoft::UI::Xaml::Controls::TabViewWidthMode, L"Globals_TabWidthMode", L"Content");
        _UpdateThemeList();
        _LoadTFELPresets();
        // Owned by this VM; no external subscription or strong self capture.
        PropertyChanged([this](const auto&, const Windows::UI::Xaml::Data::PropertyChangedEventArgs& args) {
            const auto propertyName = args.PropertyName();
            const std::wstring_view name{ propertyName };
            if (name == L"TFELColorMode") _NotifyChanges(L"TFELModeIndex");
            if (IsTFELPreviewProperty(name))
                _NotifyChanges(L"TFELCurrentPresetStatus");
        });
    }

    // Function Description:
    // - Updates the list of all themes available to choose from.
    void GlobalAppearanceViewModel::_UpdateThemeList()
    {
        // Surprisingly, though this is called every time we navigate to the page,
        // the list does not keep growing on each navigation.
        const auto& ThemeMap{ _GlobalSettings.Themes() };
        for (const auto& pair : ThemeMap)
        {
            _ThemeList.Append(pair.Value());
        }
    }

    winrt::Windows::Foundation::IInspectable GlobalAppearanceViewModel::CurrentTheme()
    {
        return _GlobalSettings.CurrentTheme(_WindowSettings);
    }

    // Get the name out of the newly selected item, stash that as the Theme name
    // set for the globals. That controls which theme is actually the current
    // theme.
    void GlobalAppearanceViewModel::CurrentTheme(const winrt::Windows::Foundation::IInspectable& tag)
    {
        if (const auto& theme{ tag.try_as<Model::Theme>() })
        {
            _WindowSettings.Theme(Model::ThemePair{ theme.Name() });
        }
    }

    // Method Description:
    // - Convert the names of the inbox themes to some more descriptive,
    //   well-known values. If the passed in theme isn't an inbox one, then just
    //   return its set Name.
    //    - "light" becomes "Light"
    //    - "dark" becomes "Dark"
    //    - "system" becomes "Use Windows theme"
    // - These values are all localized based on the app language.
    // Arguments:
    // - theme: the theme to get the display name for.
    // Return Value:
    // - the potentially localized name to use for this Theme.
    winrt::hstring GlobalAppearanceViewModel::ThemeNameConverter(const Model::Theme& theme)
    {
        if (theme.Name() == darkThemeName)
        {
            return RS_(L"Globals_ThemeDark/Content");
        }
        else if (theme.Name() == lightThemeName)
        {
            return RS_(L"Globals_ThemeLight/Content");
        }
        else if (theme.Name() == systemThemeName)
        {
            return RS_(L"Globals_ThemeSystem/Content");
        }
        else if (theme.Name() == legacyDarkThemeName)
        {
            return RS_(L"Globals_ThemeDarkLegacy/Content");
        }
        else if (theme.Name() == legacyLightThemeName)
        {
            return RS_(L"Globals_ThemeLightLegacy/Content");
        }
        else if (theme.Name() == legacySystemThemeName)
        {
            return RS_(L"Globals_ThemeSystemLegacy/Content");
        }
        return theme.Name();
    }

    bool GlobalAppearanceViewModel::InvertedDisableAnimations()
    {
        return !_WindowSettings.DisableAnimations();
    }

    void GlobalAppearanceViewModel::InvertedDisableAnimations(bool value)
    {
        _WindowSettings.DisableAnimations(!value);
    }

    void GlobalAppearanceViewModel::ShowTitlebarToggled(const winrt::Windows::Foundation::IInspectable& /* sender */, const RoutedEventArgs& /* args */)
    {
        // Set AlwaysShowTabs to true if ShowTabsInTitlebar was toggled OFF -> ON
        if (!ShowTabsInTitlebar())
        {
            AlwaysShowTabs(true);
        }
    }
}
