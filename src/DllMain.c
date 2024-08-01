#include <initguid.h>
#include <windows.h>
#include <d3d11_1.h>
#include <MinHook.h>

static HRESULT (*IDXGISwapChain_Present)(IDXGISwapChain *, UINT, UINT) = NULL;

static HRESULT Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
    return IDXGISwapChain_Present(This, 0, DXGI_PRESENT_ALLOW_TEARING);
}

static HRESULT (*IDXGIFactory2_CreateSwapChainForCoreWindow)(IDXGIFactory2 *This, IUnknown *pDevice, IUnknown *pWindow,
                                                             const DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                                             IDXGIOutput *pRestrictToOutput,
                                                             IDXGISwapChain1 **ppSwapChain) = NULL;

static HRESULT CreateSwapChainForCoreWindow(IDXGIFactory2 *This, IUnknown *pDevice, IUnknown *pWindow,
                                            DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput,
                                            IDXGISwapChain1 **ppSwapChain)
{
    pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    return IDXGIFactory2_CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static HRESULT (*IDXGISwapChain_ResizeBuffers)(IDXGISwapChain *This, UINT BufferCount, UINT Width, UINT Height,
                                               DXGI_FORMAT NewFormat, UINT SwapChainFlags) = NULL;

static HRESULT ResizeBuffers(IDXGISwapChain *This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                             UINT SwapChainFlags)
{
    return IDXGISwapChain_ResizeBuffers(This, BufferCount, Width, Height, NewFormat,
                                        DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

static DWORD ThreadProc(LPVOID lpParameter)
{
    MH_Initialize();

    IDXGIFactory2 *pFactory = NULL;
    CreateDXGIFactory(&IID_IDXGIFactory2, (void **)&pFactory);
    MH_CreateHook((*(LPVOID **)pFactory)[16], &CreateSwapChainForCoreWindow,
                  (LPVOID *)&IDXGIFactory2_CreateSwapChainForCoreWindow);

    IDXGISwapChain *pSwapChain = NULL;
    HANDLE hInstance = GetModuleHandleW(NULL);
    D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
        &((DXGI_SWAP_CHAIN_DESC){
            .BufferCount = 1,
            .BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM,
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .SampleDesc.Count = 1,
            .Windowed = TRUE,
            .OutputWindow = CreateWindowExW(
                WS_EX_LEFT,
                (LPCWSTR)(ULONG_PTR)(WORD)(RegisterClassW(
                    &((WNDCLASSW){.lpszClassName = L" ", .lpfnWndProc = DefWindowProcW, .hInstance = hInstance}))),
                NULL, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInstance, NULL)}),
        &pSwapChain, NULL, NULL, NULL);

    MH_CreateHook((*(LPVOID **)pSwapChain)[8], &Present, (LPVOID *)&IDXGISwapChain_Present);
    MH_CreateHook((*(LPVOID **)pSwapChain)[13], &ResizeBuffers, (LPVOID *)&IDXGISwapChain_ResizeBuffers);

    return MH_EnableHook(MH_ALL_HOOKS);
}

BOOL DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        CloseHandle(CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL));
    }
    return TRUE;
}