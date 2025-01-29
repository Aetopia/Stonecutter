#define _MINAPPMODEL_H_
#include <aclapi.h>
#include <shlwapi.h>
#include <appmodel.h>
#include <initguid.h>
#include <shobjidl.h>

VOID WinMainCRTStartup()
{
    WCHAR szPath[MAX_PATH] = {};
    QueryFullProcessImageNameW(GetCurrentProcess(), (DWORD){}, szPath, &((DWORD){MAX_PATH}));

    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");
    if (hMutex)
    {
        if (!GetLastError())
        {
            CoInitialize(NULL);

            IPackageDebugSettings *pSettings = {};
            CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                             (PVOID *)&pSettings);

            IApplicationActivationManager *pManager = {};
            CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                             &IID_IApplicationActivationManager, (PVOID *)&pManager);

            WCHAR szName[PACKAGE_FULL_NAME_MAX_LENGTH] = {};
            GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &((UINT32){PACKAGE_GRAPH_MIN_SIZE}),
                                       (PWSTR[]){}, &((UINT32){PACKAGE_FULL_NAME_MAX_LENGTH}), szName);

            pSettings->lpVtbl->TerminateAllProcesses(pSettings, szName);
            pSettings->lpVtbl->DisableDebugging(pSettings, szName);
            pSettings->lpVtbl->EnableDebugging(pSettings, szName, szPath, NULL);

            pManager->lpVtbl->ActivateApplication(pManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL,
                                                  AO_NOERRORUI, &((DWORD){}));
            pSettings->lpVtbl->DisableDebugging(pSettings, szName);
            pSettings->lpVtbl->EnableDebugging(pSettings, szName, NULL, NULL);

            pManager->lpVtbl->Release(pManager);
            pSettings->lpVtbl->Release(pSettings);
            CoUninitialize();
        }
        else
        {
            INT nArgs = {};
            PWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
            DWORD dwProcessId = {}, dwThreadId = {};
            for (INT _ = {}; _ < nArgs; _++)
            {
                if (CompareStringOrdinal(L"-p", -1, pArgs[_], -1, FALSE) == CSTR_EQUAL && (_ + 1) < nArgs)
                    dwProcessId = StrToIntW(pArgs[++_]);
                else if (CompareStringOrdinal(L"-tid", -1, pArgs[_], -1, FALSE) == CSTR_EQUAL && (_ + 1) < nArgs)
                    dwThreadId = StrToIntW(pArgs[++_]);
            }
            LocalFree(pArgs);

            PathRenameExtensionW(szPath, L".dll");

            PACL pAcl = {};
            GetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pAcl, NULL, NULL);
            SetEntriesInAclW(PACKAGE_GRAPH_MIN_SIZE,
                             &((EXPLICIT_ACCESSW){.grfAccessPermissions = GENERIC_ALL,
                                                  .grfAccessMode = SET_ACCESS,
                                                  .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                                  .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                              .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                              .ptstrName = L"ALL APPLICATION PACKAGES"}}),
                             pAcl, &pAcl);
            SetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);

            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);

            PVOID pAddress = VirtualAllocEx(hProcess, NULL, sizeof(szPath), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            WriteProcessMemory(hProcess, pAddress, szPath, sizeof(szPath), NULL);

            HANDLE hThread =
                CreateRemoteThread(hProcess, NULL, (SIZE_T){}, (LPTHREAD_START_ROUTINE)LoadLibraryW, pAddress, 0, NULL);
            WaitForSingleObject(hThread, INFINITE);

            VirtualFreeEx(hProcess, pAddress, (SIZE_T){}, MEM_RELEASE);
            CloseHandle(hThread);
            CloseHandle(hProcess);

            hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
            ResumeThread(hThread);
            CloseHandle(hThread);
        }
    }
    CloseHandle(hMutex);

    ExitProcess(EXIT_SUCCESS);
}