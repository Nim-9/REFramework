#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

namespace d3d12::queue_capture {
// A private-data key, never an object-layout offset. DXGI owns this reference.
inline constexpr GUID key{
    0x9d34db89, 0x75ed, 0x462c, {0x9e, 0xd7, 0x70, 0x2e, 0x97, 0x93, 0x4d, 0x8b}};

template<typename T> inline bool same_interface(T* lhs, T* rhs) {
    if (lhs == nullptr || rhs == nullptr) return false;
    Microsoft::WRL::ComPtr<IUnknown> a, b;
    const auto a_hr = lhs->QueryInterface(IID_PPV_ARGS(&a));
    const auto b_hr = rhs->QueryInterface(IID_PPV_ARGS(&b));
    if (SUCCEEDED(a_hr) && SUCCEEDED(b_hr)) return a.Get() == b.Get();
    // D3DMetal 4.0b3 was observed returning E_NOINTERFACE for device IUnknown.
    // Both inputs must already be the SAME requested, validated interface type.
    // Exact equality of these live interface pointers is sufficient; unequal
    // pointers are rejected rather than inferring identity from memory/refcounts.
    return lhs == rhs;
}

inline HRESULT validate(ID3D12CommandQueue* queue, IUnknown* device) {
    if (queue == nullptr || queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT) return E_INVALIDARG;
    Microsoft::WRL::ComPtr<ID3D12Device> queue_device;
    const auto hr = queue->GetDevice(IID_PPV_ARGS(&queue_device));
    if (FAILED(hr)) return hr;
    Microsoft::WRL::ComPtr<ID3D12Device> render_device;
    const auto device_hr = device == nullptr ? E_POINTER : device->QueryInterface(IID_PPV_ARGS(&render_device));
    if (FAILED(device_hr)) return device_hr;
    return same_interface(queue_device.Get(), render_device.Get()) ? S_OK : E_INVALIDARG;
}

inline HRESULT associate(IUnknown* device_or_queue, IDXGISwapChain* swapchain) {
    if (device_or_queue == nullptr || swapchain == nullptr) return E_INVALIDARG;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    auto hr = device_or_queue->QueryInterface(IID_PPV_ARGS(&queue));
    if (FAILED(hr)) return hr; // D3D11 passes a device, never reinterpret it as a queue.
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    hr = swapchain->GetDevice(IID_PPV_ARGS(&device));
    if (FAILED(hr)) return hr;
    hr = validate(queue.Get(), device.Get());
    if (FAILED(hr)) return hr;
    return swapchain->SetPrivateDataInterface(key, queue.Get());
}

inline HRESULT retrieve(IDXGISwapChain* swapchain, IUnknown* device,
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>& queue) {
    queue.Reset();
    if (swapchain == nullptr || device == nullptr) return E_INVALIDARG;
    Microsoft::WRL::ComPtr<IUnknown> stored;
    UINT size = sizeof(IUnknown*);
    auto hr = swapchain->GetPrivateData(key, &size, stored.GetAddressOf());
    if (FAILED(hr)) return hr;
    if (size != sizeof(IUnknown*) || stored == nullptr) return E_INVALIDARG;
    hr = stored.As(&queue);
    if (FAILED(hr)) return hr;
    hr = validate(queue.Get(), device);
    if (FAILED(hr)) queue.Reset();
    return hr;
}
} // namespace d3d12::queue_capture
