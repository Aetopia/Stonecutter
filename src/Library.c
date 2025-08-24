#define INITGUID
#define COBJMACROS
#define _MINAPPMODEL_H_
#define WIDL_C_INLINE_WRAPPERS
#define WIDL_using_Windows_UI_Core
#define WIDL_using_Windows_Foundation

#include <d3d11_1.h>
#include <minhook.h>
#include <appmodel.h>
#include <windows.ui.core.h>

#define KeyName L"D3D11"
#define AppName L"Stonecutter"
#define FileName L"%LOCALAPPDATA%\\Stonecutter.ini"
#define MutexName L"0B8EAD57-F036-48B2-84AD-B9E40F7568A5"
#define PackageFamilyName L"Microsoft.MinecraftUWP_8wekyb3d8bbwe"

BOOL D3D11 = FALSE;

ATOM (*_RegisterClassExW)(PWNDCLASSEXW value) = NULL;

HRESULT (*_Present)(IDXGISwapChain1 *swapchain, UINT interval, UINT flags) = NULL;

HRESULT (*_PointerCursor)(ICoreWindow *window, ICoreCursor *value) = NULL;

HRESULT (*_ResizeBuffers)(IDXGISwapChain1 *swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format,
                          UINT flags) = NULL;

HRESULT (*_CreateSwapChainForCoreWindow)(IDXGIFactory2 *factory, LPUNKNOWN device, ICoreWindow *window,
                                         DXGI_SWAP_CHAIN_DESC1 *description, IDXGIOutput *output,
                                         IDXGISwapChain1 **swapchain) = NULL;

PBYTE __wrap_memcpy(PBYTE destination, PBYTE source, SIZE_T count)
{
    __movsb(destination, source, count);
    return destination;
}

PBYTE __wrap_memset(PBYTE destination, BYTE data, SIZE_T count)
{
    __stosb(destination, data, count);
    return destination;
}

HRESULT $Present(IDXGISwapChain1 *swapchain, UINT interval, UINT flags)
{
    return _Present(swapchain, interval, interval ? flags : DXGI_PRESENT_ALLOW_TEARING);
}

HRESULT $ResizeBuffers(IDXGISwapChain1 *swapchain, UINT count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    return _ResizeBuffers(swapchain, count, width, height, format, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
}

HRESULT $PointerCursor(ICoreWindow *window, ICoreCursor *value)
{
    ICoreCursor *cursor = NULL;
    ICoreWindow_get_PointerCursor(window, &cursor);

    if (!cursor || !value)
    {
        Rect rectangle = {};
        ICoreWindow_get_Bounds(window, &rectangle);

        ICoreWindow2 *object = NULL;
        ICoreWindow_QueryInterface(window, &IID_ICoreWindow2, (PVOID *)&object);

        Point point = {rectangle.X + rectangle.Width / 2, rectangle.Y + rectangle.Height / 2};
        ICoreWindow2_put_PointerPosition(object, point);

        ICoreWindow2_Release(object);
    }

    if (cursor)
        ICoreCursor_Release(cursor);
    return _PointerCursor(window, value);
}

HRESULT $CreateSwapChainForCoreWindow(IDXGIFactory2 *factory, LPUNKNOWN device, ICoreWindow *window,
                                      DXGI_SWAP_CHAIN_DESC1 *description, IDXGIOutput *output,
                                      IDXGISwapChain1 **swapchain)
{
    if (D3D11)
    {
        LPUNKNOWN object = NULL;
        if (FAILED(IUnknown_QueryInterface(device, &IID_ID3D11Device, (PVOID *)&object)))
            return DXGI_ERROR_INVALID_CALL;
        IUnknown_Release(object);
    }

    description->Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    HRESULT result = _CreateSwapChainForCoreWindow(factory, device, window, description, output, swapchain);

    static BOOL hooked = FALSE;

    if (SUCCEEDED(result) && !hooked)
    {
        PVOID target = (*swapchain)->lpVtbl->Present;
        MH_CreateHook(target, &$Present, (PVOID *)&_Present);
        MH_QueueEnableHook(target);

        target = (*swapchain)->lpVtbl->ResizeBuffers;
        MH_CreateHook(target, &$ResizeBuffers, (PVOID *)&_ResizeBuffers);
        MH_QueueEnableHook(target);

        target = window->lpVtbl->put_PointerCursor;
        MH_CreateHook(target, &$PointerCursor, (PVOID *)&_PointerCursor);
        MH_QueueEnableHook(target);

        MH_ApplyQueued();
        hooked = TRUE;
    }

    return result;
}

ATOM $RegisterClassExW(PWNDCLASSEXW value)
{
    static BOOL hooked = FALSE;

    if (!hooked)
    {
        WCHAR path[MAX_PATH] = {};
        ExpandEnvironmentStringsW(FileName, path, MAX_PATH);
        D3D11 = GetPrivateProfileIntW(AppName, KeyName, FALSE, path) == TRUE;

        IDXGIFactory2 *factory = NULL;
        CreateDXGIFactory(&IID_IDXGIFactory2, (PVOID *)&factory);

        PVOID target = factory->lpVtbl->CreateSwapChainForCoreWindow;
        MH_CreateHook(target, &$CreateSwapChainForCoreWindow, (PVOID *)&_CreateSwapChainForCoreWindow);
        MH_EnableHook(target);

        IDXGIFactory2_Release(factory);
        hooked = TRUE;
    }

    return _RegisterClassExW(value);
}

BOOL DllMainCRTStartup(HINSTANCE instance, DWORD reason, PVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);

        WCHAR name[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1] = {};
        if (GetCurrentPackageFamilyName(&(UINT32){ARRAYSIZE(name)}, name) ||
            CompareStringOrdinal(name, -1, PackageFamilyName, -1, TRUE) != CSTR_EQUAL)
            return FALSE;

        PWSTR description = NULL;
        if (FAILED(GetThreadDescription(GetCurrentThread(), &description)) ||
            CompareStringOrdinal(description, -1, MutexName, -1, FALSE) != CSTR_EQUAL)
        {
            LocalFree(description);
            return FALSE;
        }
        LocalFree(description);

        HANDLE mutex = CreateMutexW(NULL, FALSE, MutexName);
        if (!mutex || GetLastError())
        {
            CloseHandle(mutex);
            return FALSE;
        }

        MH_Initialize();
        MH_CreateHook(RegisterClassExW, &$RegisterClassExW, (PVOID *)&_RegisterClassExW);
        MH_EnableHook(RegisterClassExW);
    }
    return TRUE;
}