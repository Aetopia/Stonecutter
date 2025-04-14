@echo off
cd "%~dp0\src"

rd /q /s "bin"
rd /q /s "obj"

md "bin"
md "obj"

windres.exe -i "Resources\Program.rc" -o "obj\Program.o"
windres.exe -i "Resources\Library.rc" -o "obj\Library.o"

gcc.exe -flto -Ofast -mwindows -nostdlib -s "Program.c" "obj\Program.o" -lUserenv -lShell32 -lShlwapi -lOle32 -lKernel32 -lAdvapi32 -o "bin\Stonecutter.exe"
gcc.exe -fvisibility=hidden -flto -Ofast -shared -nostdlib -s -static -Wl,--wrap=memcpy,--wrap=memset "Library.c" "obj\Library.o" -lMinHook -lKernel32 -lUser32 -lDXGI -o "bin\Stonecutter.dll"

upx.exe --best --ultra-brute "bin\*"
powershell.exe -Command "$ProgressPreference = 'SilentlyContinue'; Compress-Archive -Path 'bin\*' -DestinationPath 'bin\Stonecutter.zip' -Force"