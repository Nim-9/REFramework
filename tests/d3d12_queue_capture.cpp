// Real D3D12 WARP/DXGI smoke test. Run on Windows; no RE engine required.
#include "../src/D3D12QueueCapture.hpp"
#include "../src/DXGIFactoryHook.hpp"
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using Microsoft::WRL::ComPtr;
using namespace d3d12::queue_capture;
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
static void check(HRESULT hr, const char* message) {
    if (FAILED(hr)) { std::fprintf(stderr, "%s: %08lx\n", message, (unsigned long)hr); std::exit(1); }
}
static DXGIFactoryHook publication_hook;
static int WINAPI original_call() { return 42; }
static int WINAPI replaced_call() { return publication_hook.get_original<decltype(original_call)*>()(); }

int main(int argc, char** argv) {
    void* slot = reinterpret_cast<void*>(&original_call);
    std::atomic<bool> start{}, stop{}, bad{};
    std::thread caller{[&] {
        start.store(true);
        while (!stop.load()) {
            auto fn = reinterpret_cast<decltype(original_call)*>(InterlockedCompareExchangePointer(&slot, nullptr, nullptr));
            if (fn() != 42) bad.store(true);
        }
    }};
    while (!start.load()) std::this_thread::yield();
    publication_hook.install(&slot, reinterpret_cast<void*>(&replaced_call));
    require(replaced_call() == 42, "callback original must be published");
    stop.store(true); caller.join();
    require(!bad.load(), "concurrent hook entry must call the saved original");
    publication_hook.remove();
    require(slot == reinterpret_cast<void*>(&original_call), "factory slot must restore");

    ComPtr<IDXGIFactory4> factory;
    check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "factory");
    ComPtr<IDXGIAdapter> warp;
    const bool hardware = argc == 2 && std::strcmp(argv[1], "--hardware") == 0;
    if (hardware) {
        check(factory->EnumAdapters(0, &warp), "hardware adapter");
    } else {
        check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)), "WARP adapter");
    }
    ComPtr<ID3D12Device> device;
    check(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)), "D3D12 device");
    D3D12_COMMAND_QUEUE_DESC qd{};
    ComPtr<ID3D12CommandQueue> direct, compute;
    check(device->CreateCommandQueue(&qd, IID_PPV_ARGS(&direct)), "direct queue");
    qd.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    check(device->CreateCommandQueue(&qd, IID_PPV_ARGS(&compute)), "compute queue");
    const auto window = CreateWindowExW(0, L"STATIC", L"Queue capture test", WS_OVERLAPPEDWINDOW,
        0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    require(window != nullptr, "window");
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = desc.Height = 64;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    ComPtr<IDXGISwapChain1> sc1;
    check(factory->CreateSwapChainForHwnd(direct.Get(), window, &desc, nullptr, nullptr, &sc1), "swapchain");
    ComPtr<IDXGISwapChain3> sc3;
    check(sc1.As(&sc3), "swapchain interface alias");
    ComPtr<ID3D12CommandQueue> captured;
    require(FAILED(retrieve(sc3.Get(), device.Get(), captured)) && !captured, "missing association must fail");
    require(FAILED(associate(device.Get(), sc1.Get())), "device must not be treated as a queue");
    require(FAILED(associate(compute.Get(), sc1.Get())), "compute queue must be rejected");
    check(associate(direct.Get(), sc1.Get()), "associate");
    check(retrieve(sc3.Get(), device.Get(), captured), "retrieve through another swapchain interface");
    require(same_object(captured.Get(), direct.Get()), "must retrieve the exact queue identity");
    captured.Reset();
    require(FAILED(retrieve(sc3.Get(), factory.Get(), captured)) && !captured, "wrong device identity must fail");
    check(sc1->SetPrivateDataInterface(key, nullptr), "clear association");
    require(FAILED(retrieve(sc3.Get(), device.Get(), captured)) && !captured, "cleared association must fail");
    sc3.Reset(); sc1.Reset();
    DestroyWindow(window);
    std::puts(hardware ? "Queue capture hardware smoke test passed" : "Queue capture WARP smoke test passed");
}
