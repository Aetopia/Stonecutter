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
    QueryFullProcessImageNameW(GetCurrentProcess(), (DWORD){}, szPath, &(DWORD){MAX_PATH});

    HANDLE hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");
    if (hMutex)
    {
        if (GetLastError())
        {
            INT nArgs = {};
            PWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);

            for (INT _ = {}; _ + 1 < nArgs; _++)
                if (CompareStringOrdinal(L"-tid", -1, pArgs[_], -1, FALSE) == CSTR_EQUAL)
                {
                    PACL pAcl = {};
                    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, StrToIntW(pArgs[++_]));
                    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessIdOfThread(hThread));
                    PVOID pAddress =
                        VirtualAllocEx(hProcess, NULL, sizeof(szPath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

                    PathRenameExtensionW(szPath, L".dll");
                    SetEntriesInAclW(PACKAGE_GRAPH_MIN_SIZE,
                                     &(EXPLICIT_ACCESSW){.grfAccessPermissions = GENERIC_ALL,
                                                         .grfAccessMode = SET_ACCESS,
                                                         .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                                         .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                                     .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                                     .ptstrName = L"ALL APPLICATION PACKAGES"}},
                                     NULL, &pAcl);
                    SetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);

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
            IPackageDebugSettings *pSettings = {};
            IApplicationActivationManager *pManager = {};
            WCHAR szMutex[MAX_PATH] ={}, szPackage[PACKAGE_FULL_NAME_MAX_LENGTH + 1] = {};

            DeriveAppContainerSidFromAppContainerName(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &pSid);
            GetAppContainerNamedObjectPath(NULL, pSid, MAX_PATH, szMutex, &((ULONG){MAX_PATH}));
            LocalFree(pSid);

            HANDLE hMutex = CreateMutexW(NULL, FALSE, lstrcatW(szMutex, L"\\Stonecutter"));
            BOOL fExists = hMutex && GetLastError();
            CloseHandle(hMutex);

            CoInitialize(NULL);

            CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                             &IID_IApplicationActivationManager, (PVOID *)&pManager);

            if (!fExists)
            {
                CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                                 (PVOID *)&pSettings);

                GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &(UINT32){PACKAGE_GRAPH_MIN_SIZE},
                                           &(PWSTR){}, &(UINT32){PACKAGE_FULL_NAME_MAX_LENGTH + 1}, szPackage);

                IPackageDebugSettings_TerminateAllProcesses(pSettings, szPackage);
                IPackageDebugSettings_DisableDebugging(pSettings, szPackage);
                IPackageDebugSettings_EnableDebugging(pSettings, szPackage, szPath, NULL);
            }

            IApplicationActivationManager_ActivateApplication(pManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App",
                                                              NULL, AO_NOERRORUI, &(DWORD){});

            if (!fExists)
            {
                IPackageDebugSettings_DisableDebugging(pSettings, szPackage);
                IPackageDebugSettings_EnableDebugging(pSettings, szPackage, NULL, NULL);
                IPackageDebugSettings_Release(pSettings);
            }

            IApplicationActivationManager_Release(pManager);
            CoUninitialize();
        }
    }

    CloseHandle(hMutex);
    ExitProcess(EXIT_SUCCESS);
}