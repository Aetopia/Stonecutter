#define INITGUID
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation

#include <d3d11_1.h>
#include <minhook.h>
#include <appmodel.h>
#include <windows.ui.core.h>

BOOL bD3D11 = {};
ATOM (*_RegisterClassExW)(PVOID);
HRESULT (*_Present)(PVOID, UINT, UINT);
HRESULT (*_PointerCursor)(PVOID, PVOID);
HRESULT (*_ResizeBuffers)(PVOID, UINT, UINT, UINT, DXGI_FORMAT, UINT);
HRESULT (*_CreateSwapChainForCoreWindow)(PVOID, PVOID, PVOID, PVOID, PVOID, PVOID);

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

HRESULT $Present(PVOID This, UINT SyncInterval, UINT Flags)
{
    return _Present(This, SyncInterval, SyncInterval ? Flags : DXGI_PRESENT_ALLOW_TEARING);
}

HRESULT $ResizeBuffers(PVOID This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                       UINT SwapChainFlags)
{
    return _ResizeBuffers(This, BufferCount, Width, Height, NewFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

HRESULT $PointerCursor(ICoreWindow *This, PVOID Value)
{
    ICoreCursor *pCursor = {};
    if (!Value || (ICoreWindow_get_PointerCursor(This, &pCursor) * 0 | !pCursor))
    {
        Rect _ = {};
        ICoreWindow_get_Bounds(This, &_);

        ICoreWindow2 *pWindow = {};
        ICoreWindow_QueryInterface(This, &IID_ICoreWindow2, (PVOID)&pWindow);

        ICoreWindow2_put_PointerPosition(pWindow, (Point){_.X + _.Width / 2, _.Y + _.Height / 2});
        ICoreWindow2_Release(pWindow);
    }

    if (pCursor)
        ICoreCursor_Release(pCursor);
    return _PointerCursor(This, Value);
}

HRESULT $CreateSwapChainForCoreWindow(PVOID This, LPUNKNOWN pDevice, ICoreWindow *pWindow, DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                      PVOID pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
    if (bD3D11)
    {
        LPUNKNOWN _ = {};
        if (IUnknown_QueryInterface(pDevice, &IID_ID3D11Device, (PVOID)&_))
            return DXGI_ERROR_INVALID_CALL;
        IUnknown_Release(_);
    }

    pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    HRESULT hResult = _CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

    static BOOL _ = {};
    if (!_)
    {
        MH_CreateHook((*ppSwapChain)->lpVtbl->Present, &$Present, (PVOID)&_Present);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->Present);

        MH_CreateHook((*ppSwapChain)->lpVtbl->ResizeBuffers, &$ResizeBuffers, (PVOID)&_ResizeBuffers);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->ResizeBuffers);

        MH_CreateHook(pWindow->lpVtbl->put_PointerCursor, &$PointerCursor, (PVOID)&_PointerCursor);
        MH_QueueEnableHook(pWindow->lpVtbl->put_PointerCursor);

        MH_ApplyQueued();
        _ = !_;
    }
    return hResult;
}

ATOM $RegisterClassExW(PVOID Value)
{
    static BOOL _ = {};
    if (!_)
    {
        WCHAR szPath[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\Stonecutter.ini", szPath, MAX_PATH);
        bD3D11 = GetPrivateProfileIntW(L"Stonecutter", L"D3D11", FALSE, szPath) == TRUE;

        IDXGIFactory2 *pFactory = {};
        CreateDXGIFactory(&IID_IDXGIFactory2, (PVOID)&pFactory);

        MH_CreateHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow, &$CreateSwapChainForCoreWindow,
                      (PVOID)&_CreateSwapChainForCoreWindow);
        MH_EnableHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow);

        IDXGIFactory2_Release(pFactory);
        _ = !_;
    }
    return _RegisterClassExW(Value);
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
        MH_CreateHook(RegisterClassExW, &$RegisterClassExW, (PVOID)&_RegisterClassExW);
        MH_EnableHook(RegisterClassExW);
    }
    return TRUE;
}