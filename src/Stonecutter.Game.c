#include <initguid.h>
#include <windows.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <MinHook.h>
#include <windows.ui.core.h>

LONG NtQueryTimerResolution(PINT, PINT, PINT);

LONG NtSetTimerResolution(INT, BOOLEAN, PINT);

__x_ABI_CWindows_CUI_CCore_CICoreWindow *pCoreWindow = NULL;

__x_ABI_CWindows_CUI_CCore_CICoreWindow2 *pCoreWindow2 = NULL;

BOOL _ = FALSE, $ = FALSE;

HRESULT(*_put_PointerCursor)
(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CUI_CCore_CICoreCursor *value) = NULL;

HRESULT(*_ResizeBuffers)
(IDXGISwapChain *This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) = NULL;

HRESULT (*_Present)(IDXGISwapChain *, UINT, UINT) = NULL;

HRESULT(*_CreateSwapChainForCoreWindow)
(IDXGIFactory2 *This, IUnknown *pDevice, IUnknown *pWindow, const DXGI_SWAP_CHAIN_DESC1 *pDesc,
 IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain) = NULL;

HRESULT put_PointerCursor(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CUI_CCore_CICoreCursor *value)
{
    __x_ABI_CWindows_CFoundation_CRect _ = {};
    pCoreWindow->lpVtbl->get_Bounds(pCoreWindow, &_);
    pCoreWindow2->lpVtbl->put_PointerPosition(
        pCoreWindow2, (__x_ABI_CWindows_CFoundation_CPoint){_.X + (_.Width / 2), _.Y + (_.Height / 2)});
    return _put_PointerCursor ? _put_PointerCursor(This, value) : S_OK;
}

HRESULT CreateSwapChainForCoreWindow(IDXGIFactory2 *This, IUnknown *pDevice, IUnknown *pWindow,
                                     DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput,
                                     IDXGISwapChain1 **ppSwapChain)
{
    ID3D12CommandQueue *pCommandQueue = NULL;
    if (_ && !pDevice->lpVtbl->QueryInterface(pDevice, &IID_ID3D12CommandQueue, (void **)&pCommandQueue))
    {
        pCommandQueue->lpVtbl->Release(pCommandQueue);
        return DXGI_ERROR_INVALID_CALL;
    }

    if (!$)
    {
        pCoreWindow = (__x_ABI_CWindows_CUI_CCore_CICoreWindow *)pWindow;
        pWindow->lpVtbl->QueryInterface(pWindow, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindow2, (void **)&pCoreWindow2);
        put_PointerCursor(NULL, NULL);

        MH_CreateHook((*(LPVOID **)pWindow)[15], &put_PointerCursor, (LPVOID *)&_put_PointerCursor);
        MH_EnableHook(MH_ALL_HOOKS);
    }

    $ = pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    return _CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

HRESULT Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
    return $ ? _Present(This, 0, DXGI_PRESENT_ALLOW_TEARING) : DXGI_ERROR_DEVICE_RESET;
}

HRESULT ResizeBuffers(IDXGISwapChain *This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                      UINT SwapChainFlags)
{
    return _ResizeBuffers(This, BufferCount, Width, Height, NewFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

DWORD ThreadProc(LPVOID lpParameter)
{
    IDXGIFactory2 *pFactory = NULL;
    CreateDXGIFactory(&IID_IDXGIFactory2, (void **)&pFactory);

    IDXGISwapChain *pSwapChain = NULL;
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
                                  &((DXGI_SWAP_CHAIN_DESC){.BufferCount = 1,
                                                           .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                           .SampleDesc.Count = 1,
                                                           .Windowed = TRUE,
                                                           .OutputWindow = GetDesktopWindow()}),
                                  &pSwapChain, NULL, NULL, NULL);

    MH_Initialize();
    MH_CreateHook((*(LPVOID **)pFactory)[16], &CreateSwapChainForCoreWindow, (LPVOID *)&_CreateSwapChainForCoreWindow);
    MH_CreateHook((*(LPVOID **)pSwapChain)[8], &Present, (LPVOID *)&_Present);
    MH_CreateHook((*(LPVOID **)pSwapChain)[13], &ResizeBuffers, (LPVOID *)&_ResizeBuffers);
    MH_EnableHook(MH_ALL_HOOKS);

    pFactory->lpVtbl->Release(pFactory);
    pSwapChain->lpVtbl->Release(pSwapChain);
    return 0;
}

BOOL DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter.Game");
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            return !CloseHandle(hMutex);

        NtQueryTimerResolution(&((INT){}), &_, &((INT){}));
        NtSetTimerResolution(_, TRUE, &((INT){}));

        WCHAR $[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", $, MAX_PATH);
        _ = GetPrivateProfileIntW(L"Settings", L"D3D11", FALSE, $) == TRUE;

        DisableThreadLibraryCalls(hinstDLL);
        CloseHandle(CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL));
    }
    return TRUE;
}