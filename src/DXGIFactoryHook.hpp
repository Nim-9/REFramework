#pragma once
#include <windows.h>
#include <atomic>
#include <stdexcept>

// Persistent factory hooks can be entered while startup installs them. Publish
// the original before changing the slot, rather than from a unique_ptr constructor
// whose result has not yet been assigned to the callback's global owner.
class DXGIFactoryHook {
public:
    DXGIFactoryHook() = default;
    DXGIFactoryHook(const DXGIFactoryHook&) = delete;
    DXGIFactoryHook& operator=(const DXGIFactoryHook&) = delete;
    ~DXGIFactoryHook() { remove(); }

    bool installed() const { return m_installed; }

    void install(void** slot, void* replacement) {
        if (m_installed) return; // Installation is serialized by the startup/hook monitor.
        DWORD protection{};
        if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &protection)) {
            throw std::runtime_error("DXGI factory slot VirtualProtect failed");
        }
        m_slot = slot;
        m_replacement = replacement;
        for (;;) {
            auto original = InterlockedCompareExchangePointer(slot, nullptr, nullptr);
            if (original == replacement) {
                DWORD ignored{};
                VirtualProtect(slot, sizeof(void*), protection, &ignored);
                throw std::runtime_error("DXGI factory callback already installed without original");
            }
            m_original.store(original, std::memory_order_release);
            if (InterlockedCompareExchangePointer(slot, replacement, original) == original) break;
        }
        m_installed = true;
        DWORD ignored{};
        VirtualProtect(slot, sizeof(void*), protection, &ignored);
    }

    void remove() noexcept {
        if (!m_installed) return;
        DWORD protection{};
        if (!VirtualProtect(m_slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &protection)) return;
        // Preserve a later overlay's hook instead of overwriting its replacement.
        InterlockedCompareExchangePointer(m_slot, m_original.load(std::memory_order_acquire), m_replacement);
        DWORD ignored{};
        VirtualProtect(m_slot, sizeof(void*), protection, &ignored);
        m_installed = false;
        // Original remains valid for an already-entered callback.
    }

    template<typename T> T get_original() const {
        return reinterpret_cast<T>(m_original.load(std::memory_order_acquire));
    }
private:
    void** m_slot{};
    void* m_replacement{};
    std::atomic<void*> m_original{};
    bool m_installed{};
};
