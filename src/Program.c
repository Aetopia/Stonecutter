#define INITGUID
#define COBJMACROS
#define _MINAPPMODEL_H_
#define WIDL_C_INLINE_WRAPPERS

#include <aclapi.h>
#include <shlwapi.h>
#include <userenv.h>
#include <appmodel.h>
#include <shobjidl.h>

VOID WinMainCRTStartup()
{
    WCHAR szPath[MAX_PATH] = {};
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    if (GetLastError())
        ExitProcess(EXIT_SUCCESS);

    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");
    if (!hMutex)
        ExitProcess(EXIT_SUCCESS);

    if (GetLastError())
    {
        INT nArgs = {};
        PWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);

        for (INT iArg = {}; iArg + 1 < nArgs; iArg++)
            if (CompareStringOrdinal(L"-tid", -1, pArgs[iArg], -1, FALSE) == CSTR_EQUAL)
            {
                PACL pAcl = {};
                SetEntriesInAclW(1,
                                 &(EXPLICIT_ACCESS_W){.grfAccessMode = SET_ACCESS,
                                                      .grfAccessPermissions = GENERIC_ALL,
                                                      .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                                      .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                                  .ptstrName = L"ALL APPLICATION PACKAGES",
                                                                  .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP}},
                                 NULL, &pAcl);

                PathRenameExtensionW(szPath, L".dll");
                SetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);
                LocalFree(pAcl);

                HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, StrToIntW(pArgs[++iArg])),
                       hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessIdOfThread(hThread));

                PVOID pAddress =
                    VirtualAllocEx(hProcess, NULL, sizeof(szPath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

                WriteProcessMemory(hProcess, pAddress, szPath, sizeof(szPath), NULL);
                QueueUserAPC((PAPCFUNC)LoadLibraryW, hThread, (ULONG_PTR)pAddress);
                ResumeThread(hThread);

                CloseHandle(hThread);
                CloseHandle(hProcess);
                break;
            }

        LocalFree(pArgs);
    }
    else
    {
        PSID pSid = {};
        DeriveAppContainerSidFromAppContainerName(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &pSid);

        WCHAR szName[MAX_PATH] = {};
        GetAppContainerNamedObjectPath(NULL, pSid, MAX_PATH, szName, &((ULONG){MAX_PATH}));
        FreeSid(pSid);

        HANDLE hMutex = CreateMutexW(NULL, FALSE, lstrcatW(szName, L"\\Stonecutter"));
        if (!(hMutex && GetLastError()) | CloseHandle(hMutex) * 0)
        {
            WCHAR szName[PACKAGE_FULL_NAME_MAX_LENGTH + 1] = {};
            GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &(UINT32){1}, &(PWSTR){},
                                       &(UINT32){ARRAYSIZE(szName)}, szName);

            CoInitialize(NULL);

            IPackageDebugSettings *pSettings = {};
            CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                             (PVOID)&pSettings);

            IPackageDebugSettings_TerminateAllProcesses(pSettings, szName);
            IPackageDebugSettings_DisableDebugging(pSettings, szName);
            IPackageDebugSettings_EnableDebugging(pSettings, szName, szPath, NULL);

            IApplicationActivationManager *pManager = {};
            CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                             &IID_IApplicationActivationManager, (PVOID)&pManager);

            IApplicationActivationManager_ActivateApplication(pManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App",
                                                              NULL, AO_NOERRORUI, &(DWORD){});
            IApplicationActivationManager_Release(pManager);

            IPackageDebugSettings_DisableDebugging(pSettings, szName);
            IPackageDebugSettings_EnableDebugging(pSettings, szName, NULL, NULL);
            IPackageDebugSettings_Release(pSettings);

            CoUninitialize();
        }
    }

    CloseHandle(hMutex);
    ExitProcess(EXIT_SUCCESS);
}