> [!IMPORTANT]
> - **Stonecutter only supports UWP builds of Minecraft: Bedrock Edition.**
> - **Please use [Igneous](https://github.com/Aetopia/Igneous) instead of GDK builds of Minecraft: Bedrock Edition.** 

> [!CAUTION]
> **Only supported on Windows x64!**

# Stonecutter
Fixes various bugs related to Minecraft: Bedrock Edition.

## Fixes

|Issue|Summary|
|-|-|
|[MCPE-15796](https://bugs.mojang.com/browse/MCPE-15796)|Cursor is not recentered upon the opening of a new gui|
|[MCPE-98861](https://bugs.mojang.com/browse/MCPE-98861)|Significant input delay on devices with Render Dragon|
|[MCPE-109879](https://bugs.mojang.com/browse/MCPE-109879)|Getting disconnected from server when minimizing game or switching focus to another app|
|[MCPE-110006](https://bugs.mojang.com/browse/MCPE-110006)|Vsync not being able to be turned off|
|[MCPE-166745](https://bugs.mojang.com/browse/MCPE-166745)|FPS is capped at double the screen's refresh rate when v-sync is disabled|

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

> [!CAUTION]
> **Consider comparing performance between DirectX 11 & DirectX 12.** 

- Create the following file:
        
  ```
  %LOCALAPPDATA%\Packages\Microsoft.MinecraftUWP_8wekyb3d8bbwe\AC\Stonecutter.ini
  ```
- Add the following contents:
      
  ```ini
  [Stonecutter]
  D3D11 = 1
  ```

- Save the file.

- Launch Minecraft: Bedrock Edition via Stonecutter.

## Build
1. Install & update [MSYS2](https://www.msys2.org):

    ```bash
    pacman -Syu --noconfirm
    ```

3. Install [GCC](https://gcc.gnu.org) & [MinHook](https://github.com/TsudaKageyu/minhook):

    ```bash
    pacman -Syu mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-MinHook --noconfirm
    ```


3. Start MSYS2's `UCRT64` environment & run `Build.cmd`.
