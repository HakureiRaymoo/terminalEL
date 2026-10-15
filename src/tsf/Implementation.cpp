// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"
#include "Implementation.h"
#include <cstdio>

#include "Handle.h"
#include "../buffer/out/TextAttribute.hpp"
#include "../renderer/base/renderer.hpp"

#pragma warning(disable : 4100) // '...': unreferenced formal parameter

using namespace Microsoft::Console::TSF;

static void TfSelectionClose(const TF_SELECTION* sel)
{
    if (const auto r = sel->range)
    {
        r->Release();
    }
}
using unique_tf_selection = wil::unique_struct<TF_SELECTION, decltype(&TfSelectionClose), &TfSelectionClose>;

static void TfPropertyvalClose(TF_PROPERTYVAL* val)
{
    VariantClear(&val->varValue);
}
using unique_tf_propertyval = wil::unique_struct<TF_PROPERTYVAL, decltype(&TfPropertyvalClose), &TfPropertyvalClose>;

// The flags passed to ActivateEx don't replace the flags during previous calls.
// Instead, they're additive. So, if we pass flags that some concurrently running
// TSF clients don't expect we may blow them up accidentally.
//
// Such is the case with WPF (and TSF, which is actually at fault there).
// If you pass TF_TMAE_CONSOLE it'll instantly crash on Windows 10 on first text input.
// On Windows 11 it'll at least not crash but still make emoji input completely non-functional.
//
// --------
//
// In any case, we pass the same flags as conhost v1:
// - TF_TMAE_NOACTIVATEKEYBOARDLAYOUT: TSF does not sync the current keyboard layout
//   while this method is called. The keyboard layout will be adjusted when the
//   calling thread gets focus. This flag must be used with TF_TMAE_NOACTIVATETIP.
// - TF_TMAE_CONSOLE: A text service is activated for console usage.
//   TSF uses this flag in CInputContextAdapter (adapts TSF3 TIPs to TSF1 or IMM clients),
//   flags the client as IMM-style, and disables "single character composition" for Korean IMEs.
//   The latter would cause every syllable to be an "interim character": not marked as composing,
//   yet still being rewritten by the IME. Since we send any non-composing text to the shell and
//   can't unsend it, this would break the IME. We could fix that by recognizing `fInterimChar`
//   and only finalizing the text in front of it of course, and conhost v1 did just that
//   (see _IsInterimSelection/_MakeInterimString). But it also set TF_TMAE_CONSOLE,
//   so there were no interim characters in the first place (lol).
//   To test this, type "gksks" with a Korean IME: 하난 is correct, 하나ㄴ is wrong.
//
// ...with the exception of, for the following reason:
// - TF_TMAE_UIELEMENTENABLEDONLY: This flag tells TSF that the caller wants to render its
//   own candidate "window". The required interface for that is `UIElement` and so it only works
//   with TIPs (Text Input Panel) that flag themselves as `GUID_TFCAT_TIPCAP_UIELEMENTENABLED`.
//   We don't support an "UILess" mode (= don't render our own TIP) and so we should not set this flag.
//   See: https://learn.microsoft.com/en-us/windows/win32/tsf/uiless-mode-overview
//
// For TF_TMAE_NOACTIVATEKEYBOARDLAYOUT, I'm 99% sure it doesn't do anything, including in
// conhost v1. This is because IMM will be initialized on WM_ACTIVATE, which calls ActivateEx(0).
// Any subsequent ActivateEx() calls will update the flags, _except_ for this one and
// TF_TMAE_NOACTIVATETIP which are explicitly filtered out.
//
// TF_TMAE_NOACTIVATETIP however is important. Without it, TIPs are immediately initialized.
static std::atomic<DWORD> s_activationFlags{ TF_TMAE_NOACTIVATETIP | TF_TMAE_NOACTIVATEKEYBOARDLAYOUT | TF_TMAE_CONSOLE };
void Implementation::AvoidBuggyTSFConsoleFlags() noexcept
{
    s_activationFlags.fetch_and(~static_cast<DWORD>(TF_TMAE_CONSOLE), std::memory_order_relaxed);
}

static std::atomic<bool> s_wantsAnsiInputScope{ false };
void Implementation::SetDefaultScopeAlphanumericHalfWidth(bool enable) noexcept
{
    s_wantsAnsiInputScope.store(enable, std::memory_order_relaxed);
}

HRESULT Implementation::Initialize()
{
    RETURN_IF_FAILED_EXPECTED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(_categoryMgr.addressof())));
    RETURN_IF_FAILED_EXPECTED(CoCreateInstance(CLSID_TF_DisplayAttributeMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(_displayAttributeMgr.addressof())));
    // There's no point in calling TF_GetThreadMgr. ITfThreadMgr is a per-thread singleton.
    RETURN_IF_FAILED_EXPECTED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(_threadMgrEx.addressof())));

    RETURN_IF_FAILED(_threadMgrEx->ActivateEx(&_clientId, s_activationFlags.load(std::memory_order_relaxed)));
    RETURN_IF_FAILED(_threadMgrEx->CreateDocumentMgr(_documentMgr.addressof()));

    TfEditCookie ecTextStore;
    RETURN_IF_FAILED(_documentMgr->CreateContext(_clientId, 0, static_cast<ITfContextOwnerCompositionSink*>(this), _context.addressof(), &ecTextStore));

    _ownerCompositionServices = _context.try_query<ITfContextOwnerCompositionServices>();

    _contextSource = _context.query<ITfSource>();
    RETURN_IF_FAILED(_contextSource->AdviseSink(IID_ITfContextOwner, static_cast<ITfContextOwner*>(this), &_cookieContextOwner));
    RETURN_IF_FAILED(_contextSource->AdviseSink(IID_ITfTextEditSink, static_cast<ITfTextEditSink*>(this), &_cookieTextEditSink));

    RETURN_IF_FAILED(_documentMgr->Push(_context.get()));
    return S_OK;
}

