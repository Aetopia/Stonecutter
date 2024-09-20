@echo off

cd "%~dp0"
rmdir /Q /S "bin">nul 2>&1
mkdir "bin">nul 2>&1

windres.exe -i "Resources\Stonecutter.Game.rc" -o "%TEMP%\.o"

gcc.exe -shared -nostdlib -static -s "Stonecutter.Game.c" "%TEMP%\.o" "%SYSTEMROOT%\System32\Kernel32.dll" "%SYSTEMROOT%\System32\ntdll.dll" -lUser32 -ldxgi -ld3d11 -lMinHook -o "bin\Stonecutter.Game.dll"

windres.exe -i "Resources\Stonecutter.Display.rc" -o "%TEMP%\.o"

gcc.exe -mwindows -nostdlib -s "Stonecutter.Display.c" "%TEMP%\.o" -lDwmapi -lShell32 -lOle32 -lUser32 -lKernel32 -o "bin\Stonecutter.Display.exe"

windres.exe -i "Resources\Stonecutter.rc" -o "%TEMP%\.o"

gcc.exe -mwindows -nostdlib -s "Stonecutter.c" "%TEMP%\.o" -lOle32 -lKernel32 -lAdvapi32 -o "bin\Stonecutter.exe"

del "%TEMP%\.o">nul 2>&1
upx.exe --best --ultra-brute "bin\Stonecutter.Game.dll" "bin\Stonecutter.Display.exe" "bin\Stonecutter.exe"
powershell.exe Compress-Archive -Path "bin\*" -DestinationPath "bin\Stonecutter.zip" -Force