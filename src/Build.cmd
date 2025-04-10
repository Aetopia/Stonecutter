@echo off
cd "%~dp0">nul 2>&1

rmdir /Q /S "bin">nul 2>&1
mkdir "bin">nul 2>&1

windres.exe -i "Resources\DllMain.rc" -o "%TEMP%\DllMain.o"
windres.exe -i "Resources\WinMain.rc" -o "%TEMP%\WinMain.o"

gcc.exe -fvisibility=hidden -flto -Ofast -shared -nostdlib -s -static -Wl,--wrap=memcpy,--wrap=memset "DllMain.c" "%TEMP%\DllMain.o" -lMinHook -lKernel32 -lUser32 -lDXGI -o "bin\Stonecutter.dll"
gcc.exe -fvisibility=hidden -flto -Ofast -mwindows -nostdlib -s "WinMain.c" "%TEMP%\WinMain.o" -lUserenv -lShell32 -lShlwapi -lOle32 -lKernel32 -lAdvapi32 -o "bin\Stonecutter.exe"

del "%TEMP%\DllMain.o">nul 2>&1
del "%TEMP%\WinMain.o">nul 2>&1

upx.exe --best --ultra-brute "bin\*">nul 2>&1
powershell.exe -Command "$ProgressPreference = 'SilentlyContinue'; Compress-Archive -Path 'bin\*' -DestinationPath 'bin\Stonecutter.zip' -Force">nul 2>&1