void Implementation::Uninitialize() noexcept
{
    _endCandidateUI(true);
    if (_cookieUIElementProbe != TF_INVALID_COOKIE)
    {
        _probeEvent(L"stop", 0, S_OK);
        std::ignore = _probeSource->UnadviseSink(_cookieUIElementProbe);
        _cookieUIElementProbe = TF_INVALID_COOKIE;
    }
    _probeSource.reset();
    _provider.reset();

    if (_associatedHwnd)
    {
        wil::com_ptr<ITfDocumentMgr> prev;
        std::ignore = _threadMgrEx->AssociateFocus(_associatedHwnd, nullptr, prev.addressof());
    }

    if (_cookieTextEditSink != TF_INVALID_COOKIE)
    {
        std::ignore = _contextSource->UnadviseSink(_cookieTextEditSink);
    }
    if (_cookieContextOwner != TF_INVALID_COOKIE)
    {
        std::ignore = _contextSource->UnadviseSink(_cookieContextOwner);
    }

    if (_documentMgr)
    {
        std::ignore = _documentMgr->Pop(TF_POPF_ALL);
    }
    if (_threadMgrEx)
    {
        std::ignore = _threadMgrEx->Deactivate();
    }
}

HWND Implementation::FindWindowOfActiveTSF() noexcept
{
    // We don't know what ITfContextOwner we're going to get in
    // the code below and it may very well be us (this instance).
    // It's also possible that our IDataProvider's GetHwnd()
    // implementation calls this FindWindowOfActiveTSF() function.
    // This can result in infinite recursion because we're calling
    // GetWnd() below, which may call GetHwnd(), which may call
    // FindWindowOfActiveTSF(), and so on.
    // By temporarily clearing the _provider we fix that flaw.
    const auto restore = wil::scope_exit([this, provider = std::move(_provider)]() mutable {
        _provider = std::move(provider);
    });

    wil::com_ptr<IEnumTfDocumentMgrs> enumDocumentMgrs;
    if (FAILED_LOG(_threadMgrEx->EnumDocumentMgrs(enumDocumentMgrs.addressof())))
    {
        return nullptr;
    }

    wil::com_ptr<ITfDocumentMgr> document;
    if (FAILED_LOG(enumDocumentMgrs->Next(1, document.addressof(), nullptr)))
    {
        return nullptr;
    }

    wil::com_ptr<ITfContext> context;
    if (FAILED_LOG(document->GetTop(context.addressof())))
    {
        return nullptr;
    }

    wil::com_ptr<ITfContextView> view;
    if (FAILED_LOG(context->GetActiveView(view.addressof())))
    {
        return nullptr;
    }

    HWND hwnd;
    if (FAILED_LOG(view->GetWnd(&hwnd)))
    {
        return nullptr;
    }

    return hwnd;
}

void Implementation::AssociateFocus(IDataProvider* provider)
{
    if (_provider.get() != provider) _endCandidateUI(true);
    _provider = provider;
    _associatedHwnd = _provider->GetHwnd();

    wil::com_ptr<ITfDocumentMgr> prev;
    THROW_IF_FAILED(_threadMgrEx->AssociateFocus(_associatedHwnd, _documentMgr.get(), prev.addressof()));
}

void Implementation::Focus(IDataProvider* provider)
{
    if (_provider.get() != provider) _endCandidateUI(true);
    _provider = provider;
    if (_cookieUIElementProbe != TF_INVALID_COOKIE)
    {
        _probeEvent(L"focus", 0, S_OK);
    }

    THROW_IF_FAILED(_threadMgrEx->SetFocus(_documentMgr.get()));
    if (_candidateProbeEnabled)
    {
        wil::com_ptr<ITfDocumentMgr> focused;
        const auto hr = _threadMgrEx->GetFocus(focused.addressof());
        const auto identity = focused ? focused.try_query<IUnknown>() : wil::com_ptr<IUnknown>{};
        const auto ownIdentity = _documentMgr.try_query<IUnknown>();
        _probeEvent(identity && identity.get() == ownIdentity.get() ? L"focus-verified" : L"focus-mismatch", 0, hr);

        wil::com_ptr<ITfInputProcessorProfileMgr> profiles;
        auto profileHr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                         IID_PPV_ARGS(profiles.addressof()));
        TF_INPUTPROCESSORPROFILE profile{};
        if (SUCCEEDED(profileHr)) profileHr = profiles->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &profile);
        wchar_t message[320];
        swprintf_s(message, L"[terminalEL IME probe] pid=%lu tid=%lu event=input-profile hr=0x%08lX type=%lu lang=0x%04X overlay=%u\n",
                   GetCurrentProcessId(), GetCurrentThreadId(), static_cast<unsigned long>(profileHr),
                   profile.dwProfileType, static_cast<unsigned int>(profile.langid), _candidateOverlayEnabled ? 1u : 0u);
        OutputDebugStringW(message);
    }
}

void Implementation::Unfocus(IDataProvider* provider)
{
    if (!_provider || _provider != provider)
    {
        return;
    }
    if (_cookieUIElementProbe != TF_INVALID_COOKIE)
    {
        _probeEvent(L"unfocus", 0, S_OK);
    }

    _endCandidateUI(true);

    {
        const auto renderer = _provider->GetRenderer();
        const auto renderData = renderer->GetRenderData();

        renderData->LockConsole();
        const auto unlock = wil::scope_exit([&]() {
            renderData->UnlockConsole();
        });

        if (!renderData->tsfPreview.text.empty())
        {
            auto& comp = renderData->tsfPreview;
            comp.text.clear();
            comp.attributes.clear();
            renderer->NotifyPaintFrame();
        }
    }

    _provider.reset();

    if (_compositions > 0 && _ownerCompositionServices)
    {
        std::ignore = _ownerCompositionServices->TerminateComposition(nullptr);
    }
}

bool Implementation::HasActiveComposition() const noexcept
{
    return _compositions > 0;
}

