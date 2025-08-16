@echo off
cd "%~dp0\src"

rd /q /s "bin"
rd /q /s "obj"

md "bin"
md "obj"

windres.exe -i "Resources\Program.rc" -o "obj\Program.o"
windres.exe -i "Resources\Library.rc" -o "obj\Library.o"

gcc.exe -Oz -nostdlib -s -Wl,--gc-sections -mwindows "Program.c" "obj\Program.o" -lkernel32 -ladvapi32 -lshell32 -lole32 -lshlwapi -luserenv -o "bin\Stonecutter.exe"
gcc.exe -Oz -nostdlib -s -Wl,--gc-sections,--exclude-all-symbols,--wrap=memcpy,--wrap=memset -static -shared "Library.c" "obj\Library.o" -lminhook -lkernel32 -luser32 -ldxgi -o "bin\Stonecutter.dll"

powershell.exe -Command "$ProgressPreference = 'SilentlyContinue'; Compress-Archive -Path 'bin\*' -DestinationPath 'bin\Stonecutter.zip' -Force"