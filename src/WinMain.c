#define _MINAPPMODEL_H_
#include <initguid.h>
#include <shobjidl.h>
#include <aclapi.h>
#include <appmodel.h>
#include <shlwapi.h>
#include <userenv.h>

VOID WinMainCRTStartup()
{
    WCHAR szPath[MAX_PATH] = {};
    QueryFullProcessImageNameW(GetCurrentProcess(), (DWORD){}, szPath, &((DWORD){MAX_PATH}));

    HANDLE hObject = CreateMutexW(NULL, FALSE, L"Stonecutter");
    if (hObject && GetLastError())
    {
        INT nArgs = {};
        PWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        for (INT _ = {}; _ < nArgs; _++)
        {
            if (CompareStringOrdinal(pArgs[_], -1, L"-p", -1, FALSE) != CSTR_EQUAL && nArgs > _ + 1)
                continue;

            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, StrToIntW(pArgs[++_]));

            WCHAR szPackageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH] = {};
            GetPackageFamilyName(hProcess, &((UINT32){PACKAGE_FAMILY_NAME_MAX_LENGTH}), szPackageFamilyName);
            if (CompareStringOrdinal(szPackageFamilyName, -1, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", -1, FALSE) ==
                CSTR_EQUAL)
            {
                PathRemoveFileSpecW(szPath);

                PACL pAcl = {};
                GetNamedSecurityInfoW(lstrcatW(szPath, L"\\Stonecutter.dll"), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                      NULL, NULL, &pAcl, NULL, NULL);
                SetEntriesInAclW(PACKAGE_GRAPH_MIN_SIZE,
                                 &((EXPLICIT_ACCESSW){.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE,
                                                      .grfAccessMode = SET_ACCESS,
                                                      .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                                      .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                                  .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                                  .ptstrName = L"ALL APPLICATION PACKAGES"}}),
                                 pAcl, &pAcl);
                SetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);

                LPVOID lpBaseAddress =
                    VirtualAllocEx(hProcess, NULL, sizeof(szPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                WriteProcessMemory(hProcess, lpBaseAddress, szPath, sizeof(szPath), NULL);

                HANDLE hThread =
                    CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, lpBaseAddress, 0, NULL);
                WaitForSingleObject(hThread, INFINITE);
                VirtualFreeEx(hProcess, lpBaseAddress, 0, MEM_RELEASE);
                CloseHandle(hThread);

                HANDLE hModule = LoadLibraryExW(L"ntdll.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
                ((NTSTATUS(*)(HANDLE))GetProcAddress(hModule, "NtResumeProcess"))(hProcess);
                FreeLibrary(hModule);
            }

            CloseHandle(hProcess);
            break;
        }

        CloseHandle(hObject);
        ExitProcess(EXIT_SUCCESS);
    }

    PSID pSid = {};
    DeriveAppContainerSidFromAppContainerName(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &pSid);

    WCHAR szName[MAX_PATH] = {};
    GetAppContainerNamedObjectPath(NULL, pSid, MAX_PATH, szName, &((ULONG){MAX_PATH}));

    HANDLE hMutex = CreateMutexW(NULL, FALSE, lstrcatW(szName, L"\\Stonecutter"));
    BOOL fExists = hMutex && GetLastError();
    CloseHandle(hMutex);

    if (fExists)
    {
        ShellExecuteW(NULL, NULL, L"shell:AppsFolder\\Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL, NULL, SW_HIDE);
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