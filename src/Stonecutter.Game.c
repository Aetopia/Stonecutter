#define WIN32_LEAN_AND_MEAN
#define NOATOM
#define NOGDI
#define NOGDICAPMASKS
#define NOMETAFILE
#define NOMINMAX
#define NOOPENFILE
#define NORASTEROPS
#define NOSCROLL
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOCRYPT
#define NOMCX
#define NOIME

#include <initguid.h>
#include <windows.ui.core.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3d12.h>
#include <MinHook.h>

BOOL fD3D11 = FALSE, fEnabled = FALSE;

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
    if (!fEnabled)
    {
        fEnabled = TRUE;
        MH_CreateHook((*(LPVOID **)pWindow)[15], &put_PointerCursor, (LPVOID *)&_put_PointerCursor);
        MH_EnableHook(MH_ALL_HOOKS);
    }

    ID3D12CommandQueue *pCommandQueue = NULL;
    if (fD3D11 && !pDevice->lpVtbl->QueryInterface(pDevice, &IID_ID3D12CommandQueue, (void **)&pCommandQueue))
    {
        pCommandQueue->lpVtbl->Release(pCommandQueue);
        return DXGI_ERROR_INVALID_CALL;
    }

    pDesc->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    return _CreateSwapChainForCoreWindow(This, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

HRESULT Present(IDXGISwapChain *This, UINT SyncInterval, UINT Flags)
{
    return fEnabled ? _Present(This, 0, DXGI_PRESENT_ALLOW_TEARING) : DXGI_ERROR_DEVICE_RESET;
}

HRESULT ResizeBuffers(IDXGISwapChain *This, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
                      UINT SwapChainFlags)
{
    return _ResizeBuffers(This, BufferCount, Width, Height, NewFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

DWORD ThreadProc(LPVOID lpParameter)
{
    WCHAR szFileName[MAX_PATH] = {};
    ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\..\\RoamingState\\Stonecutter\\Game.ini", szFileName, MAX_PATH);
    fD3D11 = GetPrivateProfileIntW(L"Settings", L"D3D11", FALSE, szFileName) == TRUE;

    MH_Initialize();

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

BOOL DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter.Game");
        if (GetLastError())
        {
            CloseHandle(hMutex);
            return FALSE;
        }

        DisableThreadLibraryCalls(hinstDLL);
        CloseHandle(CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL));
    }
    return TRUE;
}