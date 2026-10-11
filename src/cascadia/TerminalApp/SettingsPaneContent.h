// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once
#include "winrt/TerminalApp.h"
#include "BasicPaneEvents.h"
#include <ThrottledFunc.h>

namespace winrt::TerminalApp::implementation
{
    class SettingsPaneContent : public winrt::implements<SettingsPaneContent, IPaneContent>, public BasicPaneEvents
    {
    public:
        SettingsPaneContent(winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings settings,
                            winrt::Microsoft::Terminal::Settings::Model::WindowSettings windowSettings);
        ~SettingsPaneContent();

        void UpdateSettings(const winrt::Microsoft::Terminal::Settings::Model::CascadiaSettings& settings,
                            const winrt::Microsoft::Terminal::Settings::Model::WindowSettings& windowSettings);

        winrt::Windows::UI::Xaml::FrameworkElement GetRoot();
        winrt::Microsoft::Terminal::Settings::Editor::MainPage SettingsUI() { return _sui; }

        winrt::Windows::Foundation::Size MinimumSize();
        void Focus(winrt::Windows::UI::Xaml::FocusState reason = winrt::Windows::UI::Xaml::FocusState::Programmatic);
        void Close();
        winrt::Microsoft::Terminal::Settings::Model::INewContentArgs GetNewTerminalArgs(const BuildStartupKind kind) const;

        winrt::hstring Title() { return RS_(L"SettingsTab"); }
        uint64_t TaskbarState() { return 0; }
        uint64_t TaskbarProgress() { return 0; }
        bool ReadOnly() { return false; }
        winrt::hstring Icon() const;
        Windows::Foundation::IReference<winrt::Windows::UI::Color> TabColor() const noexcept;
        winrt::Windows::UI::Xaml::Media::Brush BackgroundBrush();

        // See BasicPaneEvents for most generic event definitions

    private:
        void _PublishTFELPreview(bool saved);
        uint64_t _tfelSession{};
        bool _closed{};
        bool _tfelPending{};
        std::shared_ptr<std::atomic_bool> _alive{ std::make_shared<std::atomic_bool>(true) };
        std::shared_ptr<ThrottledFunc<>> _tfelUpdate;
        winrt::Microsoft::Terminal::Settings::Editor::MainPage _sui{ nullptr };
        winrt::Windows::UI::Xaml::ElementTheme _requestedTheme;
    };
}
