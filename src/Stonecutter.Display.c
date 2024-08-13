#include <initguid.h>
#include <windows.h>
#include <shobjidl.h>
#include <propkey.h>
#include <dwmapi.h>

HWND hWnd = NULL;

DWORD dmPelsWidth = 0, dmPelsHeight = 0, dmDisplayFrequency = 0;

VOID $(BOOL _)
{
    MONITORINFO mi = {.cbSize = sizeof(MONITORINFO)};
    GetMonitorInfoW(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);
    if (!mi.dwFlags)
        return;

    DEVMODEW dm = {};
    if (_)
    {
        dm = (DEVMODEW){.dmSize = sizeof(DEVMODEW),
                        .dmFields = DM_DISPLAYORIENTATION | DM_DISPLAYFIXEDOUTPUT | DM_PELSWIDTH | DM_PELSHEIGHT |
                                    DM_DISPLAYFREQUENCY,
                        .dmDisplayOrientation = DMDO_DEFAULT,
                        .dmDisplayFixedOutput = DMDFO_DEFAULT,
                        .dmPelsWidth = dmPelsWidth,
                        .dmPelsHeight = dmPelsHeight,
                        .dmDisplayFrequency = dmDisplayFrequency};

        if (ChangeDisplaySettingsW(&dm, CDS_TEST))
        {
            EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &dm);
            if (dm.dmDisplayOrientation == DMDO_90 || dm.dmDisplayOrientation == DMDO_270)
                dm = (DEVMODEW){
                    .dmSize = sizeof(DEVMODEW), .dmPelsWidth = dm.dmPelsHeight, .dmPelsHeight = dm.dmPelsWidth};
            dm.dmDisplayOrientation = DMDO_DEFAULT;
            dm.dmDisplayFixedOutput = DMDFO_DEFAULT;
            dm.dmFields =
                DM_DISPLAYORIENTATION | DM_DISPLAYFIXEDOUTPUT | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
        }
    }

    ChangeDisplaySettingsW(_ ? &dm : NULL, CDS_FULLSCREEN);
}

BOOL IsImmersiveWindow()
{
    static BOOL (*_)(HWND, PDWORD) = NULL;
    if (!_)
    {
        HMODULE hModule = LoadLibraryExW(L"User32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
        _ = (BOOL(*)(HWND, PDWORD))GetProcAddress(hModule, "GetWindowBand");
        FreeLibrary(hModule);
    }
    DWORD dwBand = 0;
    _(hWnd, &dwBand);
    return dwBand != 1;
}

VOID WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread,
                  DWORD dwmsEventTime)
{
    if (hwnd == hWnd)
        switch (event)
        {
        case EVENT_OBJECT_CLOAKED:
        case EVENT_OBJECT_UNCLOAKED:
            $(event == EVENT_OBJECT_UNCLOAKED);
            break;

        case EVENT_SYSTEM_FOREGROUND:
            if (IsImmersiveWindow())
                $(TRUE);
            break;

        case EVENT_OBJECT_DESTROY: {
            INT _ = FALSE;
            DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &_, sizeof(INT));
            if (IsImmersiveWindow() && !_)
                $(FALSE);
            ExitProcess(0);
        }
        }
}

HRESULT QueryInterface(IAppVisibilityEvents *This, REFIID riid, void **ppvObject)
{
    return RtlCompareMemory(riid, &IID_IAppVisibilityEvents, sizeof(IID)) == sizeof(IID) ||
                   RtlCompareMemory(riid, &IID_IUnknown, sizeof(IID)) == sizeof(IID)
               ? !(*ppvObject = This)
               : E_NOINTERFACE;
}

ULONG _(IAppVisibilityEvents *This)
{
    return 0;
}

HRESULT AppVisibilityOnMonitorChanged(IAppVisibilityEvents *This, HMONITOR hMonitor,
                                      MONITOR_APP_VISIBILITY previousMode, MONITOR_APP_VISIBILITY currentMode)
{
    if (hWnd == GetForegroundWindow())
        $(currentMode == MAV_APP_VISIBLE);
    return S_OK;
}

HRESULT LauncherVisibilityChange(IAppVisibilityEvents *This, WINBOOL currentVisibleState)
{
    return S_OK;
}

