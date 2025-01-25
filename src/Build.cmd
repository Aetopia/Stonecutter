@echo off
cd "%~dp0">nul 2>&1
rmdir /Q /S "bin">nul 2>&1
mkdir "bin">nul 2>&1

windres.exe -i "Resources\DllMain.rc" -o "%TEMP%\.o"
gcc.exe -Os -Wl,--gc-sections -shared -nostdlib -static -s "DllMain.c" "%TEMP%\.o" -lMinHook -lKernel32 -lucrtbase -lUser32 -lDXGI -lD3D11 -o "bin\Stonecutter.dll"

windres.exe -i "Resources\WinMain.rc" -o "%TEMP%\.o"
gcc.exe -Os -Wl,--gc-sections -mwindows -nostdlib -s "WinMain.c" "%TEMP%\.o" -lShell32 -lShlwapi -lOle32 -lKernel32 -lAdvapi32 -o "bin\Stonecutter.exe"

del "%TEMP%\.o">nul 2>&1
upx.exe --best --ultra-brute "bin\*">nul 2>&1
powershell.exe -Command "$ProgressPreference = 'SilentlyContinue'; Compress-Archive -Path 'bin\*' -DestinationPath 'bin\Stonecutter.zip' -Force">nul 2>&1