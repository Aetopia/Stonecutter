#define INITGUID
#define COBJMACROS
#define _MINAPPMODEL_H_
#define WIDL_C_INLINE_WRAPPERS

#include <aclapi.h>
#include <shlwapi.h>
#include <userenv.h>
#include <appmodel.h>
#include <shobjidl.h>

#define FileExtension L".dll"
#define ThreadIdArgument L"-tid"
#define MutexPath L"\\" MutexName
#define MutexName L"0B8EAD57-F036-48B2-84AD-B9E40F7568A5"
#define ApplicationUserModelId PackageFamilyName "!App"
#define AllApplicationPackages L"ALL APPLICATION PACKAGES"
#define PackageFamilyName L"Microsoft.MinecraftUWP_8wekyb3d8bbwe"

VOID Inject(PWSTR path)
{
    INT length = 0;
    PWSTR *args = CommandLineToArgvW(GetCommandLineW(), &length);
    PathRenameExtensionW(path, FileExtension);

    for (INT index = 0; index < length; index++)
    {
        if (CompareStringOrdinal(ThreadIdArgument, -1, args[index], -1, FALSE) != CSTR_EQUAL)
            continue;

        TRUSTEEW trustee = {.ptstrName = AllApplicationPackages,
                            .TrusteeForm = TRUSTEE_IS_NAME,
                            .TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP};

        EXPLICIT_ACCESSW access = {.Trustee = trustee,
                                   .grfAccessMode = SET_ACCESS,
                                   .grfAccessPermissions = GENERIC_ALL,
                                   .grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT};

        PACL acl = NULL;
        SetEntriesInAclW(1, &access, NULL, &acl);
        SetNamedSecurityInfoW(path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, acl, NULL);
        LocalFree(acl);

        DWORD size = sizeof(WCHAR) * MAX_PATH;
        HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, StrToIntW(args[++index]));
        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessIdOfThread(thread));
        PVOID address = VirtualAllocEx(process, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        SetThreadDescription(thread, MutexName);
        WriteProcessMemory(process, address, path, size, NULL);
        CloseHandle(process);

        QueueUserAPC((PAPCFUNC)LoadLibraryW, thread, (ULONG_PTR)address);
        ResumeThread(thread);
        CloseHandle(thread);

        break;
    }

    LocalFree(args);
}

VOID Launch(PWSTR path)
{
    CoInitialize(NULL);

    PSID sid = NULL;
    WCHAR name[MAX_PATH] = {};

    DeriveAppContainerSidFromAppContainerName(PackageFamilyName, &sid);
    GetAppContainerNamedObjectPath(NULL, sid, MAX_PATH, name, &((ULONG){}));
    FreeSid(sid);

    HANDLE mutex = CreateMutexW(NULL, FALSE, lstrcatW(name, MutexPath));
    BOOL exists = mutex && GetLastError();
    CloseHandle(mutex);

    IPackageDebugSettings *settings = NULL;
    IApplicationActivationManager *manager = NULL;

    if (!exists)
    {
        REFIID iid = &IID_IPackageDebugSettings;
        REFCLSID clsid = &CLSID_PackageDebugSettings;

        CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, (PVOID *)&settings);
        GetPackagesByPackageFamily(PackageFamilyName, &(UINT32){1}, &(PWSTR){}, &(UINT32){MAX_PATH}, name);

        IPackageDebugSettings_TerminateAllProcesses(settings, name);
        IPackageDebugSettings_DisableDebugging(settings, name);
        IPackageDebugSettings_EnableDebugging(settings, name, path, NULL);
    }

    REFIID iid = &IID_IApplicationActivationManager;
    REFCLSID clsid = &CLSID_ApplicationActivationManager;
    CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, (PVOID *)&manager);

    IApplicationActivationManager_ActivateApplication(manager, ApplicationUserModelId, NULL, AO_NOERRORUI, &(DWORD){});
    IApplicationActivationManager_Release(manager);

    if (!exists)
    {
        IPackageDebugSettings_DisableDebugging(settings, name);
        IPackageDebugSettings_EnableDebugging(settings, name, NULL, NULL);
        IPackageDebugSettings_Release(settings);
    }

    CoUninitialize();
}

VOID WinMainCRTStartup()
{
    WCHAR path[MAX_PATH] = {};
    GetModuleFileNameW(NULL, path, MAX_PATH);

    if (!GetLastError())
    {
        HANDLE mutex = CreateMutexW(NULL, FALSE, MutexName);
        if (mutex)
            (GetLastError() ? Inject : Launch)(path);
        CloseHandle(mutex);
    }

    ExitProcess(EXIT_SUCCESS);
}