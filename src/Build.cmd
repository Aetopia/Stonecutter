@echo off

cd "%~dp0"
rmdir /Q /S "bin">nul 2>&1
mkdir "bin">nul 2>&1

echo Compiling "Stonecutter.Game.dll"...
windres.exe -i "Resources\Stonecutter.Game.rc" -o "%TEMP%\.o"
gcc.exe -Os -Wl,--gc-sections -shared -nostdlib -static -s "Stonecutter.Game.c" "%TEMP%\.o" -lMinHook -lKernel32 -lucrt -lUser32 -lDXGI -lD3D11 -o "bin\Stonecutter.Game.dll"

echo Compiling "Stonecutter.Display.exe"...
windres.exe -i "Resources\Stonecutter.Display.rc" -o "%TEMP%\.o"
gcc.exe -Os -Wl,--gc-sections -mwindows -nostdlib -s "Stonecutter.Display.c" "%TEMP%\.o" -lDwmapi -lShell32 -lOle32 -lUser32 -lKernel32 -o "bin\Stonecutter.Display.exe"

echo Compiling "Stonecutter.exe"...
windres.exe -i "Resources\Stonecutter.rc" -o "%TEMP%\.o"
gcc.exe -Os -Wl,--gc-sections -mwindows -nostdlib -s "Stonecutter.c" "%TEMP%\.o" -lOle32 -lKernel32 -lAdvapi32 -o "bin\Stonecutter.exe"

echo Compressing...
del "%TEMP%\.o">nul 2>&1
upx.exe --best --ultra-brute "bin\Stonecutter.Game.dll" "bin\Stonecutter.Display.exe" "bin\Stonecutter.exe">nul 2>&1
powershell.exe -Command "$ProgressPreference = 'SilentlyContinue'; Compress-Archive -Path 'bin\*' -DestinationPath 'bin\Stonecutter.zip' -Force">nul 2>&1