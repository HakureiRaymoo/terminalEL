// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "Profiles_Appearance.h"
#include "Appearances.h"

#include "ProfileViewModel.h"
#include "PreviewConnection.h"

#include "Profiles_Appearance.g.cpp"
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <algorithm>
#include <cmath>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;

static constexpr std::wstring_view AppearanceSettingPrefix{ L"App." };

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    Profiles_Appearance::Profiles_Appearance()
    {
        InitializeComponent();
        _previewConnection = winrt::make_self<PreviewConnection>();
    }

    void Profiles_Appearance::OnNavigatedTo(const NavigationEventArgs& e)
    {
        const auto args = e.Parameter().as<Editor::NavigateToPageArgs>();
        _Profile = args.ViewModel().as<Editor::ProfileViewModel>();
        _activeTFELNumber = {};
        _weakWindowRoot = args.WindowRoot();

        // Settings are stored in Profiles_Appearance and Appearances.
        // We use the "App." prefix to indicate if it's in Appearances,
        // and remove it on the way to Appearances object.
        const auto elementToFocus = args.ElementToFocus();
        if (elementToFocus.starts_with(AppearanceSettingPrefix))
        {
            std::wstring correctedName{ elementToFocus.c_str() };
            get_self<implementation::Appearances>(DefaultAppearanceView())->BringIntoViewWhenLoaded(hstring{ correctedName.substr(AppearanceSettingPrefix.size()) });
        }
        else
        {
            BringIntoViewWhenLoaded(elementToFocus);
        }

        if (!_previewControl)
        {
            const auto settings = winrt::get_self<implementation::ProfileViewModel>(_Profile)->TermSettings();
            _previewConnection->DisplayPowerlineGlyphs(_Profile.DefaultAppearance().HasPowerlineCharacters());
            _previewControl = Control::TermControl(settings, settings, *_previewConnection);
            _previewControl.CursorVisibility(Control::CursorDisplayState::Shown);
            _previewControl.IsEnabled(false);
            _previewControl.AllowFocusWhenDisabled(false);
            ControlPreview().Child(_previewControl);
        }

        // Subscribe to some changes in the view model
        // These changes should force us to update our own set of "Current<Setting>" members,
        // and propagate those changes to the UI
        _ViewModelChangedRevoker = _Profile.PropertyChanged(winrt::auto_revoke, { this, &Profiles_Appearance::_onProfilePropertyChanged });
        // The Appearances object handles updating the values in the settings UI, but
        // we still need to listen to the changes here just to update the preview control
        _AppearanceViewModelChangedRevoker = _Profile.DefaultAppearance().PropertyChanged(winrt::auto_revoke, { this, &Profiles_Appearance::_onProfilePropertyChanged });

        TraceLoggingWrite(
            g_hTerminalSettingsEditorProvider,
            "NavigatedToPage",
            TraceLoggingDescription("Event emitted when the user navigates to a page in the settings UI"),
            TraceLoggingValue("profile.appearance", "PageId", "The identifier of the page that was navigated to"),
            TraceLoggingValue(_Profile.IsBaseLayer(), "IsProfileDefaults", "If the modified profile is the profile.defaults object"),
            TraceLoggingValue(static_cast<GUID>(_Profile.Guid()), "ProfileGuid", "The guid of the profile that was navigated to"),
            TraceLoggingValue(_Profile.Source().c_str(), "ProfileSource", "The source of the profile that was navigated to"),
            TraceLoggingValue(_Profile.DefaultAppearance().BackgroundImageSettingsEnabled(), "HasBackgroundImage", "If the profile has a background image defined"),
            TraceLoggingValue(_Profile.HasUnfocusedAppearance(), "HasUnfocusedAppearance", "If the profile has an unfocused appearance defined"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void Profiles_Appearance::OnNavigatedFrom(const NavigationEventArgs& /*e*/)
    {
        _activeTFELNumber = {};
        _ViewModelChangedRevoker.revoke();
        _AppearanceViewModelChangedRevoker.revoke();
    }

    void Profiles_Appearance::TFELNumber_GotFocus(const IInspectable&, const RoutedEventArgs& args)
    {
        _activeTFELNumber = {};
        auto element = args.OriginalSource().try_as<DependencyObject>();
        while (element)
        {
            if (const auto box = element.try_as<Microsoft::UI::Xaml::Controls::NumberBox>())
            {
                _activeTFELNumber = box;
                return;
            }
            element = Media::VisualTreeHelper::GetParent(element);
        }
    }

    void Profiles_Appearance::TFELNumber_ValueChanged(const Microsoft::UI::Xaml::Controls::NumberBox& box,
                                                     const Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs& args)
    {
        // Match the global editor: initialization never creates a profile override.
        // Keep the last focused box until focus moves, allowing focus-loss validation.
        if (!_Profile || !box.IsLoaded() || _activeTFELNumber.get() != box) return;
        const auto key = unbox_value<hstring>(box.Tag());
        const auto value = args.NewValue();
#define TFEL_NUMBER(name) \
        if (key == L## #name) \
        { \
            if (std::isfinite(value)) _Profile.name(static_cast<float>(std::clamp(value, box.Minimum(), box.Maximum()))); \
            box.Value(_Profile.name()); \
            return; \
        }
        TFEL_NUMBER(TFELHue)
        TFEL_NUMBER(TFELSaturation)
        TFEL_NUMBER(TFELGlowHue)
        TFEL_NUMBER(TFELGlowSaturation)
        TFEL_NUMBER(TFELGlowMix)
#undef TFEL_NUMBER
    }

    void Profiles_Appearance::_onProfilePropertyChanged(const IInspectable&, const PropertyChangedEventArgs&)
    {
        if (!_updatePreviewControl)
        {
            _updatePreviewControl = std::make_shared<ThrottledFunc<>>(
                winrt::Windows::System::DispatcherQueue::GetForCurrentThread(),
                til::throttled_func_options{
                    .delay = std::chrono::milliseconds{ 100 },
                    .debounce = true,
                    .trailing = true,
                },
                [this]() {
                    const auto settings = winrt::get_self<implementation::ProfileViewModel>(_Profile)->TermSettings();
                    _previewConnection->DisplayPowerlineGlyphs(_Profile.DefaultAppearance().HasPowerlineCharacters());
                    _previewControl.UpdateControlSettings(settings, settings);
                });
        }

        _updatePreviewControl->Run();
    }
}
