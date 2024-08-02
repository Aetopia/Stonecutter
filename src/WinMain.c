#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include <initguid.h>
#include <windows.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <aclapi.h>
#include <commctrl.h>
#include <sddl.h>

static PCWSTR GetPackageByPackageFamily(PCWSTR packageFamilyName)
{
    UINT count = 0, bufferLength = 0;
    if (GetPackagesByPackageFamily(packageFamilyName, &count, NULL, &bufferLength, NULL) != ERROR_INSUFFICIENT_BUFFER)
        return NULL;
    PWSTR packageFullName = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * bufferLength);
    GetPackagesByPackageFamily(packageFamilyName, &count, HeapAlloc(GetProcessHeap(), 0, sizeof(PWSTR) * count),
                               &bufferLength, packageFullName);
    return packageFullName;
}

static HRESULT TaskDialogCallbackProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData)
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
    const struct
    {
        PCWSTR PackageFullName;
        PCWSTR AppUserModelId;
    } _[] = {
        {GetPackageByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe"),
         L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App"},
        {GetPackageByPackageFamily(L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe"),
         L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe!App"},
    };

    if ((CreateMutexW(NULL, TRUE, L"Stonecutter") && GetLastError() == ERROR_ALREADY_EXISTS))
        goto _;

    int nButton = !!_[1].PackageFullName;
    if (_[0].PackageFullName && _[1].PackageFullName)
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

    DWORD nSize = MAX_PATH;
    LPWSTR lpBuffer = HeapAlloc(GetProcessHeap(), 0, nSize * sizeof(WCHAR));

    while (!QueryFullProcessImageNameW(GetCurrentProcess(), 0, lpBuffer, &nSize))
        lpBuffer = HeapReAlloc(lpBuffer, 0, lpBuffer, (nSize += MAX_PATH) * sizeof(WCHAR));

    for (DWORD iBuffer = lstrlenW(lpBuffer); iBuffer < -1; iBuffer--)
        if (lpBuffer[iBuffer] == '\\')
        {
            lpBuffer[iBuffer = iBuffer + 1] = '\0';
            lstrcatW(lpBuffer = HeapReAlloc(GetProcessHeap(), 0, lpBuffer, nSize = sizeof(WCHAR) * (iBuffer + 16)),
                     L"Stonecutter.dll");
            break;
        }

    PACL OldAcl = NULL;
    GetNamedSecurityInfoW(lpBuffer, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &OldAcl, NULL, NULL);

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
    SetNamedSecurityInfoW(lpBuffer, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NewAcl, NULL);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IPackageDebugSettings *pPackageDebugSettings = NULL;
    CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                     (LPVOID *)&pPackageDebugSettings);

    IApplicationActivationManager *pApplicationActivationManager = NULL;
    CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IApplicationActivationManager, (LPVOID *)&pApplicationActivationManager);

    DWORD dwProcessId = 0;
    pPackageDebugSettings->lpVtbl->EnableDebugging(pPackageDebugSettings, _[nButton].PackageFullName, NULL, NULL);
    pPackageDebugSettings->lpVtbl->TerminateAllProcesses(pPackageDebugSettings, _[nButton].PackageFullName);
    pApplicationActivationManager->lpVtbl->ActivateApplication(pApplicationActivationManager, _[nButton].AppUserModelId,
                                                               NULL, AO_NONE | AO_NOERRORUI, &dwProcessId);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, nSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, NULL);
    CloseHandle(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, lpBaseAddress, 0, NULL));
    CloseHandle(hProcess);
_:
    ExitProcess(0);
    return 0;
}