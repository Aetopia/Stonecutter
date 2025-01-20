#define _MINAPPMODEL_H_
#include <initguid.h>
#include <MinHook.h>
#include <d3d11_1.h>
#include <appmodel.h>
#include <windows.ui.core.h>

BOOL _ = {};

HRESULT (*__ResizeBuffers__)(IDXGISwapChain *, UINT, UINT, UINT, DXGI_FORMAT, UINT) = {};

HRESULT (*__Present__)(IDXGISwapChain *, UINT, UINT) = {};

HRESULT(*__put_PointerCursor__)
(__x_ABI_CWindows_CUI_CCore_CICoreWindow *, __x_ABI_CWindows_CUI_CCore_CICoreCursor *) = {};

HRESULT(*__CreateSwapChainForCoreWindow__)
(IDXGIFactory2 *, IUnknown *, __x_ABI_CWindows_CUI_CCore_CICoreWindow *, DXGI_SWAP_CHAIN_DESC1 *, IDXGIOutput *,
 IDXGISwapChain1 **) = {};

HWND (*__CreateWindowExW__)(DWORD, LPCWSTR, LPCWSTR, DWORD, INT, INT, INT, INT, HWND, HMENU, HINSTANCE, LPVOID) = {};

HRESULT _ResizeBuffers_(IDXGISwapChain *This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                        UINT SwapChainFlags)
{
    return __ResizeBuffers__(This, BufferCount, Width, Height, NewFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

HRESULT _Present_(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
    return __Present__(This, SyncInterval, SyncInterval ? Flags : DXGI_PRESENT_ALLOW_TEARING);
}

HRESULT _put_PointerCursor_(__x_ABI_CWindows_CUI_CCore_CICoreWindow *This,
                            __x_ABI_CWindows_CUI_CCore_CICoreCursor *value)
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

    return __put_PointerCursor__(This, value);
}

HRESULT _CreateSwapChainForCoreWindow_(IDXGIFactory2 *This, IUnknown *pDevice,
                                       __x_ABI_CWindows_CUI_CCore_CICoreWindow *pWindow, DXGI_SWAP_CHAIN_DESC1 *pDesc,
                                       IDXGIOutput *pRestrictToOutput, IDXGISwapChain1 **ppSwapChain)
{
    if (_)
    {
        ID3D11Device *_ = NULL;
        if (pDevice->lpVtbl->QueryInterface(pDevice, &IID_ID3D11Device, (LPVOID *)&_))
            return DXGI_ERROR_INVALID_CALL;
        _->lpVtbl->Release(_);
    }

    LPVOID pTarget = (*(LPVOID **)pWindow)[15];
    if (!MH_CreateHook(pTarget, &_put_PointerCursor_, (LPVOID *)&__put_PointerCursor__))
        MH_EnableHook(pTarget);

    pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    return __CreateSwapChainForCoreWindow__(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

HWND _CreateWindowExW_(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, INT X, INT Y,
                       INT nWidth, INT nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    MH_DisableHook(CreateWindowExW);

    IDXGIFactory2 *pFactory = {};
    CreateDXGIFactory(&IID_IDXGIFactory2, (void **)&pFactory);

    LPVOID *lpVtbl = (*(LPVOID **)pFactory);

    MH_CreateHook(lpVtbl[16], &_CreateSwapChainForCoreWindow_, (LPVOID *)&__CreateSwapChainForCoreWindow__);
    MH_EnableHook(lpVtbl[16]);

    pFactory->lpVtbl->Release(pFactory);

    IDXGISwapChain *pSwapChain = {};
    D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, (UINT){}, NULL, (UINT){}, D3D11_SDK_VERSION,
        &((DXGI_SWAP_CHAIN_DESC){.BufferCount = D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT,
                                 .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                 .SampleDesc.Count = D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT,
                                 .Windowed = TRUE,
                                 .OutputWindow = GetDesktopWindow()}),
        &pSwapChain, NULL, NULL, NULL);

    lpVtbl = *(LPVOID **)pSwapChain;

    MH_CreateHook(lpVtbl[8], &_Present_, (LPVOID *)&__Present__);
    MH_EnableHook(lpVtbl[8]);

    MH_CreateHook(lpVtbl[13], &_ResizeBuffers_, (LPVOID *)&__ResizeBuffers__);
    MH_EnableHook(lpVtbl[13]);

    pSwapChain->lpVtbl->Release(pSwapChain);

    return __CreateWindowExW__(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu,
                               hInstance, lpParam);
}

BOOL DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        WCHAR szPackageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH] = {};
        if (GetCurrentPackageFamilyName(&((UINT32){PACKAGE_FAMILY_NAME_MAX_LENGTH}), szPackageFamilyName) ||
            CompareStringOrdinal(szPackageFamilyName, -1, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", -1, TRUE) !=
                CSTR_EQUAL)
            return FALSE;

        HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");
        if (!hMutex || GetLastError())
        {
            CloseHandle(hMutex);
            return FALSE;
        }

        WCHAR szFileName[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter.ini", szFileName, MAX_PATH);
        _ = GetPrivateProfileIntW(L"", L"", FALSE, szFileName) == TRUE;

        MH_Initialize();

        MH_CreateHook(CreateWindowExW, &_CreateWindowExW_, (LPVOID)&__CreateWindowExW__);
        MH_EnableHook(CreateWindowExW);

        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}