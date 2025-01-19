#define _MINAPPMODEL_H_
#include <initguid.h>
#include <aclapi.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <userenv.h>

VOID WinMainCRTStartup()
{
    if (CreateMutexW(NULL, FALSE, L"Stonecutter") && GetLastError())
        ExitProcess(EXIT_SUCCESS);

    WCHAR szLibFileName[MAX_PATH] = {};
    QueryFullProcessImageNameW(GetCurrentProcess(), 0, szLibFileName, &((DWORD){MAX_PATH}));
    for (DWORD _ = lstrlenW(szLibFileName); _ < -1; _--)
        if (szLibFileName[_] == '\\')
        {
            szLibFileName[_ + 1] = '\0';
            break;
        }

    PACL pAcl = {};
    GetNamedSecurityInfoW(lstrcatW(szLibFileName, L"Stonecutter.dll"), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                          NULL, &pAcl, NULL, NULL);
    SetEntriesInAclW(PACKAGE_GRAPH_MIN_SIZE,
                     &((EXPLICIT_ACCESSW){.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE,
                                          .grfAccessMode = SET_ACCESS,
                                          .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                          .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                      .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                      .ptstrName = L"ALL APPLICATION PACKAGES"}}),
                     pAcl, &pAcl);
    SetNamedSecurityInfoW(szLibFileName, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);

    CoInitialize(NULL);

    IPackageDebugSettings *pSettings = {};
    CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                     (LPVOID *)&pSettings);

    WCHAR szPackageFullName[PACKAGE_FULL_NAME_MAX_LENGTH] = {};
    GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &((UINT){PACKAGE_GRAPH_MIN_SIZE}), (PWSTR[]){},
                               &((UINT32){PACKAGE_FULL_NAME_MAX_LENGTH}), szPackageFullName);

    PSID pSid = {};
    DeriveAppContainerSidFromAppContainerName(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &pSid);

    WCHAR szName[MAX_PATH] = {};
    GetAppContainerNamedObjectPath(NULL, pSid, MAX_PATH, szName, &((ULONG){MAX_PATH}));

    HANDLE hMutex = CreateMutexW(NULL, FALSE, lstrcatW(szName, L"\\Stonecutter"));
    if (!GetLastError())
    {
        CloseHandle(hMutex);
        pSettings->lpVtbl->DisableDebugging(pSettings, szPackageFullName);
        pSettings->lpVtbl->TerminateAllProcesses(pSettings, szPackageFullName);
    }
    pSettings->lpVtbl->EnableDebugging(pSettings, szPackageFullName, NULL, NULL);

    IApplicationActivationManager *pManager = {};
    CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IApplicationActivationManager, (LPVOID *)&pManager);

    DWORD dwProcessId = {};
    pManager->lpVtbl->ActivateApplication(pManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL, AO_NOERRORUI,
                                          &dwProcessId);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    LPVOID lpBaseAddress =
        VirtualAllocEx(hProcess, NULL, sizeof(szLibFileName), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, lpBaseAddress, szLibFileName, sizeof(szLibFileName), NULL);

    HANDLE hThread =
        CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, lpBaseAddress, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    ExitProcess(EXIT_SUCCESS);
}