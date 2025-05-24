#define INITGUID
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation

#include <d3d11_1.h>
#include <MinHook.h>
#include <appmodel.h>
#include <windows.ui.core.h>

BOOL bForce = {};

HRESULT (*__IDXGISwapChain1_Present)(IDXGISwapChain1 *, UINT, UINT) = {};

HRESULT (*__IDXGISwapChain1_ResizeBuffers)(IDXGISwapChain1 *, UINT, UINT, UINT, DXGI_FORMAT, UINT) = {};

HRESULT (*__ICoreWindow_put_PointerCursor)(ICoreWindow *, ICoreCursor *);

HRESULT (*__IDXGIFactory2_CreateSwapChainForCoreWindow)(IDXGIFactory2 *, IUnknown *, ICoreWindow *,
                                                        DXGI_SWAP_CHAIN_DESC1 *, IDXGIOutput *,
                                                        IDXGISwapChain1 **) = {};

ATOM (*__RegisterClassExW)(PWNDCLASSEXW) = {};

HRESULT _IDXGISwapChain1_Present(IDXGISwapChain1 *This, UINT SyncInterval, UINT Flags)
{
    if (!SyncInterval)
        Flags |= DXGI_PRESENT_ALLOW_TEARING;
    return __IDXGISwapChain1_Present(This, SyncInterval, Flags);
}

HRESULT _IDXGISwapChain1_ResizeBuffers(IDXGISwapChain1 *This, UINT BufferCount, UINT Width, UINT Height,
                                       DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    return __IDXGISwapChain1_ResizeBuffers(This, BufferCount, Width, Height, NewFormat,
                                           SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

HRESULT _ICoreWindow_put_PointerCursor(ICoreWindow *This, ICoreCursor *Value)
{
    ICoreCursor *pCursor = {};
    ICoreWindow_get_PointerCursor(This, &pCursor);

    if (!pCursor || !Value)
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
    return __ICoreWindow_put_PointerCursor(This, Value);
}

HRESULT _IDXGIFactory2_CreateSwapChainForCoreWindow(IDXGIFactory2 *This, IUnknown *pDevice, ICoreWindow *pWindow,
                                                    DXGI_SWAP_CHAIN_DESC1 *pDesc, IDXGIOutput *pRestrictToOutput,
                                                    IDXGISwapChain1 **ppSwapChain)
{
    if (bForce)
    {
        IUnknown *pUnknown = {};
        if (IUnknown_QueryInterface(pDevice, &IID_ID3D11Device, (PVOID *)&pUnknown))
            return DXGI_ERROR_INVALID_CALL;
        IUnknown_Release(pUnknown);
    }

    pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    HRESULT hResult =
        __IDXGIFactory2_CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);

    static BOOL bFlag = {};

    if (!bFlag)
    {
        bFlag = TRUE;

        MH_CreateHook((*ppSwapChain)->lpVtbl->Present, &_IDXGISwapChain1_Present, (PVOID)&__IDXGISwapChain1_Present);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->Present);

        MH_CreateHook((*ppSwapChain)->lpVtbl->ResizeBuffers, &_IDXGISwapChain1_ResizeBuffers,
                      (PVOID)&__IDXGISwapChain1_ResizeBuffers);
        MH_QueueEnableHook((*ppSwapChain)->lpVtbl->ResizeBuffers);

        MH_CreateHook(pWindow->lpVtbl->put_PointerCursor, &_ICoreWindow_put_PointerCursor,
                      (PVOID)&__ICoreWindow_put_PointerCursor);
        MH_QueueEnableHook(pWindow->lpVtbl->put_PointerCursor);

        MH_ApplyQueued();
    }

    return hResult;
}

ATOM _RegisterClassExW(PWNDCLASSEXW Value)
{
    static BOOL bFlag = {};

    if (!bFlag)
    {
        bFlag = TRUE;

        WCHAR szPath[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", szPath, MAX_PATH);
        bForce = GetPrivateProfileIntW(L"Stonecutter", L"Force", FALSE, szPath) == TRUE;

        IDXGIFactory2 *pFactory = {};
        CreateDXGIFactory(&IID_IDXGIFactory2, (PVOID *)&pFactory);

        MH_CreateHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow, &_IDXGIFactory2_CreateSwapChainForCoreWindow,
                      (PVOID *)&__IDXGIFactory2_CreateSwapChainForCoreWindow);
        MH_EnableHook(pFactory->lpVtbl->CreateSwapChainForCoreWindow);

        IDXGIFactory2_Release(pFactory);
    }

    return __RegisterClassExW(Value);
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

        MH_CreateHook(RegisterClassExW, &_RegisterClassExW, (PVOID *)&__RegisterClassExW);
        MH_EnableHook(RegisterClassExW);
    }
    return TRUE;
}

PBYTE __wrap_memcpy(PBYTE Destination, PBYTE Source, SIZE_T Count)
{
    __movsb(Destination, Source, Count);
    return Destination;
}

PBYTE __wrap_memset(PBYTE Destination, BYTE Data, SIZE_T Count)
{
    __stosb(Destination, Data, Count);
    return Destination;
}
