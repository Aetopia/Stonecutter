#define _MINAPPMODEL_H_
#include <initguid.h>
#include <shobjidl.h>
#include <aclapi.h>
#include <appmodel.h>
#include <shlwapi.h>

VOID WinMainCRTStartup()
{
    WCHAR szPath[MAX_PATH] = {};
    QueryFullProcessImageNameW(GetCurrentProcess(), (DWORD){}, szPath, &((DWORD){MAX_PATH}));

    HANDLE hObject = CreateMutexW(NULL, FALSE, L"Stonecutter");
    if (!hObject)
        ExitProcess(EXIT_SUCCESS);
    else if (GetLastError())
    {
        PathRemoveFileSpecW(szPath);

        PACL pAcl = {};
        GetNamedSecurityInfoW(lstrcatW(szPath, L"\\Stonecutter.dll"), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL,
                              NULL, &pAcl, NULL, NULL);
        SetEntriesInAclW(PACKAGE_GRAPH_MIN_SIZE,
                         &((EXPLICIT_ACCESSW){.grfAccessPermissions = GENERIC_ALL,
                                              .grfAccessMode = SET_ACCESS,
                                              .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                              .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                          .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                          .ptstrName = L"ALL APPLICATION PACKAGES"}}),
                         pAcl, &pAcl);
        SetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);

        PWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &((INT){}));

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, StrToIntW(pArgs[2]));
        LPVOID lpBaseAddress = VirtualAllocEx(hProcess, NULL, sizeof(szPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        WriteProcessMemory(hProcess, lpBaseAddress, szPath, sizeof(szPath), NULL);
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, (SIZE_T){}, (LPTHREAD_START_ROUTINE)LoadLibraryW,
                                            lpBaseAddress, 0, NULL);

        WaitForSingleObject(hThread, INFINITE);
        VirtualFreeEx(hProcess, lpBaseAddress, (SIZE_T){}, MEM_RELEASE);
        CloseHandle(hThread);
        CloseHandle(hProcess);

        hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, StrToIntW(pArgs[4]));
        ResumeThread(hThread);
        CloseHandle(hThread);

        CloseHandle(hObject);
        ExitProcess(EXIT_SUCCESS);
    }

    CoInitialize(NULL);

    IPackageDebugSettings *pSettings = {};
    CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                     (LPVOID *)&pSettings);

    IApplicationActivationManager *pManager = {};
    CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                     &IID_IApplicationActivationManager, (LPVOID *)&pManager);

    WCHAR szPackageFullName[PACKAGE_FULL_NAME_MAX_LENGTH] = {};
    GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &((UINT){PACKAGE_GRAPH_MIN_SIZE}), (PWSTR[]){},
                               &((UINT32){PACKAGE_FULL_NAME_MAX_LENGTH}), szPackageFullName);

    pSettings->lpVtbl->TerminateAllProcesses(pSettings, szPackageFullName);
    pSettings->lpVtbl->DisableDebugging(pSettings, szPackageFullName);
    pSettings->lpVtbl->EnableDebugging(pSettings, szPackageFullName, szPath, NULL);

    pManager->lpVtbl->ActivateApplication(pManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL, AO_NOERRORUI,
                                          &((DWORD){}));
    pSettings->lpVtbl->DisableDebugging(pSettings, szPackageFullName);
    pSettings->lpVtbl->EnableDebugging(pSettings, szPackageFullName, NULL, NULL);

    CloseHandle(hObject);
    ExitProcess(EXIT_SUCCESS);
}