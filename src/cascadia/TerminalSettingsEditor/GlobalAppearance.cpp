// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "GlobalAppearance.h"
#include "GlobalAppearance.g.cpp"

#include <WtExeUtils.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <algorithm>
#include <cmath>

using namespace winrt;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Navigation;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Microsoft::Terminal::Settings::Model;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    GlobalAppearance::GlobalAppearance()
    {
        InitializeComponent();
    }

    void GlobalAppearance::OnNavigatedTo(const NavigationEventArgs& e)
    {
        const auto args = e.Parameter().as<Editor::NavigateToPageArgs>();
        _ViewModel = args.ViewModel().as<Editor::GlobalAppearanceViewModel>();
        _activeTFELNumber = {};
        _windowRoot = args.WindowRoot();
        // Populate only after navigation supplies the view model. Select the
        // first item after ItemsSource exists; selection alone does not apply it.
        TFELPresetPicker().ItemsSource(_ViewModel.TFELPresetNames());
        TFELPresetPicker().SelectedIndex(0);
        BringIntoViewWhenLoaded(args.ElementToFocus());

        TraceLoggingWrite(
            g_hTerminalSettingsEditorProvider,
            "NavigatedToPage",
            TraceLoggingDescription("Event emitted when the user navigates to a page in the settings UI"),
            TraceLoggingValue("globalAppearance", "PageId", "The identifier of the page that was navigated to"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage));
    }

    void GlobalAppearance::ApplyTFELPreset_Click(const IInspectable&, const RoutedEventArgs&)
    {
        _ViewModel.ApplyTFELPreset(TFELPresetPicker().SelectedIndex());
    }

    void GlobalAppearance::UpdateTFELPreset_Click(const IInspectable&, const RoutedEventArgs&)
    {
        try
        {
            _ViewModel.UpdateTFELPreset(TFELPresetPicker().SelectedIndex());
            TFELPresetStatus().Text(RS_(L"TFELUpdated"));
        }
        catch (const hresult_error& error) { TFELPresetStatus().Text(error.message()); }
    }

    void GlobalAppearance::TFELNumber_GotFocus(const IInspectable&, const RoutedEventArgs& args)
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

    void GlobalAppearance::TFELNumber_ValueChanged(const Microsoft::UI::Xaml::Controls::NumberBox& box,
                                                  const Microsoft::UI::Xaml::Controls::NumberBoxValueChangedEventArgs& args)
    {
        // One-way binding displays model updates. Only focused user input writes
        // back, including validation on focus loss. NumberBox's inner TextBox
        // owns keyboard focus, so checking the outer control's FocusState is insufficient.
        if (!_ViewModel || !box.IsLoaded() || _activeTFELNumber.get() != box) return;
        const auto key = unbox_value<hstring>(box.Tag());
        const auto value = args.NewValue();
#define TFEL_NUMBER(name) \
        if (key == L## #name) \
        { \
            if (std::isfinite(value)) _ViewModel.name(static_cast<float>(std::clamp(value, box.Minimum(), box.Maximum()))); \
            box.Value(_ViewModel.name()); \
            return; \
        }
        TFEL_NUMBER(TFELBrightness)
        TFEL_NUMBER(TFELContrast)
        TFEL_NUMBER(TFELBlackLevel)
        TFEL_NUMBER(TFELGamma)
        TFEL_NUMBER(TFELBloomIntensity)
        TFEL_NUMBER(TFELBloomThreshold)
        TFEL_NUMBER(TFELBloomRadius)
        TFEL_NUMBER(TFELBloomSoftness)
        TFEL_NUMBER(TFELHue)
        TFEL_NUMBER(TFELSaturation)
        TFEL_NUMBER(TFELGlowHue)
        TFEL_NUMBER(TFELGlowSaturation)
        TFEL_NUMBER(TFELGlowMix)
#undef TFEL_NUMBER
    }

    void GlobalAppearance::CreateTFELPreset_Click(const IInspectable&, const RoutedEventArgs&)
    {
        try
        {
            const auto index = _ViewModel.CreateTFELPreset(TFELPresetName().Text());
            TFELPresetPicker().ItemsSource(_ViewModel.TFELPresetNames());
            TFELPresetPicker().SelectedIndex(index);
            TFELPresetName().Text(L"");
            TFELPresetStatus().Text(RS_(L"TFELCreated"));
        }
        catch (const hresult_error& error)
        {
            TFELPresetStatus().Text(error.message());
        }
    }

    void GlobalAppearance::RenameTFELPreset_Click(const IInspectable&, const RoutedEventArgs&)
    {
        try
        {
            const auto index = TFELPresetPicker().SelectedIndex();
            _ViewModel.RenameTFELPreset(index, TFELPresetName().Text());
            TFELPresetPicker().ItemsSource(_ViewModel.TFELPresetNames());
            TFELPresetPicker().SelectedIndex(index);
            TFELPresetName().Text(L"");
            TFELPresetStatus().Text(RS_(L"TFELRenamed"));
        }
        catch (const hresult_error& error) { TFELPresetStatus().Text(error.message()); }
    }

    void GlobalAppearance::DeleteTFELPreset_Click(const IInspectable&, const RoutedEventArgs&)
    {
        try
        {
            _ViewModel.DeleteTFELPreset(TFELPresetPicker().SelectedIndex());
            TFELPresetPicker().ItemsSource(_ViewModel.TFELPresetNames());
            TFELPresetPicker().SelectedIndex(0);
            TFELPresetStatus().Text(RS_(L"TFELDeleted"));
        }
        catch (const hresult_error& error) { TFELPresetStatus().Text(error.message()); }
    }

    safe_void_coroutine GlobalAppearance::ImportTFELPreset_Click(const IInspectable&, const RoutedEventArgs&)
    {
        const auto lifetime = get_strong();
        // Keep a snapshot of the destination VM across the asynchronous picker.
        const auto viewModel = _ViewModel;
        const auto name = TFELPresetName().Text();
        try
        {
            Windows::Storage::Pickers::FileOpenPicker picker;
            const auto root = _windowRoot.get();
            if (!root || !root.TryPropagateHostingWindow(picker))
            {
                TFELPresetStatus().Text(RS_(L"TFELPickerError"));
                co_return;
            }
            picker.FileTypeFilter().Append(L".json");
            if (const auto file = co_await picker.PickSingleFileAsync())
            {
                const auto properties = co_await file.GetBasicPropertiesAsync();
                if (properties.Size() > 262144) throw hresult_invalid_argument(RS_(L"TFELFileTooLarge"));
                const auto json = co_await Windows::Storage::FileIO::ReadTextAsync(file);
                // Navigation/reload may replace the editor while the picker is open.
                if (viewModel != _ViewModel || !IsLoaded()) co_return;
                const auto index = viewModel.ImportTFELPreset(json, name);
                TFELPresetPicker().ItemsSource(viewModel.TFELPresetNames());
                TFELPresetPicker().SelectedIndex(index);
                TFELPresetName().Text(L"");
                TFELPresetStatus().Text(RS_(L"TFELImported"));
            }
        }
        catch (const hresult_error& error) { TFELPresetStatus().Text(error.message()); }
    }

    safe_void_coroutine GlobalAppearance::ExportTFELPreset_Click(const IInspectable&, const RoutedEventArgs&)
    {
        const auto lifetime = get_strong();
        try
        {
            const auto json = _ViewModel.ExportTFELPreset(TFELPresetPicker().SelectedIndex());
            Windows::Storage::Pickers::FileSavePicker picker;
            const auto root = _windowRoot.get();
            if (!root || !root.TryPropagateHostingWindow(picker))
            {
                TFELPresetStatus().Text(RS_(L"TFELPickerError"));
                co_return;
            }
            picker.SuggestedFileName(L"terminalEL-preset");
            picker.FileTypeChoices().Insert(RS_(L"TFELFileType"), single_threaded_vector<hstring>({ L".json" }));
            if (const auto file = co_await picker.PickSaveFileAsync())
            {
                co_await Windows::Storage::FileIO::WriteTextAsync(file, json);
                TFELPresetStatus().Text(RS_(L"TFELExported"));
            }
        }
        catch (const hresult_error& error)
        {
            TFELPresetStatus().Text(error.message());
        }
    }
}
