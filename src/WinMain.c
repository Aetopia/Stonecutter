#define _MINAPPMODEL_H_
#include <initguid.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <aclapi.h>
#include <sddl.h>

VOID WinMainCRTStartup()
{
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");
    if (GetLastError())
        goto _;

    WCHAR szLibFileName[MAX_PATH] = {};
    QueryFullProcessImageNameW(GetCurrentProcess(), 0, szLibFileName, &((DWORD){MAX_PATH}));
    for (DWORD _ = lstrlenW(szLibFileName); _ < -1; _--)
        if (szLibFileName[_] == '\\')
        {
            szLibFileName[_ + 1] = '\0';
            break;
        }

    PACL OldAcl = NULL;
    GetNamedSecurityInfoW(lstrcatW(szLibFileName, L"Stonecutter.dll"), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                          NULL, &OldAcl, NULL, NULL);

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

    CoInitialize(NULL);

    IPackageDebugSettings *pPackageDebugSettings = NULL;
    CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                     (LPVOID *)&pPackageDebugSettings);

    WCHAR szPackageFullName[PACKAGE_FULL_NAME_MAX_LENGTH + 1] = {};
    GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &((UINT){1}), (PWSTR[]){},
                               &((UINT32){PACKAGE_FULL_NAME_MAX_LENGTH}), szPackageFullName);
    pPackageDebugSettings->lpVtbl->EnableDebugging(pPackageDebugSettings, szPackageFullName, NULL, NULL);
    pPackageDebugSettings->lpVtbl->Release(pPackageDebugSettings);

    IApplicationActivationManager *pApplicationActivationManager = NULL;
    CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_LOCAL_SERVER, &IID_IApplicationActivationManager,
                     (LPVOID *)&pApplicationActivationManager);
    CoAllowSetForegroundWindow((IUnknown *)pApplicationActivationManager, NULL);

    DWORD dwProcessId = 0;
    pApplicationActivationManager->lpVtbl->ActivateApplication(
        pApplicationActivationManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL, AO_NOERRORUI, &dwProcessId);
    pApplicationActivationManager->lpVtbl->Release(pApplicationActivationManager);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    LPVOID lpBaseAddress =
        VirtualAllocEx(hProcess, NULL, sizeof(szLibFileName), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, lpBaseAddress, szLibFileName, sizeof(szLibFileName), NULL);

    HANDLE hThread =
        CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, lpBaseAddress, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
_:
    CloseHandle(hThread);
    CloseHandle(hProcess);
    CloseHandle(hMutex) ;
    CoUninitialize();
    ExitProcess(0);
}