@echo off

cd "%~dp0"
rmdir /Q /S "bin">nul 2>&1
mkdir "bin">nul 2>&1

windres.exe -i "Resources\VersionInfo.rc" -o "%TEMP%\VersionInfo.o"

gcc.exe -shared -nostdlib -static -s "DllMain.c" "%TEMP%\VersionInfo.o" "%SYSTEMROOT%\System32\Kernel32.dll" "%SYSTEMROOT%\System32\ntdll.dll" -lUser32 -ldxgi -ld3d11 -lMinHook -o "bin\Stonecutter.dll"

windres.exe -i "Resources\Resources.rc" -o "%TEMP%\Resources.o"

gcc.exe -mwindows -nostdlib -s "WinMain.c" "%TEMP%\Resources.o" "%TEMP%\VersionInfo.o" -lUserenv -lOle32 -lComctl32 -lKernel32 -lUser32 -lAdvapi32 -o "bin\Stonecutter.exe"

del "%TEMP%\Resources.o" "%TEMP%\VersionInfo.o">nul 2>&1
upx.exe --best --ultra-brute "bin\Stonecutter.dll" "bin\Stonecutter.exe"
powershell.exe Compress-Archive -Path "bin\*" -DestinationPath "bin\Stonecutter.zip" -Force