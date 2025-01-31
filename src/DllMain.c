#include <initguid.h>
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

HRESULT(*__put_PointerCursor__)
(__x_ABI_CWindows_CUI_CCore_CICoreWindow *, __x_ABI_CWindows_CUI_CCore_CICoreCursor *) = {};

HRESULT _put_PointerCursor_(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This,
                            __x_ABI_CWindows_CUI_CCore_CICoreCursor *value)
{
    __x_ABI_CWindows_CUI_CCore_CICoreCursor *pCursor = {};
    This->lpVtbl->get_PointerCursor(This, &pCursor);

    if (pCursor)
        pCursor->lpVtbl->Release(pCursor);
    else
    {
        __x_ABI_CWindows_CFoundation_CRect rcClient = {};
        This->lpVtbl->get_Bounds(This, &rcClient);

        __x_ABI_CWindows_CUI_CCore_CICoreWindow2 *pWindow = {};
        This->lpVtbl->QueryInterface(This, &IID___x_ABI_CWindows_CUI_CCore_CICoreWindow2, (PVOID *)&pWindow);

        pWindow->lpVtbl->put_PointerPosition(
            pWindow,
            (__x_ABI_CWindows_CFoundation_CPoint){rcClient.X + rcClient.Width / 2, rcClient.Y + rcClient.Height / 2});

        pWindow->lpVtbl->Release(pWindow);
    }

    return __put_PointerCursor__(This, value);
}

HRESULT(*__CreateSwapChainForCoreWindow__)
(LPUNKNOWN, LPUNKNOWN, LPUNKNOWN, DXGI_SWAP_CHAIN_DESC1 *, LPUNKNOWN, LPUNKNOWN *) = {};

HRESULT _CreateSwapChainForCoreWindow_(LPUNKNOWN This, LPUNKNOWN pDevice, LPUNKNOWN pWindow,
                                       DXGI_SWAP_CHAIN_DESC1 *pDesc, LPUNKNOWN pRestrictToOutput,
                                       LPUNKNOWN *ppSwapChain)
{
    if (fForce)
    {
        LPUNKNOWN pUnknown = {};
        if (pDevice->lpVtbl->QueryInterface(pDevice, &IID_ID3D11Device, (PVOID *)&pUnknown))
            return DXGI_ERROR_INVALID_CALL;
        pUnknown->lpVtbl->Release(pUnknown);
    }

    pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    HRESULT hResult = __CreateSwapChainForCoreWindow__(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

    static BOOL fHook = {};
    if (!fHook && !hResult)
    {
        PVOID *pVtbl = *(PVOID **)*ppSwapChain, pTarget = pVtbl[8];

        MH_CreateHook(pTarget, &_Present_, (PVOID *)&__Present__);
        MH_QueueEnableHook(pTarget);

        MH_CreateHook(pTarget = pVtbl[13], &_ResizeBuffers_, (PVOID *)&__ResizeBuffers__);
        MH_QueueEnableHook(pTarget);

        MH_CreateHook(pTarget = (*(PVOID **)pWindow)[15], &_put_PointerCursor_, (PVOID *)&__put_PointerCursor__);
        MH_QueueEnableHook(pTarget);

        fHook = !MH_ApplyQueued();
    }

    return hResult;
}

HWND (*__CreateWindowExW__)(DWORD, PCWSTR, PCWSTR, DWORD, INT, INT, INT, INT, HWND, HMENU, HINSTANCE, PVOID) = {};

HWND _CreateWindowExW_(DWORD dwExStyle, PCWSTR lpClassName, PCWSTR lpWindowName, DWORD dwStyle, INT X, INT Y,
                       INT nWidth, INT nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, PVOID lpParam)
{
    static BOOL fHook = {};
    if (!fHook)
    {
        WCHAR szPath[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", szPath, MAX_PATH);
        fForce = GetPrivateProfileIntW(L"Stonecutter", L"Force", FALSE, szPath) == TRUE;

        LPUNKNOWN pUnknown = {};
        CreateDXGIFactory(&IID_IDXGIFactory2, (PVOID *)&pUnknown);

        PVOID pTarget = (*(PVOID **)pUnknown)[16];

        MH_CreateHook(pTarget, &_CreateSwapChainForCoreWindow_, (PVOID *)&__CreateSwapChainForCoreWindow__);
        MH_EnableHook(pTarget);

        pUnknown->lpVtbl->Release(pUnknown);
    }
    return __CreateWindowExW__(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu,
                               hInstance, lpParam);
}

BOOL DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, PVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if (GetCurrentPackageFamilyName(&((UINT32){}), NULL) != ERROR_INSUFFICIENT_BUFFER)
            return FALSE;

        DisableThreadLibraryCalls(hInstance);

        MH_Initialize();
        MH_CreateHook(CreateWindowExW, &_CreateWindowExW_, (PVOID)&__CreateWindowExW__);
        MH_EnableHook(CreateWindowExW);
    }
    return TRUE;
}