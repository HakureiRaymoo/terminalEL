// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "SettingsPaneContent.h"
#include "Utils.h"
#include "TFELPreview.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Microsoft::Terminal::Settings::Model;

#define ASSERT_UI_THREAD() assert(_sui.Dispatcher().HasThreadAccess())

namespace winrt::TerminalApp::implementation
{
    SettingsPaneContent::SettingsPaneContent(CascadiaSettings settings, WindowSettings windowSettings)
    {
        _sui = winrt::Microsoft::Terminal::Settings::Editor::MainPage{ settings, windowSettings };
        _tfelSession = TFELPreview::Begin();
        _tfelUpdate = std::make_shared<ThrottledFunc<>>(
            winrt::Windows::System::DispatcherQueue::GetForCurrentThread(),
            til::throttled_func_options{ .delay = std::chrono::milliseconds{ 33 }, .debounce = false, .trailing = true },
            [this, alive = _alive]() {
                if (alive->load(std::memory_order_acquire) && !_closed && std::exchange(_tfelPending, false))
                {
                    _PublishTFELPreview(false);
                }
            });
        _sui.TFELPreviewChanged([this, alive = _alive](auto&&, bool saved) {
            if (alive->load(std::memory_order_acquire) && !_closed)
            {
                if (saved)
                {
                    _tfelPending = false;
                    _PublishTFELPreview(true);
                }
                else
                {
                    _tfelPending = true;
                    _tfelUpdate->Run();
                }
            }
        });

        // Stash away the current requested theme of the app. We'll need that in
        // _BackgroundBrush() to do a theme-aware resource lookup
        _requestedTheme = settings.GlobalSettings().CurrentTheme(windowSettings).RequestedTheme();
    }

    SettingsPaneContent::~SettingsPaneContent()
    {
        _alive->store(false, std::memory_order_release);
        Close();
    }

    void SettingsPaneContent::_PublishTFELPreview(bool saved)
    {
        const auto settings = _sui.TFELPreviewSettings();
        const auto globals = settings.GlobalSettings();
        TFELPreview::Snapshot snapshot;
        for (const auto& profile : settings.AllProfiles())
        {
#define RESOLVE_TFEL(name) ((profile.Has##name() || profile.name##OverrideSource()) ? profile.name() : globals.name())
            snapshot.emplace(profile.Guid(), TFELPreview::Values{
                RESOLVE_TFEL(TFELBrightness), RESOLVE_TFEL(TFELContrast),
                RESOLVE_TFEL(TFELBlackLevel), RESOLVE_TFEL(TFELGamma),
                RESOLVE_TFEL(TFELBloomIntensity), RESOLVE_TFEL(TFELBloomThreshold),
                RESOLVE_TFEL(TFELBloomRadius), RESOLVE_TFEL(TFELBloomSoftness),
                RESOLVE_TFEL(TFELPhosphorRed), RESOLVE_TFEL(TFELPhosphorGreen), RESOLVE_TFEL(TFELPhosphorBlue),
                RESOLVE_TFEL(TFELHue), RESOLVE_TFEL(TFELSaturation),
                RESOLVE_TFEL(TFELGlowHue), RESOLVE_TFEL(TFELGlowSaturation), RESOLVE_TFEL(TFELGlowMix) });
#undef RESOLVE_TFEL
        }
        TFELPreview::Update(_tfelSession, std::move(snapshot), saved);
    }

    void SettingsPaneContent::UpdateSettings(const CascadiaSettings& settings, const WindowSettings& windowSettings)
    {
        ASSERT_UI_THREAD();
        _tfelPending = false;
        _sui.UpdateSettings(settings, windowSettings);

        _requestedTheme = settings.GlobalSettings().CurrentTheme(windowSettings).RequestedTheme();
    }

    winrt::Windows::UI::Xaml::FrameworkElement SettingsPaneContent::GetRoot()
    {
        return _sui;
    }
    winrt::Windows::Foundation::Size SettingsPaneContent::MinimumSize()
    {
        return { 1, 1 };
    }
    void SettingsPaneContent::Focus(winrt::Windows::UI::Xaml::FocusState reason)
    {
        if (reason != FocusState::Unfocused)
        {
            _sui.as<Controls::Page>().Focus(reason);
        }
    }
    void SettingsPaneContent::Close()
    {
        if (!_closed)
        {
            _closed = true;
            TFELPreview::End(_tfelSession);
        }
    }

    INewContentArgs SettingsPaneContent::GetNewTerminalArgs(const BuildStartupKind /*kind*/) const
    {
        return BaseContentArgs(L"settings");
    }

    winrt::hstring SettingsPaneContent::Icon() const
    {
        // This is the Setting icon (looks like a gear)
        static constexpr std::wstring_view glyph{ L"\xE713" };
        return winrt::hstring{ glyph };
    }

    Windows::Foundation::IReference<winrt::Windows::UI::Color> SettingsPaneContent::TabColor() const noexcept
    {
        return nullptr;
    }

    winrt::Windows::UI::Xaml::Media::Brush SettingsPaneContent::BackgroundBrush()
    {
        // Look up the color we should use for the settings tab item from our
        // resources. This should only be used for when "terminalBackground" is
        // requested.
        static const auto key = winrt::box_value(L"SettingsUiTabBrush");
        // You can't just do a Application::Current().Resources().TryLookup
        // lookup, cause the app theme never changes! Do the hacky version
        // instead.
        return ThemeLookup(Application::Current().Resources(), _requestedTheme, key).try_as<winrt::Windows::UI::Xaml::Media::Brush>();
    }
}