BOOL EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    IPropertyStore *_ = NULL;
    SHGetPropertyStoreForWindow(hwnd, &IID_IPropertyStore, (void **)&_);

    PROPVARIANT $ = {};
    _->lpVtbl->GetValue(_, &PKEY_AppUserModel_ID, &$);
    _->lpVtbl->Release(_);

    if (CompareStringOrdinal($.pwszVal, -1, (LPCWCH)lParam, -1, FALSE) == CSTR_EQUAL)
        hWnd = hwnd;

    PropVariantClear(&$);
    return !hWnd;
}

VOID WinMainCRTStartup()
{
    INT nArgs = 0;
    PWSTR *szArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (nArgs == 1)
        goto _;

    PCWSTR lpName = *(szArgs[1]) == L'1'   ? L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe!App"
                    : *(szArgs[1]) == L'0' ? L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App"
                                           : NULL;
    CreateMutexW(NULL, FALSE, lpName);
    if (!lpName || GetLastError() == ERROR_ALREADY_EXISTS)
        goto _;

    WCHAR szFileName[MAX_PATH] = {};
    ExpandEnvironmentStringsW(*(szArgs[1]) == L'1' ? L"%LOCALAPPDATA%\\Packages\\Microsoft.MinecraftWindowsBeta_"
                                                     L"8wekyb3d8bbwe\\RoamingState\\Stonecutter."
                                                     L"ini"
                                                   : L"%LOCALAPPDATA%\\Packages\\Microsoft.MinecraftUWP_"
                                                     L"8wekyb3d8bbwe\\RoamingState\\Stonecutter.ini",
                              szFileName, MAX_PATH);

    if (GetPrivateProfileIntW(L"Settings", L"Fullscreen", FALSE, szFileName) != TRUE ||
        EnumWindows(EnumWindowsProc, (LPARAM)lpName))
        goto _;
    LocalFree(szArgs);

    if (!(dmPelsWidth = GetPrivateProfileIntW(L"Settings", L"Width", -1, szFileName)) ||
        !(dmPelsHeight = GetPrivateProfileIntW(L"Settings", L"Height", -1, szFileName)) ||
        !(dmDisplayFrequency = GetPrivateProfileIntW(L"Settings", L"Frequency", -1, szFileName)))
        dmDisplayFrequency = -1;

    DWORD dwThreadId = GetWindowThreadProcessId(hWnd, NULL);
    SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, WinEventProc, 0, dwThreadId,
                    WINEVENT_OUTOFCONTEXT);
    SetWinEventHook(EVENT_OBJECT_CLOAKED, EVENT_OBJECT_CLOAKED, NULL, WinEventProc, 0, dwThreadId,
                    WINEVENT_OUTOFCONTEXT);
    SetWinEventHook(EVENT_OBJECT_UNCLOAKED, EVENT_OBJECT_UNCLOAKED, NULL, WinEventProc, 0, dwThreadId,
                    WINEVENT_OUTOFCONTEXT);
    SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, NULL, WinEventProc, 0, dwThreadId,
                    WINEVENT_OUTOFCONTEXT);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IAppVisibility *pAppVisibility = NULL;
    CoCreateInstance(&CLSID_AppVisibility, NULL, CLSCTX_INPROC_SERVER, &IID_IAppVisibility, (LPVOID *)&pAppVisibility);

    IAppVisibilityEvents *pCallback =
        &((IAppVisibilityEvents){.lpVtbl = &((IAppVisibilityEventsVtbl){
                                     QueryInterface, _, _, AppVisibilityOnMonitorChanged, LauncherVisibilityChange})});
    pAppVisibility->lpVtbl->Advise(pAppVisibility, pCallback, &((DWORD){}));

    MONITOR_APP_VISIBILITY $ = MAV_UNKNOWN;
    pAppVisibility->lpVtbl->GetAppVisibilityOnMonitor(pAppVisibility,
                                                      MonitorFromPoint((POINT){}, MONITOR_DEFAULTTOPRIMARY), &$);
    if ($ == MAV_APP_VISIBLE)
        pCallback->lpVtbl->AppVisibilityOnMonitorChanged(NULL, NULL, MAV_UNKNOWN, MAV_APP_VISIBLE);

    MSG _ = {};
    while (GetMessageW(&_, NULL, 0, 0))
        DispatchMessageW(&_);
_:
    ExitProcess(0);
}