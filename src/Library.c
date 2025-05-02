#define INITGUID
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation

#include <d3d11_1.h>
#include <MinHook.h>
#include <appmodel.h>
#include <windows.ui.core.h>

BOOL fForce = {};
ATOM (*__RegisterClassExW)(PVOID) = {};
HRESULT (*__Present)(PVOID, UINT, UINT) = {};
HRESULT (*__put_PointerCursor)(PVOID, PVOID) = {};
HRESULT (*__ResizeBuffers)(PVOID, UINT, UINT, UINT, DXGI_FORMAT, UINT) = {};
HRESULT (*__CreateSwapChainForCoreWindow)(PVOID, PVOID, PVOID, PVOID, PVOID, PVOID) = {};

PVOID __wrap_memcpy(PVOID Destination, PVOID Source, SIZE_T Count)
{
    __movsb(Destination, Source, Count);
    return Destination;
}

PVOID __wrap_memset(PVOID Destination, BYTE Data, SIZE_T Count)
{
    __stosb(Destination, Data, Count);
    return Destination;
}

HRESULT _Present(PVOID This, UINT SyncInterval, UINT Flags)
{
    if (!SyncInterval)
        Flags |= DXGI_PRESENT_ALLOW_TEARING;
    return __Present(This, SyncInterval, Flags);
}

HRESULT _ResizeBuffers(PVOID This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                       UINT SwapChainFlags)
{
    return __ResizeBuffers(This, BufferCount, Width, Height, NewFormat,
                           SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

HRESULT _put_PointerCursor(PVOID This, PVOID value)
{
    ICoreCursor *pCursor = {};
    ICoreWindow_get_PointerCursor(This, &pCursor);

    if (!pCursor || !value)
    {
        Rect rcClient = {};
        PVOID pWindow = {};

        ICoreWindow_get_Bounds(This, &rcClient);
        ICoreWindow_QueryInterface(This, &IID_ICoreWindow2, &pWindow);

        ICoreWindow2_put_PointerPosition(pWindow,
                                         (Point){rcClient.X + rcClient.Width / 2, rcClient.Y + rcClient.Height / 2});
        ICoreWindow2_Release(pWindow);
    }

    if (pCursor)
        ICoreCursor_Release(pCursor);
    return __put_PointerCursor(This, value);
}

HRESULT _CreateSwapChainForCoreWindow(PVOID This, PVOID pDevice, ICoreWindow *pWindow, DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                      PVOID pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
    static BOOL fHook = {};

    if (fForce)
    {
        PVOID pUnknown = {};
        if (IUnknown_QueryInterface(pDevice, &IID_ID3D11Device, &pUnknown))
            return DXGI_ERROR_INVALID_CALL;
        IUnknown_Release(pUnknown);
    }

    pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    HRESULT hResult = __CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

    if (!fHook)
    {
        fHook = TRUE;

        MH_CreateHook((*ppSwapChain)->lpVtbl->Present, &_Present, (PVOID)&__Present);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->Present);

        MH_CreateHook((*ppSwapChain)->lpVtbl->ResizeBuffers, &_ResizeBuffers, (PVOID)&__ResizeBuffers);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->ResizeBuffers);

        MH_CreateHook(pWindow->lpVtbl->put_PointerCursor, &_put_PointerCursor, (PVOID)&__put_PointerCursor);
        MH_QueueEnableHook(pWindow->lpVtbl->put_PointerCursor);

        MH_ApplyQueued();
    }

    return hResult;
}

ATOM _RegisterClassExW(PVOID lpwcx)
{
    static BOOL fHook = {};

    if (!fHook)
    {
        fHook = TRUE;

        WCHAR szPath[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", szPath, MAX_PATH);
        fForce = GetPrivateProfileIntW(L"Stonecutter", L"Force", FALSE, szPath) == TRUE;

        IDXGIFactory2 *pFactory = {};
        CreateDXGIFactory(&IID_IDXGIFactory2, (PVOID)&pFactory);

        MH_CreateHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow, &_CreateSwapChainForCoreWindow,
                      (PVOID)&__CreateSwapChainForCoreWindow);
        MH_EnableHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow);

        IDXGIFactory2_Release(pFactory);
    }

    return __RegisterClassExW(lpwcx);
}

BOOL DllMainCRTStartup(PVOID hInstance, DWORD dwReason, PVOID pReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        if (GetCurrentPackageFamilyName(&(UINT32){}, NULL) != ERROR_INSUFFICIENT_BUFFER)
            return FALSE;

        PVOID hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");
        if (!hMutex || GetLastError())
        {
            CloseHandle(hMutex);
            return FALSE;
        }

        MH_Initialize();
        MH_CreateHook(RegisterClassExW, &_RegisterClassExW, (PVOID)&__RegisterClassExW);
        MH_EnableHook(RegisterClassExW);
    }

    return TRUE;
}