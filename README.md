> [!CAUTION]
> **Only supported on Windows x64!**

# Stonecutter
Fixes various bugs related to Minecraft: Bedrock Edition.

## Fixes
- [MCPE-98861](https://bugs.mojang.com/browse/MCPE-98861)

- [MCPE-109879](https://bugs.mojang.com/browse/MCPE-109879)

- [MCPE-15796](https://bugs.mojang.com/browse/MCPE-15796)


## Usage

- Download the latest release from:
    
    - [GitHub Releases](https://github.com/Aetopia/Stonecutter/releases)

        - Extract & run `Stonecutter.exe`.

    - [Scoop](https://scoop.sh)

        ```
        scoop bucket add games
        scoop install stonecutter
        ```

        - Run Stonecutter from the Windows Start Menu.

- Stonecutter will launch and patch Minecraft: Bedrock Edition.

## FAQ

#### How can I disable V-Sync?

- Open the following file:
  ```
  %LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\LocalState\games\com.mojang\minecraftpe\options.txt
  ```

- Set `gfx_vsync` to `0`.

- Save the file.

- Launch Minecraft: Bedrock Edition via Stonecutter.

#### How can I force DirectX 11?

- Create the following file:
        
  ```
  %LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\RoamingState\Stonecutter.ini
  ```
- Add the following contents:
      
  ```ini
  []
  =1
  ```

- Save the file.

- Launch Minecraft: Bedrock Edition via Stonecutter.

## Building
1. Install [MSYS2](https://www.msys2.org/) & [UPX](https://upx.github.io/) for optional compression.
2. Update the MSYS2 Environment until there are no pending updates using:

    ```bash
    pacman -Syu --noconfirm
    ```

3. Install GCC & [MinHook](https://github.com/TsudaKageyu/minhook) using:

    ```bash
    pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-MinHook --noconfirm
    ```

3. Make sure `<MSYS2 Installation Directory>\ucrt64\bin` is added to the Windows `PATH` environment variable.
4. Run [`Build.cmd`](src/Build.cmd).
