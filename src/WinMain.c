#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include <initguid.h>
#include <windows.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <aclapi.h>
#include <commctrl.h>
#include <sddl.h>

int WinMainCRTStartup()
{
    if (CreateMutexW(NULL, TRUE, L"Stonecutter") && GetLastError() == ERROR_ALREADY_EXISTS)
        ExitProcess(0);

    int nButton = 0;
    TaskDialogIndirect(
        &((TASKDIALOGCONFIG){.cbSize = sizeof(TASKDIALOGCONFIG),
                             .hMainIcon = LoadImageW(GetModuleHandleW(NULL), NULL, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR),
                             .pszWindowTitle = L"Stonecutter",
                             .pszMainInstruction = L"Play",
                             .pszContent = L"Select what to play.",
                             .dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_HICON_MAIN | TDF_SIZE_TO_CONTENT |
                                        TDF_USE_COMMAND_LINKS | TDF_POSITION_RELATIVE_TO_WINDOW,
                             .cButtons = 2,
                             .pButtons = (TASKDIALOG_BUTTON[]){{.nButtonID = -1, .pszButtonText = L"Release"},
                                                               {.nButtonID = -2, .pszButtonText = L"Preview"}}}),
        &nButton, NULL, NULL);
    if (nButton == IDCANCEL)
        ExitProcess(0);

    BOOL fFlag = nButton == -2;

    PCWSTR packageFamilyName =
        fFlag ? L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe" : L"Microsoft.MinecraftUWP_8wekyb3d8bbwe";

    PCWSTR appUserModelId =
        fFlag ? L"Microsoft.MinecraftWindowsBeta_8wekyb3d8bbwe!App" : L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App";

    UINT count = 0, bufferLength = 0;
    GetPackagesByPackageFamily(packageFamilyName, &count, NULL, &bufferLength, NULL);
    PWSTR packageFullName = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) * bufferLength);
    GetPackagesByPackageFamily(packageFamilyName, &count, HeapAlloc(GetProcessHeap(), 0, sizeof(PWSTR) * count),
                               &bufferLength, packageFullName);

    DWORD dwSize = MAX_PATH;
    LPWSTR lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwSize * sizeof(WCHAR));

    while (!QueryFullProcessImageNameW(GetCurrentProcess(), 0, lpBuffer, &dwSize))
        lpBuffer = HeapReAlloc(lpBuffer, 0, lpBuffer, (dwSize += MAX_PATH) * sizeof(WCHAR));

    for (DWORD _ = lstrlenW(lpBuffer); _ < -1; _--)
        if (lpBuffer[_] == '\\')
        {
            lpBuffer[_ = _ + 1] = '\0';
            lstrcatW(lpBuffer = HeapReAlloc(GetProcessHeap(), 0, lpBuffer, dwSize = sizeof(WCHAR) * (_ + 16)),
                     L"Stonecutter.dll");
            break;
        }

    PSID Sid = NULL;
    ConvertStringSidToSidW(L"S-1-15-2-1", &Sid);

    PACL OldAcl = NULL;
    GetNamedSecurityInfoW(lpBuffer, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &OldAcl, NULL, NULL);

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

    IPackageDebugSettings *pSettings = NULL;
    CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                     (LPVOID *)&pSettings);
    pSettings->lpVtbl->TerminateAllProcesses(pSettings, packageFullName);
    pSettings->lpVtbl->EnableDebugging(pSettings, packageFullName, NULL, NULL);

    IApplicationActivationManager *pManager = NULL;
    CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IApplicationActivationManager, (LPVOID *)&pManager);
    DWORD processId = 0;
    pManager->lpVtbl->ActivateApplication(pManager, appUserModelId, NULL, AO_NONE | AO_NOERRORUI, &processId);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, dwSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, dwSize, NULL);
    CloseHandle(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, lpBaseAddress, 0, NULL));
    CloseHandle(hProcess);

    ExitProcess(0);
    return 0;
}