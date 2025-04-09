#define INITGUID
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation

#include <d3d11.h>
#include <MinHook.h>
#include <dxgi1_5.h>
#include <appmodel.h>
#include <windows.ui.core.h>

BOOL fForce = {}, fFeature = {};
ATOM (*__RegisterClassExW__)(PVOID) = {};
HRESULT (*__Present__)(LPUNKNOWN, UINT, UINT) = {};
HRESULT (*__put_PointerCursor__)(ICoreWindow *, LPUNKNOWN) = {};
HRESULT (*__ResizeBuffers__)(LPUNKNOWN, UINT, UINT, UINT, DXGI_FORMAT, UINT) = {};
HRESULT (*__CreateSwapChainForCoreWindow__)(LPUNKNOWN, LPUNKNOWN, ICoreWindow *, DXGI_SWAP_CHAIN_DESC1 *, LPUNKNOWN,
                                            IDXGISwapChain1 **ppSwapChain) = {};

HRESULT _Present_(LPUNKNOWN This, UINT SyncInterval, UINT Flags)
{
    if (fFeature && !SyncInterval)
        Flags |= DXGI_PRESENT_ALLOW_TEARING;

    return __Present__(This, SyncInterval, Flags);
}

HRESULT _ResizeBuffers_(LPUNKNOWN This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                        UINT SwapChainFlags)
{
    if (fFeature)
        SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    return __ResizeBuffers__(This, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT _put_PointerCursor_(ICoreWindow *This, LPUNKNOWN value)
{
    ICoreCursor *pCursor = {};
    ICoreWindow_get_PointerCursor(This, &pCursor);

    if (!pCursor || !value)
    {
        Rect rcClient = {};
        ICoreWindow2 *pWindow = {};

        ICoreWindow_get_Bounds(This, &rcClient);
        ICoreWindow_QueryInterface(This, &IID_ICoreWindow2, (PVOID *)&pWindow);

        ICoreWindow2_put_PointerPosition(pWindow,
                                         (Point){rcClient.X + rcClient.Width / 2, rcClient.Y + rcClient.Height / 2});
        ICoreWindow2_Release(pWindow);
    }

    if (pCursor)
        ICoreCursor_Release(pCursor);

    return __put_PointerCursor__(This, value);
}

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

    if (fFeature)
        pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    HRESULT hResult = __CreateSwapChainForCoreWindow__(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

    static BOOL fHook = {};

    if (!fHook)
    {
        fHook = TRUE;

        IDXGISwapChain1Vtbl *pVtbl = (*ppSwapChain)->lpVtbl;
        PVOID pTarget = pVtbl->Present;

        MH_CreateHook(pTarget, &_Present_, (PVOID *)&__Present__);
        MH_QueueEnableHook(pTarget);

        MH_CreateHook(pTarget = pVtbl->ResizeBuffers, &_ResizeBuffers_, (PVOID *)&__ResizeBuffers__);
        MH_QueueEnableHook(pTarget);

        MH_CreateHook(pTarget = pWindow->lpVtbl->put_PointerCursor, &_put_PointerCursor_,
                      (PVOID *)&__put_PointerCursor__);
        MH_QueueEnableHook(pTarget);

        MH_ApplyQueued();
    }

    return hResult;
}

ATOM _RegisterClassExW_(PVOID lpwcx)
{
    static BOOL fHook = {};

    if (!fHook)
    {
        fHook = TRUE;

        WCHAR szPath[MAX_PATH] = {};
        IDXGIFactory5 *pFactory = {};

        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", szPath, MAX_PATH);
        fForce = GetPrivateProfileIntW(L"Stonecutter", L"Force", FALSE, szPath) == TRUE;

        CreateDXGIFactory2((UINT){}, &IID_IDXGIFactory5, (PVOID *)&pFactory);

        IDXGIFactory5_CheckFeatureSupport(pFactory, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &fFeature, sizeof(BOOL));

        MH_CreateHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow, &_CreateSwapChainForCoreWindow_,
                      (PVOID *)&__CreateSwapChainForCoreWindow__);
        MH_EnableHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow);

        IDXGIFactory5_Release(pFactory);
    }

    return __RegisterClassExW__(lpwcx);
}

BOOL DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, PVOID pReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);

        if (GetCurrentPackageFamilyName(&(UINT32){}, NULL) != ERROR_INSUFFICIENT_BUFFER)
            return FALSE;

        HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");

        if (!hMutex || GetLastError())
        {
            CloseHandle(hMutex);
            return FALSE;
        }

        MH_Initialize();

        MH_CreateHook(RegisterClassExW, &_RegisterClassExW_, (PVOID *)&__RegisterClassExW__);
        MH_EnableHook(RegisterClassExW);
    }

    return TRUE;
}