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

    PVOID hMutex = CreateMutexW(NULL, FALSE, L"Stonecutter");
    if (!hMutex)
        ExitProcess(EXIT_SUCCESS);

    if (GetLastError())
    {
        INT nArgs = {};
        PWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);

        for (INT _ = {}; _ + 1 < nArgs; _++)
            if (CompareStringOrdinal(L"-tid", -1, pArgs[_], -1, FALSE) == CSTR_EQUAL)
            {
                PACL pAcl = {};
                SetEntriesInAclW(PACKAGE_GRAPH_MIN_SIZE,
                                 &(EXPLICIT_ACCESSW){.grfAccessPermissions = GENERIC_ALL,
                                                     .grfAccessMode = SET_ACCESS,
                                                     .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT,
                                                     .Trustee = {.TrusteeForm = TRUSTEE_IS_NAME,
                                                                 .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP,
                                                                 .ptstrName = L"ALL APPLICATION PACKAGES"}},
                                 NULL, &pAcl);

                PathRenameExtensionW(szPath, L".dll");
                SetNamedSecurityInfoW(szPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pAcl, NULL);
               
                LocalFree(pAcl);

                PVOID hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, StrToIntW(pArgs[++_])),
                      hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessIdOfThread(hThread)),
                      pAddress =
                          VirtualAllocEx(hProcess, NULL, sizeof(szPath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

                WriteProcessMemory(hProcess, pAddress, szPath, sizeof(szPath), NULL);
                QueueUserAPC((PVOID)LoadLibraryW, hThread, (ULONG_PTR)pAddress);

                ResumeThread(hThread);
                CloseHandle(hThread);
                CloseHandle(hProcess);
                break;
            }

        LocalFree(pArgs);
    }
    else
    {
        PVOID pSid = {}, pSettings = {}, pManager = {};
        WCHAR szMutex[MAX_PATH] = {}, szPackage[PACKAGE_FULL_NAME_MAX_LENGTH + 1] = {};

        DeriveAppContainerSidFromAppContainerName(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &pSid);
        GetAppContainerNamedObjectPath(NULL, pSid, MAX_PATH, szMutex, &((ULONG){MAX_PATH}));
        FreeSid(pSid);

        PVOID hMutex = CreateMutexW(NULL, FALSE, lstrcatW(szMutex, L"\\Stonecutter"));
        BOOL fExists = hMutex && GetLastError();
        CloseHandle(hMutex);

        CoInitialize(NULL);
        CoCreateInstance(&CLSID_ApplicationActivationManager, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IApplicationActivationManager, &pManager);

        if (!fExists)
        {
            CoCreateInstance(&CLSID_PackageDebugSettings, NULL, CLSCTX_INPROC_SERVER, &IID_IPackageDebugSettings,
                             &pSettings);
            GetPackagesByPackageFamily(L"Microsoft.MinecraftUWP_8wekyb3d8bbwe", &(UINT32){PACKAGE_GRAPH_MIN_SIZE},
                                       &(PWSTR){}, &(UINT32){PACKAGE_FULL_NAME_MAX_LENGTH + 1}, szPackage);

            IPackageDebugSettings_TerminateAllProcesses(pSettings, szPackage);
            IPackageDebugSettings_DisableDebugging(pSettings, szPackage);
            IPackageDebugSettings_EnableDebugging(pSettings, szPackage, szPath, NULL);
        }

        IApplicationActivationManager_ActivateApplication(pManager, L"Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL,
                                                          AO_NOERRORUI, &(DWORD){});

        if (!fExists)
        {
            IPackageDebugSettings_DisableDebugging(pSettings, szPackage);
            IPackageDebugSettings_EnableDebugging(pSettings, szPackage, NULL, NULL);
            IPackageDebugSettings_Release(pSettings);
        }

        IApplicationActivationManager_Release(pManager);
        CoUninitialize();
    }

    CloseHandle(hMutex);
    ExitProcess(EXIT_SUCCESS);
}