// OnEndComposition() can only request TF_ES_ASYNC, because it runs from within an active
// edit session. If the composition ended due to a relevant terminal key (arrow keys,
// Enter, etc.), the terminal will receive and handle that key input synchronously, before
// we got around to handling the completion asynchronously. This method allows you to
// synchronously flush it from inside the key handler. GH#20244
void Implementation::FlushPendingComposition() noexcept
{
    // Nothing to flush unless a composition has ended and its edit session is still pending.
    if (_compositions > 0 || !_editSessionCompositionUpdate.referenceCount || !_context)
    {
        return;
    }

    // NOTE: _request() would reject this, because the pending TF_ES_ASYNC request still
    // references the proxy. Letting that request run afterwards is harmless, because
    // _doCompositionUpdate() derives everything from the current state of the context.
    HRESULT hr = S_OK;
    LOG_IF_FAILED(_context->RequestEditSession(_clientId, &_editSessionCompositionUpdate, TF_ES_READWRITE | TF_ES_SYNC, &hr));
    LOG_IF_FAILED(hr);
}

// Opt-in for the XAML Terminal host only. No activation flags are changed.
void Implementation::EnableCandidateProbe() noexcept
{
    wchar_t enabled[2]{};
    _candidateProbeEnabled = GetEnvironmentVariableW(L"TERMINALEL_IME_PROBE", enabled, ARRAYSIZE(enabled)) == 1 && enabled[0] == L'1';
    _candidateOverlayEnabled = GetEnvironmentVariableW(L"TERMINALEL_IME_OVERLAY", enabled, ARRAYSIZE(enabled)) == 1 && enabled[0] == L'1';
    // XAML hosts subscribe even when disabled so saved per-profile settings can
    // enable the next candidate session without recreating the shared manager.
    if (_cookieUIElementProbe != TF_INVALID_COOKIE)
    {
        return;
    }

    auto hr = _threadMgrEx->QueryInterface(IID_PPV_ARGS(_probeSource.addressof()));
    if (SUCCEEDED(hr))
    {
        hr = _probeSource->AdviseSink(IID_ITfUIElementSink, static_cast<ITfUIElementSink*>(this), &_cookieUIElementProbe);
    }
    if (FAILED(hr))
    {
        _cookieUIElementProbe = TF_INVALID_COOKIE;
        _probeSource.reset();
    }
    _probeEvent(L"subscribe", 0, hr);
    _probeEvent(_candidateOverlayEnabled ? L"overlay-enabled" : L"overlay-disabled", 0, S_OK);
}

void Implementation::_probeEvent(const wchar_t* event, DWORD id, HRESULT result) const noexcept
{
    if (!_candidateProbeEnabled) return;
    wchar_t message[320];
    swprintf_s(message, L"[terminalEL IME probe] pid=%lu tid=%lu event=%ls id=%lu hr=0x%08lX provider=%u composition=%d\n",
               GetCurrentProcessId(), GetCurrentThreadId(), event, id, static_cast<unsigned long>(result),
               _provider ? 1u : 0u, _compositions);
    OutputDebugStringW(message);
}

STDMETHODIMP Implementation::BeginUIElement(DWORD id, BOOL* show) noexcept
{
    if (!show) return E_POINTER;
    _probeUIElement(id, show);
    return S_OK;
}

STDMETHODIMP Implementation::UpdateUIElement(DWORD id) noexcept
{
    _probeUIElement(id, nullptr);
    return S_OK;
}

STDMETHODIMP Implementation::EndUIElement(DWORD id) noexcept
{
    if (_candidateElement && _candidateId == id) _endCandidateUI(false);
    // End may arrive after the object is unavailable: identify it by ID only.
    _probeEvent(L"end", id, S_OK);
    return S_OK;
}

void Implementation::_probeUIElement(DWORD id, BOOL* show) noexcept
{
    const bool begin = show != nullptr;
    const BOOL incomingShow = show ? *show : FALSE;
    wil::com_ptr<ITfUIElementMgr> manager;
    auto hr = _threadMgrEx->QueryInterface(IID_PPV_ARGS(manager.addressof()));
    wil::com_ptr<ITfUIElement> element;
    if (SUCCEEDED(hr)) hr = manager->GetUIElement(id, element.addressof());
    _probeEvent(begin ? L"begin" : L"update", id, hr);
    if (FAILED(hr) || !element)
    {
        if (_candidateElement && _candidateId == id) _endCandidateUI(true);
        return;
    }

    const auto candidates = element.try_query<ITfCandidateListUIElement>();
    const auto behavior = element.try_query<ITfCandidateListUIElementBehavior>();
    const auto reading = element.try_query<ITfReadingInformationUIElement>();
    wil::com_ptr<ITfDocumentMgr> document;
    const auto documentHr = candidates ? candidates->GetDocumentMgr(document.addressof()) : E_NOINTERFACE;
    // Compare COM identity, not arbitrary interface pointer addresses.
    wil::com_ptr<IUnknown> identity;
    if (document) identity = document.try_query<IUnknown>();
    const auto ownIdentity = _documentMgr.try_query<IUnknown>();
    const bool owned = SUCCEEDED(documentHr) && identity && identity.get() == ownIdentity.get();

    // A registered sink must explicitly allow the UI it does not draw. The
    // observed initial value is FALSE on Microsoft Pinyin; preserving it hid
    // the candidate window. Limit this decision to our active document.
    if (show && owned && _provider)
    {
        *show = TRUE;
        if (_provider->CandidateUIEnabled() && behavior && _beginCandidateUI(id, element.get()))
        {
            *show = FALSE;
            _probeEvent(L"overlay-begin", id, S_OK);
        }
        else
        {
            _probeEvent(L"allow-system-ui", id, S_OK);
        }
    }

    if (!begin && _candidateElement && _candidateId == id)
    {
        ++_candidateRevision;
        _candidateDataDirty = true;
        // Coalesce a burst of TSF notifications on the UI timer instead of
        // rebuilding layout synchronously inside the IME's input callback.
        // The overlay already fetched a complete page. Avoid a second set of
        // cross-process metadata/string calls purely for the observation probe.
        return;
    }
    if (!_candidateProbeEnabled) return;

    UINT count = 0, selection = 0, page = 0, pages = 0;
    HRESULT countHr = E_PENDING, selectionHr = E_PENDING, pageHr = E_PENDING, pagesHr = E_PENDING, stringHr = E_PENDING;
    // Begin need not have populated content. Do not inspect other controls'
    // candidates. Candidate/reading/composition strings are never logged.
    if (!begin && owned && _provider)
    {
        countHr = candidates->GetCount(&count);
        selectionHr = candidates->GetSelection(&selection);
        pageHr = candidates->GetCurrentPage(&page);
        pagesHr = candidates->GetPageIndex(nullptr, 0, &pages);
        if (countHr == S_OK && selectionHr == S_OK && selection < count)
        {
            BSTR text = nullptr;
            stringHr = candidates->GetString(selection, &text);
            SysFreeString(text);
        }
    }
    wchar_t message[768];
    swprintf_s(message, L"[terminalEL IME probe] pid=%lu tid=%lu id=%lu begin=%u incomingShow=%d candidate=%u behavior=%u reading=%u owned=%u docHr=0x%08lX count=%u/0x%08lX selection=%u/0x%08lX page=%u/0x%08lX pages=%u/0x%08lX stringHr=0x%08lX\n",
               GetCurrentProcessId(), GetCurrentThreadId(), id, begin ? 1u : 0u, incomingShow,
               candidates ? 1u : 0u, behavior ? 1u : 0u, reading ? 1u : 0u, owned ? 1u : 0u,
               static_cast<unsigned long>(documentHr), count, static_cast<unsigned long>(countHr),
               selection, static_cast<unsigned long>(selectionHr), page, static_cast<unsigned long>(pageHr),
               pages, static_cast<unsigned long>(pagesHr), static_cast<unsigned long>(stringHr));
    OutputDebugStringW(message);
}

