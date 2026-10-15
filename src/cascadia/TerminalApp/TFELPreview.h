// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once
#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace winrt::TerminalApp::implementation
{
    // Process-wide, value-only preview state. XAML controls stay on their owning
    // dispatcher; the settings model is never shared with another UI thread.
    class TFELPreview
    {
    public:
        // Tone[0..3], Bloom[4..7], phosphor RGB[8..10], hue/saturation[11..12],
        // glow hue/saturation/mix[13..15], render mode[16], panel grid[17]. Keep ControlCore::PreviewTFEL in sync.
        using Values = std::array<float, 18>;
        using Snapshot = std::map<winrt::guid, Values>;

        static uint64_t Begin()
        {
            const std::scoped_lock lock{ _mutex };
            return ++_sequence;
        }

        static void Update(uint64_t session, Snapshot values, bool saved)
        {
            const std::scoped_lock lock{ _mutex };
            auto& entry = _sessions[session];
            entry.values = std::move(values);
            entry.sequence = ++_sequence;
            if (saved)
            {
                entry.saved = entry.values;
            }
            _Broadcast();
        }

        static void End(uint64_t session)
        {
            const std::scoped_lock lock{ _mutex };
            if (const auto it = _sessions.find(session); it != _sessions.end())
            {
                if (it->second.saved)
                {
                    // Save already wrote the file, but its reload may still be queued.
                    it->second.values = *it->second.saved;
                }
                else
                {
                    _sessions.erase(it);
                }
                _Broadcast();
            }
        }

        static void Reloaded()
        {
            const std::scoped_lock lock{ _mutex };
            _sessions.clear();
            _Broadcast();
        }

        static void Register(const Microsoft::Terminal::Control::TermControl& control, winrt::guid profile)
        {
            const std::scoped_lock lock{ _mutex };
            _controls.push_back({ control, control.Dispatcher(), profile, std::make_shared<std::atomic<bool>>(true), std::nullopt });
            _Dispatch(_controls.back());
        }

        static void Unregister(const Microsoft::Terminal::Control::TermControl& control)
        {
            const std::scoped_lock lock{ _mutex };
            std::erase_if(_controls, [&](const auto& entry) {
                const auto target = entry.control.get();
                if (!target || target == control)
                {
                    *entry.active = false;
                    return true;
                }
                return false;
            });
        }

    private:
        struct Session
        {
            Snapshot values;
            std::optional<Snapshot> saved;
            uint64_t sequence{};
        };
        struct Target
        {
            winrt::weak_ref<Microsoft::Terminal::Control::TermControl> control;
            Windows::UI::Core::CoreDispatcher dispatcher;
            winrt::guid profile;
            std::shared_ptr<std::atomic<bool>> active;
            std::optional<Values> lastValues;
        };

        static void _Dispatch(Target& target)
        {
            if (!target.control.get() || !target.dispatcher)
            {
                return;
            }
            std::optional<Values> values;
            uint64_t sequence{};
            for (const auto& [id, session] : _sessions)
            {
                if (session.sequence > sequence)
                {
                    if (const auto it = session.values.find(target.profile); it != session.values.end())
                    {
                        sequence = session.sequence;
                        values = it->second;
                    }
                }
            }
            if (values == target.lastValues)
            {
                return;
            }
            target.lastValues = values;
            target.dispatcher.TryRunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                                          [weak = target.control, active = target.active, values]() {
                                              if (!*active)
                                              {
                                                  return;
                                              }
                                              if (const auto control = weak.get())
                                              {
                                                  if (values)
                                                  {
                                                      control.PreviewTFEL(*values);
                                                  }
                                                  else
                                                  {
                                                      control.PreviewTFEL({});
                                                  }
                                              }
                                          });
        }

        static void _Broadcast()
        {
            for (auto& target : _controls)
            {
                _Dispatch(target);
            }
        }

        inline static std::mutex _mutex;
        inline static uint64_t _sequence{};
        inline static std::map<uint64_t, Session> _sessions;
        inline static std::vector<Target> _controls;
    };
}
