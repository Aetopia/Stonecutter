@echo off

cd "%~dp0"
rmdir /Q /S "bin">nul 2>&1
mkdir "bin">nul 2>&1

windres.exe -i "Resources\.rc" -o "%TEMP%\.o"

gcc.exe -shared -nostdlib -static -s "Stonecutter.DirectX.c" "%SYSTEMROOT%\System32\Kernel32.dll" "%SYSTEMROOT%\System32\ntdll.dll" -lUser32 -ldxgi -ld3d11 -lMinHook -o "bin\Stonecutter.DirectX.dll"

gcc.exe -mwindows -nostdlib -s "Stonecutter.c" "%TEMP%\.o" -lOle32 -lComctl32 -lKernel32 -lUser32 -lAdvapi32 -o "bin\Stonecutter.exe"

gcc.exe -mwindows -nostdlib -s "Stonecutter.Display.c" "%TEMP%\.o" -lShell32 -lOle32 -lUser32 -lKernel32 -o "bin\Stonecutter.Display.exe"

del "%TEMP%\.o">nul 2>&1
upx.exe --best --ultra-brute "bin\Stonecutter.DirectX.dll" "bin\Stonecutter.Display.exe" "bin\Stonecutter.exe"
powershell.exe Compress-Archive -Path "bin\*" -DestinationPath "bin\Stonecutter.zip" -Force