// Keep the candidate object and owning provider together. Never retarget an
// in-flight element to the next pane just because the shared TSF handle focused it.
bool Implementation::_beginCandidateUI(DWORD id, ITfUIElement* element) noexcept
{
    try
    {
        _endCandidateUI(true);
        if (!_provider) return false;
        _candidateProvider = _provider;
        if (!_candidateProvider->BeginCandidateUI())
        {
            _endCandidateUI(false);
            return false;
        }
        _candidateElement = element;
        _candidateId = id;
        _candidateDeadline = GetTickCount64() + 250;
        ++_candidateRevision;
        _candidateDataDirty = true;
        _candidateLayoutDeadline = 0;
        return true;
    }
    catch (...)
    {
        _endCandidateUI(true);
        return false;
    }
}

void Implementation::_endCandidateUI(bool restoreSystem) noexcept
{
    // Clear state before external calls: Show can reenter our sink.
    auto element = std::move(_candidateElement);
    ++_candidateRevision;
    auto provider = std::move(_candidateProvider);
    _candidateDeadline = 0;
    try
    {
        if (provider) provider->EndCandidateUI();
    }
    CATCH_LOG();
    if (restoreSystem && element)
    {
        const auto hr = element->Show(TRUE);
        _probeEvent(L"overlay-system-fallback", _candidateId, hr);
    }
}

void Implementation::RefreshCandidateUI(IDataProvider* provider) noexcept
{
    if (_refreshingCandidates || !_candidateElement || provider != _candidateProvider.get()) return;
    if (!_candidateDataDirty && GetTickCount64() < _candidateLayoutDeadline) return;
    _refreshingCandidates = true;
    const auto reset = wil::scope_exit([&] { _refreshingCandidates = false; });
    const auto element = _candidateElement;
    const auto owner = _candidateProvider;
    try
    {
        THROW_HR_IF(E_ABORT, _provider.get() != provider);
        THROW_HR_IF(E_ABORT, !owner->CandidateUIEnabled());
        wil::com_ptr<ITfDocumentMgr> focusedDocument;
        THROW_IF_FAILED(_threadMgrEx->GetFocus(focusedDocument.addressof()));
        THROW_HR_IF(E_ABORT, !focusedDocument ||
                    focusedDocument.query<IUnknown>().get() != _documentMgr.query<IUnknown>().get());
        if (!_candidateDataDirty)
        {
            THROW_HR_IF(E_ABORT, !owner->RefreshCandidateLayout());
            _candidateLayoutDeadline = GetTickCount64() + 100;
            return;
        }
        // Clear before calling COM so a reentrant Update is not lost.
        _candidateDataDirty = false;
        const auto candidates = element.query<ITfCandidateListUIElement>();
        UINT count = 0;
        const auto countHr = candidates->GetCount(&count);
        // Begin may precede populated data. A UI-thread timer bounds that wait,
        // including IMEs which never send Update while suppressed.
        if ((countHr == E_PENDING || (SUCCEEDED(countHr) && count == 0)) &&
            _candidateDeadline && GetTickCount64() < _candidateDeadline)
        {
            _candidateDataDirty = true;
            return;
        }
        THROW_HR_IF(E_UNEXPECTED, countHr != S_OK);
        THROW_HR_IF(E_UNEXPECTED, count == 0);
        CandidatePage snapshot;
        snapshot.revision = _candidateRevision;
        UINT selection = 0;
        THROW_HR_IF(E_UNEXPECTED, candidates->GetSelection(&selection) != S_OK);
        THROW_HR_IF(E_UNEXPECTED, candidates->GetCurrentPage(&snapshot.page) != S_OK);
        THROW_IF_FAILED(candidates->GetPageIndex(nullptr, 0, &snapshot.pages));
        THROW_HR_IF(E_UNEXPECTED, snapshot.pages == 0 || snapshot.pages > 4096 || snapshot.page >= snapshot.pages);
        std::vector<UINT> starts(snapshot.pages);
        UINT actual = 0;
        THROW_HR_IF(E_UNEXPECTED, candidates->GetPageIndex(starts.data(), snapshot.pages, &actual) != S_OK);
        THROW_HR_IF(E_UNEXPECTED, actual != snapshot.pages || starts.front() != 0);
        for (size_t i = 0; i < starts.size(); ++i)
        {
            THROW_HR_IF(E_UNEXPECTED, starts[i] >= count || (i && starts[i] <= starts[i - 1]));
        }
        const auto first = starts[snapshot.page];
        const auto last = snapshot.page + 1 < snapshot.pages ? starts[snapshot.page + 1] : count;
        THROW_HR_IF(E_UNEXPECTED, last <= first || last - first > 32 || selection < first || selection >= last);
        snapshot.selection = selection - first;
        snapshot.first = first;
        for (auto index = first; index < last; ++index)
        {
            BSTR text = nullptr;
            const auto free = wil::scope_exit([&] { SysFreeString(text); });
            THROW_HR_IF(E_UNEXPECTED, candidates->GetString(index, &text) != S_OK);
            const auto length = SysStringLen(text);
            THROW_HR_IF(E_UNEXPECTED, length == 0 || length > 4096);
            snapshot.entries.emplace_back(text, length);
        }
        // An IME COM call may pump messages and end/change this element.
        if (_candidateElement.get() != element.get() || _candidateProvider.get() != owner.get()) return;
        if (snapshot.revision != _candidateRevision) return;
        _candidateFirst = first;
        _candidateLast = last;
        THROW_HR_IF(E_ABORT, !owner->UpdateCandidateUI(snapshot));
        if (_candidateElement.get() != element.get() || _candidateProvider.get() != owner.get()) return;
        if (_candidateDeadline) _probeEvent(L"overlay-ready", _candidateId, S_OK);
        _candidateDeadline = 0;
        _candidateLayoutDeadline = GetTickCount64() + 100;
    }
    catch (...)
    {
        // Do not tear down a newer element if an external call pumped messages.
        if (_candidateElement.get() == element.get() && _candidateProvider.get() == owner.get())
        {
            _probeEvent(L"overlay-read-failed", _candidateId, wil::ResultFromCaughtException());
            _endCandidateUI(true);
        }
    }
}

