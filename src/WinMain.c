#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define _MINAPPMODEL_H_

#include <initguid.h>
#include <aclapi.h>
#include <shlwapi.h>
#include <appmodel.h>
#include <shobjidl.h>

VOID WinMainCRTStartup()
{
    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");

    if (hMutex)
    {
        WCHAR szPath[MAX_PATH] = {};
        QueryFullProcessImageNameW(GetCurrentProcess(), (DWORD){}, szPath, &(DWORD){MAX_PATH});

        if (GetLastError())
        {
            INT nArgs = {};
            PWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
            DWORD dwProcessId = {}, dwThreadId = {};

            for (INT _ = {}; _ + 1 < nArgs; _++)
                if (CompareStringOrdinal(L"-p", -1, pArgs[_], -1, FALSE) == CSTR_EQUAL)
                    dwProcessId = StrToIntW(pArgs[++_]);
                else if (CompareStringOrdinal(L"-tid", -1, pArgs[_], -1, FALSE) == CSTR_EQUAL)
                    dwThreadId = StrToIntW(pArgs[++_]);

            LocalFree(pArgs);

            PathRenameExtensionW(szPath, L".dll");

            PACL pAcl = {};
            SetEntriesInAclW(PACKAGE_GRAPH_MIN_SIZE,
                             &(EXPLICIT_ACCESSW){.grfAccessPermissions = GENERIC_ALL,
                                                 .grfAccessMode = SET_ACCESS,
                                                 .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                                 .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                             .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                             .ptstrName = L"ALL APPLICATION PACKAGES"}},
                             NULL, &pAcl);
            SetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);

            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);

            PVOID pAddress = VirtualAllocEx(hProcess, NULL, sizeof(szPath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            WriteProcessMemory(hProcess, pAddress, szPath, sizeof(szPath), NULL);

            HANDLE hThread =
                CreateRemoteThread(hProcess, NULL, (SIZE_T){}, (PTHREAD_START_ROUTINE)LoadLibraryW, pAddress, 0, NULL);
            WaitForSingleObject(hThread, INFINITE);

            VirtualFreeEx(hProcess, pAddress, (SIZE_T){}, MEM_RELEASE);
            CloseHandle(hThread);
            CloseHandle(hProcess);

            hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, dwThreadId);
            ResumeThread(hThread);
            CloseHandle(hThread);
        }
        else
        {
            CoInitialize(NULL);

            IPackageDebugSettings *pSettings = {};
            CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                             (PVOID *)&pSettings);

            IApplicationActivationManager *pManager = {};
            CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                             &IID_IApplicationActivationManager, (PVOID *)&pManager);

            WCHAR szName[PACKAGE_FULL_NAME_MAX_LENGTH + 1] = {};
            GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &(UINT32){PACKAGE_GRAPH_MIN_SIZE},
                                       &(PWSTR){}, &(UINT32){sizeof(szName) / sizeof(WCHAR)}, szName);

            IPackageDebugSettings_TerminateAllProcesses(pSettings, szName);
            IPackageDebugSettings_DisableDebugging(pSettings, szName);
            IPackageDebugSettings_EnableDebugging(pSettings, szName, szPath, NULL);

            IApplicationActivationManager_ActivateApplication(pManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App",
                                                              NULL, AO_NOERRORUI, &(DWORD){});
            IPackageDebugSettings_DisableDebugging(pSettings, szName);
            IPackageDebugSettings_EnableDebugging(pSettings, szName, NULL, NULL);

            IApplicationActivationManager_Release(pManager);
            IPackageDebugSettings_Release(pSettings);

            CoUninitialize();
        }
    }

    CloseHandle(hMutex);

    ExitProcess(EXIT_SUCCESS);
}