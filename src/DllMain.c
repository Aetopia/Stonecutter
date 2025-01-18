#include <initguid.h>
#include <MinHook.h>
#include <d3d11_1.h>
#include <windows.ui.core.h>

BOOL _ = {};

HRESULT(*_put_PointerCursor)
(__x_ABI_CWindows_CUI_CCore_CICoreWindow *, __x_ABI_CWindows_CUI_CCore_CICoreCursor *) = NULL;

HRESULT (*_ResizeBuffers)(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT) = NULL;

HRESULT (*_Present)(IDXGISwapChain *, UINT, UINT) = NULL;

HRESULT(*_CreateSwapChainForCoreWindow)
(IDXGIFactory2 *, IUnknown *, __x_ABI_CWindows_CUI_CCore_CICoreWindow *, DXGI_SWAP_CHAIN_DESC1 *, IDXGIOutput *,
 IDXGISwapChain1 **) = NULL;

HRESULT put_PointerCursor(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This, __x_ABI_CWindows_CUI_CCore_CICoreCursor *value)
{
    __x_ABI_CWindows_CUI_CCore_CICoreCursor *pCursor = NULL;
    This->lpVtbl->get_PointerCursor(This, &pCursor);

    if (!pCursor)
    {
        __x_ABI_CWindows_CFoundation_CRect _ = {};
        This->lpVtbl->get_Bounds(This, &_);

        __x_ABI_CWindows_CUI_CCore_CICoreWindow2 *pWindow = NULL;
        This->lpVtbl->QueryInterface(This, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindow2, (void **)&pWindow);
        pWindow->lpVtbl->put_PointerPosition(
            pWindow, (__x_ABI_CWindows_CFoundation_CPoint){_.X + _.Width / 2, _.Y + _.Height / 2});
        pWindow->lpVtbl->Release(pWindow);
    }
    else
        pCursor->lpVtbl->Release(pCursor);

    return _put_PointerCursor(This, value);
}

HRESULT CreateSwapChainForCoreWindow(IDXGIFactory2 *This, IUnknown *pDevice,
                                     __x_ABI_CWindows_CUI_CCore_CICoreWindow *pWindow, DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                     IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
    if (!MH_CreateHook((*(LPVOID **)pWindow)[15], &put_PointerCursor, (LPVOID *)&_put_PointerCursor))
        MH_EnableHook(MH_ALL_HOOKS);

    if (_)
    {
        ID3D11Device *_ = NULL;
        if (pDevice->lpVtbl->QueryInterface(pDevice, &IID_ID3D11Device, (LPVOID *)&_))
            return DXGI_ERROR_INVALID_CALL;
        _->lpVtbl->Release(_);
    }

    pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    return _CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

HRESULT Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
    return _Present(This, SyncInterval, SyncInterval ? Flags : DXGI_PRESENT_ALLOW_TEARING);
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
    MH_CreateHook((*(LPVOID **)pFactory)[16], &CreateSwapChainForCoreWindow, (LPVOID *)&_CreateSwapChainForCoreWindow);
    pFactory->lpVtbl->Release(pFactory);

    IDXGISwapChain *pSwapChain = NULL;
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
                                  &((DXGI_SWAP_CHAIN_DESC){.BufferCount = 1,
                                                           .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                           .SampleDesc.Count = 1,
                                                           .Windowed = TRUE,
                                                           .OutputWindow = GetDesktopWindow()}),
                                  &pSwapChain, NULL, NULL, NULL);
    LPVOID *lpVtbl = *(LPVOID **)pSwapChain;
    MH_CreateHook(lpVtbl[8], &Present, (LPVOID *)&_Present);
    MH_CreateHook(lpVtbl[13], &ResizeBuffers, (LPVOID *)&_ResizeBuffers);
    pSwapChain->lpVtbl->Release(pSwapChain);

    return MH_EnableHook(MH_ALL_HOOKS);
}

BOOL DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        WCHAR szFileName[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", szFileName, MAX_PATH);
        _ = GetPrivateProfileIntW(L"", L"", FALSE, szFileName) == TRUE;
        
        MH_Initialize();
        DisableThreadLibraryCalls(hInstance);
        CloseHandle(CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL));
    }
    return TRUE;
}