void Implementation::SelectCandidate(IDataProvider* provider, uint64_t revision, UINT index) noexcept
{
    if (!_candidateElement || _provider.get() != provider || _candidateProvider.get() != provider ||
        _candidateDataDirty || revision != _candidateRevision || index < _candidateFirst || index >= _candidateLast) return;
    const auto element = _candidateElement;
    try
    {
        THROW_HR_IF(E_ABORT, !provider->CandidateUIEnabled());
        wil::com_ptr<ITfDocumentMgr> focused;
        THROW_IF_FAILED(_threadMgrEx->GetFocus(focused.addressof()));
        THROW_HR_IF(E_ABORT, !focused || focused.query<IUnknown>().get() != _documentMgr.query<IUnknown>().get());
        const auto behavior = element.query<ITfCandidateListUIElementBehavior>();
        if (_candidateElement.get() != element.get() || revision != _candidateRevision || _candidateDataDirty) return;
        THROW_IF_FAILED(behavior->SetSelection(index));
        // SetSelection can synchronously send Update (and thus change revision).
        // End/focus changes must still prevent finalizing an obsolete element.
        if (_candidateElement.get() != element.get() || _provider.get() != provider || _candidateProvider.get() != provider) return;
        THROW_IF_FAILED(behavior->Finalize());
        _probeEvent(L"overlay-click-finalize", 0, S_OK);
    }
    catch (...)
    {
        if (_candidateElement.get() == element.get() && _candidateProvider.get() == provider)
        {
            _probeEvent(L"overlay-click-failed", _candidateId, wil::ResultFromCaughtException());
            _endCandidateUI(true);
        }
    }
}

#pragma region IUnknown

