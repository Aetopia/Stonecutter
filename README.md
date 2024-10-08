> [!CAUTION]
> **Only supported on Windows x64!**

# Stonecutter
Fixes various bugs related to Minecraft: Bedrock Edition.

## Fixes
<details><summary><a href="https://bugs.mojang.com/browse/MCPE-98861">MCPE-98861 - Significant input delay on devices with Render Dragon</a></summary>

### Cause
The cause for this bug seems to be related to how the game handles input updates.<br>
According to the bug report, the game captures input updates at a fixed frame interval.

The fix, Mojang implemented is through a new option called "Improved Input Response" which seems capture input updates at a reduced frame interval. 
 
But this option is ironically is only available if you have the in game V-Sync **On**.

Thus, the only workaround is to:
- Disable in game V-Sync.

    - [This is borked](#analysis).

- Disable V-Sync through your GPU's control panel.

    - It's best to leave the in game V-Sync **On** to have access to the "Improved Input Response" option.

### Analysis
It seems the culprit for this issue is the in game V-Sync implementation.

Minecraft calls [`IDXGISwapchain::Present(1, 0)`](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present) to enable V-Sync.

We may use a tool like PresentMon to verify this:

```
Minecraft.Windows.exe[6236]:
    0000022714F40AE0 (DXGI): SyncInterval=1 Flags=0 CPU=13.342ms (75.0 fps) Display=13.338ms (75.0 fps) GPU=13.331ms Latency=53.500ms Hardware Composed: Independent Flip
```
Here, the game is syncing with a 75 Hz monitor.

Let's first breakdown what PresentMon is reporting:
|Field|Description|
|-|-|
|SyncInterval|Sync for *n* th vertical blanks (V-Sync).|
|Flags|Flags for swapchain presentation options.|
|CPU|Displays the frametime/framerate being produced by the CPU.|
|Display|Displays the presentation framerate.|
|GPU|Display the frametime/framerate being produced by the GPU.|

- On non-hybrid GPU systems, **Display** & **GPU** metrics are essentially similar.
- When the framerate is uncapped PresentMon will report **CPU** & **GPU** metrics being similar.

If we now disable the in game V-Sync, we get the following from [PresentMon](https://github.com/GameTechDev/PresentMon):

```
Minecraft.Windows.exe[6236]:
    0000022714F40AE0 (DXGI): SyncInterval=0 Flags=0 CPU=6.622ms (151.1 fps) Display=13.333ms (75.1 fps) GPU=6.698ms Latency=25.287ms Hardware: Independent Flip
```

The game is now capped 2× the monitor refresh rate when V-Sync is disabled, this is due to the following reasons:

- Minecraft uses 3 swapchain buffers, 1 front, 2 back.

    - You can verify this by checking [`pDesc->BufferCount`](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_swap_chain_desc1) & by hooking [`IDXGIFactory2::CreateSwapChainForCoreWindow`](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgifactory2-createswapchainforcorewindow).

- According the documentation on [swapchain presentation options](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-present), if `Flags` is set to `0`, a DXGI application will present frames from all buffers.

Here is with 4 buffers, 1 front, 3 back:

```
Minecraft.Windows.exe[3536]:
    0000027126C6AE20 (DXGI): SyncInterval=0 Flags=0 CPU=4.445ms (225.0 fps) Display=13.334ms (75.0 fps) GPU=4.441ms Latency=11.678ms Hardware: Independent Flip
```

Now the game is capped at 3× the monitor's refresh rate.

So what's exactly going on when `IDXGISwapchain::Present(0, 0)` is called?

- Desktop Window Manager is forcing the game to synchronize with the monitor's refresh rate or use V-Sync.

- The maximum framerate is determined by the buffer count.

### Fix
Knowing that the issue is being induced by implicitly forced V-Sync, shouldn't there be a way to disable it?

By using `DXGI_PRESENT_ALLOW_TEARING` & `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING`, an application can notify Desktop Window Manager that it allows screen tearing & shouldn't synchronize with the monitor's refresh rate.

This not only fixes the mentioned issue but also fixes the game's V-Sync **Off** implementation.

Stonecutter implements this fix as follows:

- Hook `IDXGISwapchain::Present`, `IDXGISwapchain::ResizeBuffers` & `IDXGIFactory2::CreateSwapChainForCoreWindow`.
 
- Force V-Sync **Off** & allow for screen tearing by calling `IDXGISwapchain::Present(0, DXGI_PRESENT_ALLOW_TEARING)`.
 
- Have the swapchain support screen tearing by specifying `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` in `IDXGISwapchain::ResizeBuffers` & `IDXGIFactory2::CreateSwapChainForCoreWindow`.

With all of these changes, PresentMon reports the following:

```
Minecraft.Windows.exe[1688]:
    0000028138C8C100 (DXGI): SyncInterval=0 Flags=512 CPU=1.273ms (785.4 fps) Display=1.263ms (791.6 fps) GPU=1.269ms Latency=2.711ms Hardware Composed: Independent Flip
```
</details>

<details><summary><a href="https://bugs.mojang.com/browse/MCPE-109879">MCPE-109879 - Getting disconnected from server when minimizing game or switching focus to another app.</a></summary>

### Cause
UWP apps are controlled by the [Process Lifecycle Manager (PLM)](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/app-lifecycle).

![ ](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/images/updated-lifecycle.png)

The main intent of the Process Lifecycle Manager is to allow the current foreground app to have more system resources by reducing system usage of background apps.<br>
The Process Lifecycle Manager does this by suspending background apps and reclaiming their system resources.<br>

### Analysis
The clear culprit for this issue is the Process Lifecycle Manager suspending apps.

Luckily apps can opt out of this behavior in the following cases:

- Performing prolonged workloads in the background.
  
    - [An app may request to postpone suspension but may not prevent it.](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/run-minimized-with-extended-execution#run-while-minimized)

    - [An app may also use background tasks to perform work in the background.](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/support-your-app-with-background-tasks)

- Performing tasks that must be active at all times.

    - An app may entirely opt out of this behavior but this restricted to specific tasks only like:
        
        - [Playing media in the background.](https://learn.microsoft.com/en-us/windows/uwp/audio-video-camera/background-audio)

        - [Downloading files.](https://learn.microsoft.com/en-us/previous-versions/windows/apps/hh452975)

For Minecraft, none of these cases fit since the root issue is all network activity ceasing due to app suspension.<br>
So opting out to **be not managed by the Process Lifecycle Manager** is desirable.

UWP does provide an API to [opt out of being managed by the Process Lifecycle Manager](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/run-in-the-background-indefinetly).<br>
Though if you use it, you can't submit your app onto the Microsoft Store.<br>
For Minecraft, this is essentially means not being available on the Microsoft Store.

### Fix
Instead of sticking to the UWP API, we can venture into the Windows API to seek a solution.

Luckily [`IPackageDebugSettings::EnableDebugging`](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ipackagedebugsettings-enabledebugging) provides with exactly what we need, the ability to not be managed by the Process Lifecycle Manager.

Another fix would be to have Minecraft migrate from UWP to Windows Desktop breaking from the limitations of UWP.<br>
This has been done with Minecraft: Education Edition which is based off Minecraft: Bedrock Edition.

Stonecutter implements this fix as follows:

- Obtain the full package name of Minecraft.

- Enable debug mode for the package by calling `IPackageDebugSettings::EnableDebugging`.
</details>

<details><summary><a href="https://bugs.mojang.com/browse/MCPE-15796">MCPE-15796 - Cursor is not recentered upon the opening of a new gui.</a></summary>

### Cause
Minecraft: Bedrock Edition doesn't center the cursor automatically when a GUI is shown.

### Analysis

According to the [documentation](https://learn.microsoft.com/en-us/windows/uwp/gaming/relative-mouse-movement), an UWP app can switch between absolute & relative mouse movement on the fly.

It seems that no effort is made to center the cursor when restoring absolute mouse movement.

### Fix
As far as Windows is concerned, this fix can implemented as follows:

- Check the value of [`CoreWindow.PointerCursor`](https://learn.microsoft.com/en-us/uwp/api/windows.ui.core.corewindow.pointercursor).

    - If the value is `null` this indicates a GUI isn't shown or relative mouse movement is active.

    - If the value is not `null` this indicates a GUI is shown or absolute mouse movement is active.

- Center the cursor using [`CoreWindow.PointerPosition`](https://learn.microsoft.com/en-us/uwp/api/windows.ui.core.corewindow.pointerposition) whenever the value is `null`.


Stonecutter implements this fix as follows:

- Hook `__x_ABI_CWindows_CUI_CCore_CICoreWindow->put_PointerCursor` to intercept cursor changes.

- Get the current cursor by calling `__x_ABI_CWindows_CUI_CCore_CICoreWindow->get_PointerCursor`.

- If the cursor is `null`:
  
  - Get the current bounds of the window by calling `__x_ABI_CWindows_CUI_CCore_CICoreWindow->get_Bounds`.

  - Center the cursor using `__x_ABI_CWindows_CUI_CCore_CICoreWindow2->put_PointerPosition`.

</details>

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

- Once selected, Stonecutter will launch and patch the game.

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
