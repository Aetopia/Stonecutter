#include <initguid.h>
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

    DEVMODEW $ = {};
    if (_)
    {
        $ = (DEVMODEW){.dmSize = sizeof(DEVMODEW),
                       .dmFields = DM_DISPLAYORIENTATION | DM_DISPLAYFIXEDOUTPUT | DM_PELSWIDTH | DM_PELSHEIGHT |
                                   DM_DISPLAYFREQUENCY,
                       .dmDisplayOrientation = DMDO_DEFAULT,
                       .dmDisplayFixedOutput = DMDFO_DEFAULT,
                       .dmPelsWidth = dmPelsWidth,
                       .dmPelsHeight = dmPelsHeight,
                       .dmDisplayFrequency = dmDisplayFrequency};

        if (ChangeDisplaySettingsW(&$, CDS_TEST))
        {
            EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &$);
            if ($.dmDisplayOrientation == DMDO_90 || $.dmDisplayOrientation == DMDO_270)
                $ = (DEVMODEW){
                    .dmSize = sizeof(DEVMODEW), .dmPelsWidth = $.dmPelsHeight, .dmPelsHeight = $.dmPelsWidth};
            $.dmDisplayOrientation = DMDO_DEFAULT;
            $.dmDisplayFixedOutput = DMDFO_DEFAULT;
            $.dmFields =
                DM_DISPLAYORIENTATION | DM_DISPLAYFIXEDOUTPUT | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
        }
    }

    ChangeDisplaySettingsW(_ ? &$ : NULL, CDS_FULLSCREEN);
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
            BOOL _ = FALSE;
            DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &_, sizeof(BOOL));
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

    WCHAR szClassName[23] = {};
    GetClassNameW(hwnd, szClassName, 23);

    if (CompareStringOrdinal($.pwszVal, -1, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", -1, TRUE) == CSTR_EQUAL &&
        CompareStringOrdinal(szClassName, -1, L"ApplicationFrameWindow", -1, FALSE) == CSTR_EQUAL)
        hWnd = hwnd;

    PropVariantClear(&$);
    return !hWnd;
}

VOID WinMainCRTStartup()
{
    WCHAR szFileName[MAX_PATH] = {};
    ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\Packages\\Microsoft.MinecraftUWP_"
                              L"8wekyb3d8bbwe\\RoamingState\\Stonecutter\\Display.ini",
                              szFileName, MAX_PATH);

    if ((CreateMutexW(NULL, FALSE, L"Stonecutter.Display") && GetLastError() == ERROR_ALREADY_EXISTS) ||
        GetPrivateProfileIntW(L"Settings", L"Fullscreen", FALSE, szFileName) != TRUE || EnumWindows(EnumWindowsProc, 0))
        goto _;

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