STDMETHODIMP Implementation::QueryInterface(REFIID riid, void** ppvObj) noexcept
{
    if (!ppvObj)
    {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, IID_ITfContextOwner))
    {
        *ppvObj = static_cast<ITfContextOwner*>(this);
    }
    else if (IsEqualGUID(riid, IID_ITfContextOwnerCompositionSink))
    {
        *ppvObj = static_cast<ITfContextOwnerCompositionSink*>(this);
    }
    else if (IsEqualGUID(riid, IID_ITfTextEditSink))
    {
        *ppvObj = static_cast<ITfTextEditSink*>(this);
    }
    else if (IsEqualGUID(riid, IID_ITfUIElementSink))
    {
        *ppvObj = static_cast<ITfUIElementSink*>(this);
    }
    else if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<ITfContextOwner*>(this));
    }
    else
    {
        *ppvObj = nullptr;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE Implementation::AddRef() noexcept
{
    return InterlockedIncrement(&_referenceCount);
}

ULONG STDMETHODCALLTYPE Implementation::Release() noexcept
{
    const auto r = InterlockedDecrement(&_referenceCount);
    if (r == 0)
    {
        delete this;
    }
    return r;
}

#pragma endregion IUnknown

#pragma region ITfContextOwner

STDMETHODIMP Implementation::GetACPFromPoint(const POINT* ptScreen, DWORD dwFlags, LONG* pacp) noexcept
{
    assert(false);
    return E_NOTIMPL;
}

// The returned rectangle is used to position the TSF candidate window.
STDMETHODIMP Implementation::GetTextExt(LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped) noexcept
try
{
    if (prc)
    {
        *prc = _provider ? _provider->GetCursorPosition() : RECT{};
    }

    if (pfClipped)
    {
        *pfClipped = FALSE;
    }

    return S_OK;
}
CATCH_RETURN()

// The returned rectangle is used to activate the touch keyboard.
STDMETHODIMP Implementation::GetScreenExt(RECT* prc) noexcept
try
{
    if (prc)
    {
        *prc = _provider ? _provider->GetViewport() : RECT{};
    }

    return S_OK;
}
CATCH_RETURN()

STDMETHODIMP Implementation::GetStatus(TF_STATUS* pdcs) noexcept
{
    if (pdcs)
    {
        pdcs->dwDynamicFlags = 0;
        // The use of TF_SS_TRANSITORY / TS_SS_TRANSITORY is incredibly important...
        // ...and it has the least complete description:
        // > TS_SS_TRANSITORY: The document is expected to have a short usage cycle.
        //
        // Proper documentation about the flag has been lost and can only be found via archive.org:
        //   http://web.archive.org/web/20140520210042/http://blogs.msdn.com/b/tsfaware/archive/2007/04/25/transitory-contexts.aspx
        // It states:
        // > The most significant difference is that Transitory contexts don't retain state - once you end the composition [...],
        // > any knowledge of the document (or any previous insertions/modifications/etc.) is gone.
        // In other words, non-transitory contexts expect access to previously completed contents, which is something we cannot provide.
        // Because once some text has finished composition we'll immediately send it to the shell via HandleOutput(), which we cannot undo.
        // It's also the primary reason why we cannot use the WinRT CoreTextServices APIs, as they don't set TS_SS_TRANSITORY.
        //
        // Additionally, "short usage cycle" also significantly undersells another importance of the flag:
        // If set, it enables CUAS, the Cicero Unaware Application Support, which is an emulation layer that fakes IMM32.
        // Cicero is the internal code name for TSF. In other words, "TS_SS_TRANSITORY" = "Disable modern TSF".
        // This results in a couple modern composition features not working (Korean reconversion primarily),
        // but it's a trade-off we're forced to make, because otherwise it doesn't work at all.
        //
        // TS_SS_NOHIDDENTEXT tells TSF that we don't support TS_RT_HIDDEN, which is used if a document contains hidden markup
        // inside the text. For instance an HTML document contains tags which aren't visible, but nonetheless exist.
        // It's not publicly documented, but allegedly specifying this flag results in a minor performance uplift.
        // Ironically, the only two places that mention this flag internally state:
        // > perf: we could check TS_SS_NOHIDDENTEXT for better perf
        pdcs->dwStaticFlags = TS_SS_TRANSITORY | TS_SS_NOHIDDENTEXT;
    }

    return S_OK;
}

STDMETHODIMP Implementation::GetWnd(HWND* phwnd) noexcept
{
    *phwnd = _provider ? _provider->GetHwnd() : nullptr;
    return S_OK;
}

STDMETHODIMP Implementation::GetAttribute(REFGUID rguidAttribute, VARIANT* pvarValue) noexcept
{
    if (s_wantsAnsiInputScope.load(std::memory_order_relaxed) && IsEqualGUID(rguidAttribute, GUID_PROP_INPUTSCOPE))
    {
        _ansiInputScope.AddRef();
        pvarValue->vt = VT_UNKNOWN;
        pvarValue->punkVal = &_ansiInputScope;
        return S_OK;
    }

    pvarValue->vt = VT_EMPTY;
    return S_OK;
}

#pragma endregion ITfContextOwner

#pragma region ITfContextOwnerCompositionSink

STDMETHODIMP Implementation::OnStartComposition(ITfCompositionView* pComposition, BOOL* pfOk) noexcept
try
{
    _compositions++;
    *pfOk = TRUE;
    _probeEvent(L"composition-start", 0, S_OK);
    return S_OK;
}
CATCH_RETURN()

STDMETHODIMP Implementation::OnUpdateComposition(ITfCompositionView* pComposition, ITfRange* pRangeNew) noexcept
{
    _probeEvent(L"composition-update", 0, S_OK);
    return S_OK;
}

STDMETHODIMP Implementation::OnEndComposition(ITfCompositionView* pComposition) noexcept
try
{
    if (_compositions <= 0)
    {
        return E_FAIL;
    }

    _compositions--;
    _probeEvent(L"composition-end", 0, S_OK);
    if (_compositions == 0)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfcontext-requesteditsession
        // > A text service can request an edit session within the context of an existing edit session,
        // > provided a write access session is not requested within a read-only session.
        // --> Requires TF_ES_ASYNC to work properly. TF_ES_ASYNCDONTCARE randomly fails because... TSF.
        std::ignore = _request(_editSessionCompositionUpdate, TF_ES_READWRITE | TF_ES_ASYNC);
    }

    return S_OK;
}
CATCH_RETURN()

#pragma endregion ITfContextOwnerCompositionSink

#pragma region ITfTextEditSink

STDMETHODIMP Implementation::OnEndEdit(ITfContext* pic, TfEditCookie ecReadOnly, ITfEditRecord* pEditRecord) noexcept
try
{
    if (_compositions == 1)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/msctf/nf-msctf-itfcontext-requesteditsession
        // > A text service can request an edit session within the context of an existing edit session,
        // > provided a write access session is not requested within a read-only session.
        // --> Requires TF_ES_ASYNC to work properly. TF_ES_ASYNCDONTCARE randomly fails because... TSF.
        std::ignore = _request(_editSessionCompositionUpdate, TF_ES_READWRITE | TF_ES_ASYNC);
    }

    return S_OK;
}
CATCH_RETURN()

#pragma endregion ITfTextEditSink

Implementation::EditSessionProxyBase::EditSessionProxyBase(Implementation* self) noexcept :
    self{ self }
{
}

