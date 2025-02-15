#define INITGUID
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation

#include <d3d11.h>
#include <MinHook.h>
#include <dxgi1_2.h>
#include <appmodel.h>
#include <windows.ui.core.h>

BOOL fForce = {};

HRESULT (*__Present__)(LPUNKNOWN, UINT, UINT) = {};

HRESULT _Present_(LPUNKNOWN This, UINT SyncInterval, UINT Flags)
{
    return __Present__(This, SyncInterval, SyncInterval ? Flags : DXGI_PRESENT_ALLOW_TEARING);
}

HRESULT (*__ResizeBuffers__)(LPUNKNOWN, UINT, UINT, UINT, DXGI_FORMAT, UINT) = {};

HRESULT _ResizeBuffers_(LPUNKNOWN This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                        UINT SwapChainFlags)
{
    return __ResizeBuffers__(This, BufferCount, Width, Height, NewFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

HRESULT (*__put_PointerCursor__)(ICoreWindow *, ICoreCursor *) = {};

HRESULT _put_PointerCursor_(ICoreWindow *This, ICoreCursor *value)
{
    ICoreCursor *pCursor = {};
    ICoreWindow_get_PointerCursor(This, &pCursor);

    if (pCursor)
        ICoreCursor_Release(pCursor);
    else
    {
        Rect rcClient = {};
        ICoreWindow_get_Bounds(This, &rcClient);

        ICoreWindow2 *pWindow = {};
        ICoreWindow_QueryInterface(This, &IID_ICoreWindow2, (PVOID *)&pWindow);

        ICoreWindow2_put_PointerPosition(pWindow,
                                         (Point){rcClient.X + rcClient.Width / 2, rcClient.Y + rcClient.Height / 2});

        ICoreWindow2_Release(pWindow);
    }

    return __put_PointerCursor__(This, value);
}

HRESULT (*__CreateSwapChainForCoreWindow__)(LPUNKNOWN, LPUNKNOWN, ICoreWindow *, DXGI_SWAP_CHAIN_DESC1 *, LPUNKNOWN,
                                            IDXGISwapChain1 **ppSwapChain) = {};

HRESULT _CreateSwapChainForCoreWindow_(LPUNKNOWN This, LPUNKNOWN pDevice, ICoreWindow *pWindow,
                                       DXGI_SWAP_CHAIN_DESC1 *pDesc, LPUNKNOWN pRestrictToOutput,
                                       IDXGISwapChain1 **ppSwapChain)
{
    if (fForce)
    {
        LPUNKNOWN pUnknown = {};
        if (IUnknown_QueryInterface(pDevice, &IID_ID3D11Device, (PVOID *)&pUnknown))
            return DXGI_ERROR_INVALID_CALL;
        IUnknown_Release(pUnknown);
    }

    pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    HRESULT hResult = __CreateSwapChainForCoreWindow__(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

    static BOOL fHook = {};

    if (!fHook && !hResult)
    {
        fHook = TRUE;

        MH_CreateHook((*ppSwapChain)->lpVtbl->Present, &_Present_, (PVOID *)&__Present__);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->Present);

        MH_CreateHook((*ppSwapChain)->lpVtbl->ResizeBuffers, &_ResizeBuffers_, (PVOID *)&__ResizeBuffers__);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->ResizeBuffers);

        MH_CreateHook(pWindow->lpVtbl->put_PointerCursor, &_put_PointerCursor_, (PVOID *)&__put_PointerCursor__);
        MH_QueueEnableHook(pWindow->lpVtbl->put_PointerCursor);

        MH_ApplyQueued();
    }

    return hResult;
}

ATOM (*__RegisterClassExW__)(PWNDCLASSEXW) = {};

ATOM _RegisterClassExW_(PWNDCLASSEXW lpwcx)
{
    static BOOL fHook = {};

    if (!fHook)
    {
        fHook = TRUE;

        WCHAR szPath[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", szPath, MAX_PATH);
        fForce = GetPrivateProfileIntW(L"Stonecutter", L"Force", FALSE, szPath) == TRUE;

        IDXGIFactory2 *pFactory = {};
        CreateDXGIFactory(&IID_IDXGIFactory2, (PVOID *)&pFactory);

        MH_CreateHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow, &_CreateSwapChainForCoreWindow_,
                      (PVOID *)&__CreateSwapChainForCoreWindow__);
        MH_EnableHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow);

        IDXGIFactory2_Release(pFactory);
    }

    return __RegisterClassExW__(lpwcx);
}

BOOL DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, PVOID pReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (GetCurrentPackageFamilyName(&(UINT32){}, NULL) != ERROR_INSUFFICIENT_BUFFER)
            return FALSE;

        DisableThreadLibraryCalls(hInstance);

        MH_Initialize();

        MH_CreateHook(RegisterClassExW, &_RegisterClassExW_, (PVOID *)&__RegisterClassExW__);
        MH_EnableHook(RegisterClassExW);
    }

    return TRUE;
}