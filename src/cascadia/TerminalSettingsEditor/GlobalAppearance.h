// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "GlobalAppearance.g.h"
#include "Utils.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct GlobalAppearance : public HasScrollViewer<GlobalAppearance>, GlobalAppearanceT<GlobalAppearance>
    {
    public:
        GlobalAppearance();
        void ApplyTFELPreset_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void CreateTFELPreset_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        safe_void_coroutine ExportTFELPreset_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        safe_void_coroutine ImportTFELPreset_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void RenameTFELPreset_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void DeleteTFELPreset_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void UpdateTFELPreset_Click(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);
        void TFELNumber_ValueChanged(const Microsoft::UI::Xaml::Controls::NumberBox&, const Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs&);
        void TFELNumber_GotFocus(const Windows::Foundation::IInspectable&, const Windows::UI::Xaml::RoutedEventArgs&);

        void OnNavigatedTo(const winrt::Windows::UI::Xaml::Navigation::NavigationEventArgs& e);

        til::property_changed_event PropertyChanged;
        WINRT_OBSERVABLE_PROPERTY(Editor::GlobalAppearanceViewModel, ViewModel, PropertyChanged.raise, nullptr);
    private:
        winrt::weak_ref<Editor::IHostedInWindow> _windowRoot;
        winrt::weak_ref<Microsoft::UI::Xaml::Controls::NumberBox> _activeTFELNumber;
    };
}

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(GlobalAppearance);
}