STDMETHODIMP Implementation::EditSessionProxyBase::QueryInterface(REFIID riid, void** ppvObj) noexcept
{
    if (!ppvObj)
    {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, IID_ITfEditSession))
    {
        *ppvObj = static_cast<ITfEditSession*>(this);
    }
    else if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IUnknown*>(this);
    }
    else
    {
        *ppvObj = nullptr;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE Implementation::EditSessionProxyBase::AddRef() noexcept
{
    InterlockedIncrement(&referenceCount);
    return self->AddRef();
}

ULONG STDMETHODCALLTYPE Implementation::EditSessionProxyBase::Release() noexcept
{
    InterlockedDecrement(&referenceCount);
    return self->Release();
}

Implementation::AnsiInputScope::AnsiInputScope(Implementation* self) noexcept :
    self{ self }
{
}

HRESULT Implementation::AnsiInputScope::QueryInterface(const IID& riid, void** ppvObj) noexcept
{
    if (!ppvObj)
    {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, IID_ITfInputScope))
    {
        *ppvObj = static_cast<ITfInputScope*>(this);
    }
    else if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObj = static_cast<IUnknown*>(this);
    }
    else
    {
        *ppvObj = nullptr;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG Implementation::AnsiInputScope::AddRef() noexcept
{
    return self->AddRef();
}

ULONG Implementation::AnsiInputScope::Release() noexcept
{
    return self->Release();
}

HRESULT Implementation::AnsiInputScope::GetInputScopes(InputScope** pprgInputScopes, UINT* pcCount) noexcept
{
    const auto scopes = static_cast<InputScope*>(CoTaskMemAlloc(1 * sizeof(InputScope)));
    if (!scopes)
    {
        return E_OUTOFMEMORY;
    }

    scopes[0] = IS_ALPHANUMERIC_HALFWIDTH;

    *pprgInputScopes = scopes;
    *pcCount = 1;
    return S_OK;
}

HRESULT Implementation::AnsiInputScope::GetPhrase(BSTR** ppbstrPhrases, UINT* pcCount) noexcept
{
    return E_NOTIMPL;
}

HRESULT Implementation::AnsiInputScope::GetRegularExpression(BSTR* pbstrRegExp) noexcept
{
    return E_NOTIMPL;
}

HRESULT Implementation::AnsiInputScope::GetSRGS(BSTR* pbstrSRGS) noexcept
{
    return E_NOTIMPL;
}

HRESULT Implementation::AnsiInputScope::GetXML(BSTR* pbstrXML) noexcept
{
    return E_NOTIMPL;
}

[[nodiscard]] HRESULT Implementation::_request(EditSessionProxyBase& session, DWORD flags) const
{
    // Some of the sessions are async, and we don't want to send another request if one is still in flight.
    if (session.referenceCount)
    {
        return S_FALSE;
    }

    HRESULT hr = S_OK;
    THROW_IF_FAILED(_context->RequestEditSession(_clientId, &session, flags, &hr));
    RETURN_IF_FAILED(hr);
    return S_OK;
}

void Implementation::_doCompositionUpdate(TfEditCookie ec)
{
    wil::com_ptr<ITfRange> fullRange;
    LONG fullRangeLength;
    THROW_IF_FAILED(_context->GetStart(ec, fullRange.addressof()));
    THROW_IF_FAILED(fullRange->ShiftEnd(ec, LONG_MAX, &fullRangeLength, nullptr));

    std::wstring finalizedString;
    std::wstring activeComposition;
    til::small_vector<Render::CompositionRange, 2> activeCompositionRanges;
    bool activeCompositionEncountered = false;

    const GUID* guids[] = { &GUID_PROP_COMPOSING, &GUID_PROP_ATTRIBUTE };
    wil::com_ptr<ITfReadOnlyProperty> props;
    THROW_IF_FAILED(_context->TrackProperties(&guids[0], ARRAYSIZE(guids), nullptr, 0, props.addressof()));

    wil::com_ptr<IEnumTfRanges> enumRanges;
    THROW_IF_FAILED(props->EnumRanges(ec, enumRanges.addressof(), fullRange.get()));

    // IEnumTfRanges::Next returns S_FALSE when it has reached the end of the list.
    // This includes any call where the number of returned items is less than what was requested.
    for (HRESULT nextResult = S_OK; nextResult == S_OK;)
    {
        ITfRange* ranges[8];
        ULONG rangesCount;
        nextResult = enumRanges->Next(ARRAYSIZE(ranges), &ranges[0], &rangesCount);

        const auto cleanup = wil::scope_exit([&] {
            for (ULONG i = 0; i < rangesCount; ++i)
            {
                ranges[i]->Release();
            }
        });

        for (ULONG i = 0; i < rangesCount; ++i)
        {
            const auto range = ranges[i];

            bool composing = false;
            TfGuidAtom atom = TF_INVALID_GUIDATOM;
            {
                wil::unique_variant var;
                THROW_IF_FAILED(props->GetValue(ec, range, var.addressof()));

                wil::com_ptr<IEnumTfPropertyValue> propVal;
                wil::com_query_to(var.punkVal, propVal.addressof());

                unique_tf_propertyval propVals[2];
                THROW_IF_FAILED(propVal->Next(2, propVals[0].addressof(), nullptr));

                for (const auto& val : propVals)
                {
                    if (IsEqualGUID(val.guidId, GUID_PROP_COMPOSING))
                    {
                        composing = V_VT(&val.varValue) == VT_I4 && V_I4(&val.varValue) != 0;
                    }
                    else if (IsEqualGUID(val.guidId, GUID_PROP_ATTRIBUTE))
                    {
                        atom = V_VT(&val.varValue) == VT_I4 ? static_cast<TfGuidAtom>(V_I4(&val.varValue)) : TF_INVALID_GUIDATOM;
                    }
                }
            }

            // Since we can't un-finalize finalized text, we only finalize text that's at the start of the document.
            // In other words, don't put text that's in the middle between two active compositions into the finalized string.
            const auto isActiveComposition = composing || activeCompositionEncountered;
            auto& target = isActiveComposition ? activeComposition : finalizedString;

            size_t totalLen = 0;
            for (;;)
            {
                // GetText() won't throw if the range is empty. It'll simply return len == 0.
                // However, you'll likely never see this happen with a bufCap this large (try 16 instead or something).
                // It seems TSF doesn't support such large compositions in any language.
                static constexpr ULONG bufCap = 128;
                WCHAR buf[bufCap];
                ULONG len = bufCap;
                THROW_IF_FAILED(range->GetText(ec, TF_TF_MOVESTART, buf, len, &len));

                target.append(buf, len);
                totalLen += len;

                if (len < bufCap)
                {
                    break;
                }
            }

            if (isActiveComposition)
            {
                activeCompositionRanges.emplace_back(totalLen, _textAttributeFromAtom(atom));
            }

            activeCompositionEncountered |= composing;
        }
    }

    LONG cursorPos = LONG_MAX;
    {
        // According to the docs this may result in TF_E_NOSELECTION. While I haven't actually seen that happen myself yet,
        // I don't want this to result in log-spam, which is why this doesn't use SUCCEEDED_LOG().
        unique_tf_selection sel;
        ULONG selCount;
        if (SUCCEEDED(_context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &sel, &selCount)) && selCount == 1)
        {
            wil::com_ptr<ITfRange> start;
            THROW_IF_FAILED(_context->GetStart(ec, start.addressof()));

            TF_HALTCOND hc{
                .pHaltRange = sel.range,
                .aHaltPos = sel.style.ase == TF_AE_START ? TF_ANCHOR_START : TF_ANCHOR_END,
            };
            THROW_IF_FAILED(start->ShiftEnd(ec, LONG_MAX, &cursorPos, &hc));
        }

        // Compensate for the fact that we'll be erasing the start of the string below.
        cursorPos -= static_cast<LONG>(finalizedString.size());
        cursorPos = std::clamp(cursorPos, 0l, static_cast<LONG>(activeComposition.size()));
    }

    if (!finalizedString.empty())
    {
        // Erase the text that's done with composition from the context.
        wil::com_ptr<ITfRange> range;
        LONG cch;
        THROW_IF_FAILED(_context->GetStart(ec, range.addressof()));
        THROW_IF_FAILED(range->ShiftEnd(ec, static_cast<LONG>(finalizedString.size()), &cch, nullptr));
        THROW_IF_FAILED(range->SetText(ec, 0, nullptr, 0));
    }

    if (_provider)
    {
        {
            const auto renderer = _provider->GetRenderer();
            const auto renderData = renderer->GetRenderData();

            renderData->LockConsole();
            const auto unlock = wil::scope_exit([&]() {
                renderData->UnlockConsole();
            });

            auto& comp = renderData->tsfPreview;
            comp.text = std::move(activeComposition);
            comp.attributes = std::move(activeCompositionRanges);
            // The code block above that calculates the `cursorPos` will clamp it to a positive number.
            comp.cursorPos = static_cast<size_t>(cursorPos);
            renderer->NotifyPaintFrame();
        }

        if (!finalizedString.empty())
        {
            _provider->HandleOutput(finalizedString);
        }
    }
}

