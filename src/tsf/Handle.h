// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace Microsoft::Console::Render
{
    class Renderer;
}

namespace Microsoft::Console::TSF
{
    struct Implementation;

    struct CandidatePage
    {
        std::vector<std::wstring> entries;
        UINT selection = 0; // Relative to this page.
        UINT page = 0;
        UINT pages = 0;
        UINT first = 0;
        uint64_t revision = 0;
    };

    // It is fine for any of the IDataProvider functions to throw.
    // However, this doesn't apply to the IUnknown ones.
    //
    // NOTE: This is a pure virtual interface, just like those for COM.
    // It cannot have a `~IDataProvider() = default;` destructor, because then it would need a vtable.
    MIDL_INTERFACE("A86B8AAF-1531-40F5-95BB-611AA9DBDC18")
    IDataProvider : IUnknown
    {
        virtual HWND GetHwnd() = 0;
        virtual RECT GetViewport() = 0;
        virtual RECT GetCursorPosition() = 0;
        virtual void HandleOutput(std::wstring_view text) = 0;
        virtual Render::Renderer* GetRenderer() = 0;
        // Optional XAML-host extension. Other hosts keep the system candidate UI.
        virtual bool BeginCandidateUI() { return false; }
        virtual bool CandidateUIEnabled() noexcept { return false; }
        virtual bool UpdateCandidateUI(const CandidatePage&) { return false; }
        virtual bool RefreshCandidateLayout() { return false; }
        virtual void EndCandidateUI() {}
    };

    // A pimpl idiom wrapper for `Implementation` so that we don't pull in all the TSF headers everywhere.
    // Simultaneously it allows us to handle AdviseSink/UnadviseSink properly, because those hold strong
    // references on `Implementation` which results in an (unfortunate but intentional) reference cycle.
    struct Handle
    {
        static Handle Create();
        static void AvoidBuggyTSFConsoleFlags();
        static void SetDefaultScopeAlphanumericHalfWidth(bool enable);

        Handle() = default;
        ~Handle();
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        Handle(Handle&& other) noexcept;
        Handle& operator=(Handle&& other) noexcept;

        explicit operator bool() const noexcept;
        HWND FindWindowOfActiveTSF() const noexcept;
        void AssociateFocus(IDataProvider* provider) const;
        void Focus(IDataProvider* provider) const;
        void Unfocus(IDataProvider* provider) const;
        bool HasActiveComposition() const noexcept;
        void FlushPendingComposition() const noexcept;
        void EnableCandidateProbe() const noexcept;
        void RefreshCandidateUI(IDataProvider* provider) const noexcept;
        void SelectCandidate(IDataProvider* provider, uint64_t revision, UINT index) const noexcept;

    private:
        void _destroy() noexcept;

        Implementation* _impl = nullptr;
    };
}
