#include <initguid.h>
#include <windows.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <aclapi.h>
#include <commctrl.h>
#include <sddl.h>

PCWSTR $(PCWSTR pszPackageFullName)
{
    UINT _ = 0;
    PWSTR $ = GetPackagesByPackageFamily(pszPackageFullName, &((UINT32){}), NULL, &_, NULL) == ERROR_INSUFFICIENT_BUFFER
                  ? HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * _)
                  : NULL;
    GetPackagesByPackageFamily(pszPackageFullName, &((UINT32){1}), HeapAlloc(GetProcessHeap(), 0, sizeof(PWSTR)), &_,
                               $);
    return $;
}

HRESULT TaskDialogCallbackProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData)
{
    if (uMsg == TDN_CREATED)
    {
        LPARAM lParam = (LPARAM)LoadImageW(GetModuleHandleW(NULL), NULL, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
        SendMessageW(hWnd, WM_SETICON, ICON_SMALL, lParam);
        SendMessageW(hWnd, WM_SETICON, ICON_BIG, lParam);
    }
    return S_OK;
}

int WinMainCRTStartup()
{
    if ((CreateMutexW(NULL, FALSE, L"Stonecutter") && GetLastError() == ERROR_ALREADY_EXISTS))
        goto _;

    PCWSTR _[] = {$(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe"), $(L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe")};
    int nButton = !!_[1];

    if (_[0] && _[1])
    {
        TaskDialogIndirect(
            &((TASKDIALOGCONFIG){
                .cbSize = sizeof(TASKDIALOGCONFIG),
                .pfCallback = TaskDialogCallbackProc,
                .pszWindowTitle = L"Stonecutter",
                .pszMainIcon = NULL,
                .dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_USE_COMMAND_LINKS,
                .cButtons = 2,
                .pButtons = (TASKDIALOG_BUTTON[]){{.nButtonID = 0, .pszButtonText = L"Minecraft"},
                                                  {.nButtonID = 1, .pszButtonText = L"Minecraft Preview"}}}),
            &nButton, NULL, NULL);
        if (nButton == IDCANCEL)
            goto _;
    }

    WCHAR szLibFileName[MAX_PATH] = {};
    QueryFullProcessImageNameW(GetCurrentProcess(), 0, szLibFileName, &((DWORD){MAX_PATH}));

    for (DWORD _ = lstrlenW(szLibFileName); _ < -1; _--)
        if (szLibFileName[_] == '\\')
        {
            szLibFileName[_ = _ + 1] = '\0';
            SetCurrentDirectoryW(szLibFileName);
            break;
        }

    PACL OldAcl = NULL;
    GetNamedSecurityInfoW(lstrcatW(szLibFileName, L"Stonecutter.DirectX.dll"), SE_FILE_OBJECT,
                          DACL_SECURITY_INFORMATION, NULL, NULL, &OldAcl, NULL, NULL);

    PSID Sid = NULL;
    ConvertStringSidToSidW(L"S-1-15-2-1", &Sid);

    PACL NewAcl = NULL;
    SetEntriesInAclW(
        1,
        &((EXPLICIT_ACCESSW){
            .grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE,
            .grfAccessMode = SET_ACCESS,
            .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
            .Trustee = {.TrusteeForm = TRUSTEE_IS_SID, .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP, .ptstrName = Sid}}),
        OldAcl, &NewAcl);

    SetNamedSecurityInfoW(szLibFileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NewAcl, NULL);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IPackageDebugSettings *pPackageDebugSettings = NULL;
    CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                     (LPVOID *)&pPackageDebugSettings);

    IApplicationActivationManager *pApplicationActivationManager = NULL;
    CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IApplicationActivationManager, (LPVOID *)&pApplicationActivationManager);
    CoAllowSetForegroundWindow((IUnknown *)pApplicationActivationManager, NULL);

    DWORD dwProcessId = 0;
    pPackageDebugSettings->lpVtbl->EnableDebugging(pPackageDebugSettings, _[nButton], NULL, NULL);
    pApplicationActivationManager->lpVtbl->ActivateApplication(
        pApplicationActivationManager,
        nButton ? L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe!App" : L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App",
        NULL, AO_NOERRORUI, &dwProcessId);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    LPVOID lpBaseAddress =
        VirtualAllocEx(hProcess, NULL, sizeof(WCHAR) * MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    WriteProcessMemory(hProcess, lpBaseAddress, szLibFileName, sizeof(WCHAR) * MAX_PATH, NULL);
    HANDLE hThread =
        CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, lpBaseAddress, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    PROCESS_INFORMATION $ = {};
    CreateProcessW(NULL, nButton ? L"Stonecutter.Display.exe 1" : L"Stonecutter.Display.exe 0", NULL, NULL, FALSE, 0,
                   NULL, NULL, &((STARTUPINFOW){}), &$);
    CloseHandle($.hProcess);
    CloseHandle($.hThread);
_:
    ExitProcess(0);
    return 0;
}