TextAttribute Implementation::_textAttributeFromAtom(TfGuidAtom atom) const
{
    TextAttribute attr;

    // You get TF_INVALID_GUIDATOM by (for instance) using the Vietnamese Telex IME.
    // A dashed underline is used because that's what Firefox used at the time and it
    // looked kind of neat. In the past, conhost used a blue background and white text.
    if (atom == TF_INVALID_GUIDATOM)
    {
        attr.SetUnderlineStyle(UnderlineStyle::DashedUnderlined);
        return attr;
    }

    GUID guid;
    if (FAILED_LOG(_categoryMgr->GetGUID(atom, &guid)))
    {
        return attr;
    }

    wil::com_ptr<ITfDisplayAttributeInfo> dai;
    if (FAILED_LOG(_displayAttributeMgr->GetDisplayAttributeInfo(guid, dai.addressof(), nullptr)))
    {
        return attr;
    }

    TF_DISPLAYATTRIBUTE da;
    THROW_IF_FAILED(dai->GetAttributeInfo(&da));

    // The Tencent QQPinyin IME creates TF_CT_COLORREF attributes with a color of 0x000000 (black).
    // We respect their wish, which results in the preview text being invisible.
    // (Note that sending this COLORREF is incorrect, and not a bug in our handling.)
    //
    // After some discussion, we realized that an IME which sets only one color but not
    // the others is likely not properly tested anyway, so we reject those cases.
    // After all, what behavior do we expect, if the IME sends e.g. foreground=blue,
    // without knowing whether our terminal theme already uses a blue background?
    if (da.crText.type != TF_CT_NONE && da.crText.type == da.crBk.type)
    {
        attr.SetForeground(_colorFromDisplayAttribute(da.crText));
        attr.SetBackground(_colorFromDisplayAttribute(da.crBk));
        // I'm not sure what the best way to handle this is.
        if (da.crText.type == da.crLine.type)
        {
            attr.SetUnderlineColor(_colorFromDisplayAttribute(da.crLine));
        }
    }
    if (da.lsStyle >= TF_LS_NONE && da.lsStyle <= TF_LS_SQUIGGLE)
    {
        static constexpr UnderlineStyle lut[] = {
            /* TF_LS_NONE     */ UnderlineStyle::NoUnderline,
            /* TF_LS_SOLID    */ UnderlineStyle::SinglyUnderlined,
            /* TF_LS_DOT      */ UnderlineStyle::DottedUnderlined,
            /* TF_LS_DASH     */ UnderlineStyle::DashedUnderlined,
            /* TF_LS_SQUIGGLE */ UnderlineStyle::CurlyUnderlined,
        };
        attr.SetUnderlineStyle(lut[da.lsStyle]);
    }
    // You can reproduce bold lines with the Japanese IME by typing "kyouhaishaheiku" and pressing space.
    // The IME will allow you to navigate between the 3 parts of the composition and the current one is
    // marked as fBoldLine. We don't support bold lines so we just use a double underline instead.
    if (da.fBoldLine)
    {
        attr.SetUnderlineStyle(UnderlineStyle::DoublyUnderlined);
    }

    return attr;
}

COLORREF Implementation::_colorFromDisplayAttribute(TF_DA_COLOR color)
{
    switch (color.type)
    {
    case TF_CT_SYSCOLOR:
        return GetSysColor(color.nIndex);
    case TF_CT_COLORREF:
        return color.cr;
    default:
        // If you get here you either called this when .type is TF_CT_NONE
        // (don't call in that case; there's no color to be had), or
        // there's a new .type which you need to add.
        assert(false);
        return 0;
    }
}
