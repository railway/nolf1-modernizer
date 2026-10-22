# No One Lives Forever Modernizer

[![Build Status](https://dev.azure.com/heytherecoffee/NOLF-Modernizer/_apis/build/status/haekb.nolf1-modernizer?branchName=master)](https://dev.azure.com/heytherecoffee/NOLF-Modernizer/_build/latest?definitionId=3&branchName=master)

The goal of NOLF Modernizer is to help fix some long standing bugs, and update some more outdated features of the game.

## Features

 - Working multiplayer out of the box
 - Cap framerate during menus and gameplay to 60fps
 - Fixed the mouse stutter
 - Optimized performance in select cases
 - Jukebox to play some of your favourite in-game tunes
 - Supports the Game of the Year edition

## Patch 1

 - Fixed a bug with defusing bombs and activating some switches.

## Patch 2

- Added `NoRawInput` to disable mouse raw input.
- Fixed missing continue button on mission summary screen.
- Improved loading screen time (...that I broke, oops!)
- Fixed some invisible impassible geometry.
- Added a experimental developer console, can be toggled on/off with tilde. (`~`)
- Fixed a crash in the weapons hotkey screen.

## Patch 3

- Bumped version to 1.006.
- Patched out GameSpy from dedicated/hosted servers and the server browser.
- Fixed a bug in ai path finding causing values to not always be accurate.
- Fixed a silent out of range bug that could cause enemies to disappear and travel to a nearby galaxy at FTL speed!
- Made the console key rebindable. (It's at the very bottom of the custom controls list.)
- Added Big Head Mode! It's currently a little buggy, but humourous. Check the console command list on how to enable it.
- Included some patched binaries to help improve compatibility. 
- Added a windowed mode toggle to the display options.
- Added anisotropic filtering to advanced performance options.
- Fixed shadows disappearing between cutscenes and saved games.
- Added a "Blackscreen Fix" work around for Intel HD graphics chips in Display options.

## Patch 4
- Re-worked jukebox into an attribute file. 
- Added missing ambient track for the Main Theme to the Jukebox.
- Added some jukebox strings to CRes.dll

## Additional Config/Console Commands

The following are new config/console commands:
  - `FramerateLock`           - INT - Locks the framerate if the value is 1. (Default is 1)
  - `FramerateLockRate`       - FLOAT - The frames a second `FramerateLock` locks to; multiplayer stays at 60. (Default is 60)
  - `ShowFramerate`           - INT - Displays the framerate if the value is 1. (Default is 0)
  - `OldMouseLook`            - INT - Uses the old mouse look code if the value is 1. (Default is 0)
  - `NoFunMenus`              - INT - Only displays the default main menu if the value is 1. (Default is 0)
  - `RestrictCinematicsTo4x3` - INT - Adds black bars onto the sides of cinematics on a non 4x3 resolution, if the value is 1. (Default is 0)
  - `QuickSwitch`             - INT - Instantly switch between weapons, if the value is 1. (Default is 0)
  - `UIScale`                 - FLOAT - Scales the in-game HUD. (Default is 0.5)
  - `UseGotyMenu`             - INT - Switch between the original main menu and the GOTY version. (Default is based on your version)
  - `NoRawInput`              - INT - Disables raw mouse input. (Default is 0)
  - `ConsoleBackdrop`         - INT - Swap between 3 different console backdrops: (0) demo, (1) blanktag, and (2) black. (Default is 0)
  - `BigHeadMode`             - INT - Enable or Disable BigHeadMode. (Default is 0)
  - `DisplayTriggers`         - INT - Shows or Hides a physical representation of level triggers. (Default is 0)
  - `EnableScreenTinting`     - INT - Enable or disable native screen tinting. Disabling will activate an alternate screen tinting. (Default is 1)
  - `EnableLightScale`        - INT - Enable or disable light scaling. (Default is 1)


Most of these commands are also available in their respective options menu.

## Building

You can now compile it using Visual Studio 2019 (Requires C++ and MFC), thanks to the NOLF2's sdk including some key files. They're all included and ready to compile.

The following build configurations are setup to build: 
 - Debug
 - Final Release

If you experience any issues, feel free to open an issue.

### Building on Linux

`build-linux.py` cross-compiles the same configurations (Final Release for CShell/Object, Release for the rest) with clang-cl and lld-link. It reads the source lists and settings straight from the `.vcxproj` files, so no separate project files need maintaining.

Requirements: `python3`, `clang` (with `clang-cl`), `lld`, `llvm` (for `llvm-rc`/`llvm-lib`), `ninja`, and `wine` (only for packaging, to run `lithrez.exe`).

1. Grab the MSVC CRT and Windows SDK headers/libs with [xwin](https://github.com/Jake-Shadle/xwin) (this accepts Microsoft's license):

   ```sh
   xwin --accept-license --arch x86 splat --output ~/.cache/xwin/splat
   ```

2. Build:

   ```sh
   ./build-linux.py            # outputs build/CShell.dll, build/Object.lto, build/CRes.dll
   ./build-linux.py --dist     # also assembles build/dist (like the Azure pipeline), incl. Custom/Modernizer.rez
   ```

   Use `--xwin <dir>` if the splat lives elsewhere. Any other arguments are passed to ninja (e.g. `-k 0`).

Copy the contents of `build/dist` into your NOLF directory, then add the rez via the launcher as described in `nolf-modernizer-readme.txt` (or see below for Wine).

### Playing on Linux (Wine)

Tested with the Game of the Year CDs and Wine 11. The InstallShield installer and the `NOLF.exe` launcher (which insists on disc 2 being in a CD drive) are both skipped; the files are copied by hand and the engine is started directly.

1. **Get the files off the discs.** Mount the CDs, or turn BIN/CUE images into ISOs first (e.g. `bchunk "NOLF Disc 1.bin" "NOLF Disc 1.cue" disc1`), then extract them with `7z x`. Other editions may lay out their discs differently.

2. **Assemble the game directory** from disc 2's `Game` folder, the `.rez` files in `Data` on both discs, and the movies, then add the Modernizer build on top:

   ```sh
   G=~/Games/nolf/NOLF
   mkdir -p $G
   cp -r disc2/Game/. $G/
   cp disc1/Data/*.[rR][eE][zZ] disc2/Data/*.[rR][eE][zZ] $G/
   cp -r disc2/Movies $G/
   cp -r build/dist/. $G/
   ```

3. **Create a Wine prefix** and set the language key the game reads:

   ```sh
   export WINEPREFIX=~/Games/nolf/pfx
   wineboot -i
   wine reg add "HKLM\Software\Monolith Productions\No One Lives Forever\1.0" /v Language /d English /f /reg:32
   ```

4. **Music:** Wine's own DirectMusic doesn't play NOLF's soundtrack. Install the native DirectX 7 DirectMusic DLLs that ship on disc 2:

   ```sh
   cabextract -d dx7 disc2/DirectX7/directx.cab
   W=$WINEPREFIX/drive_c/windows
   for d in dmband dmcompos dmime dmloader dmstyle dmsynth dmusic; do
       cp dx7/$d.dll $W/syswow64/
       wine reg add 'HKCU\Software\Wine\DllOverrides' /v $d /d native /f
       wine 'C:\windows\syswow64\regsvr32.exe' /s $d.dll
   done
   mkdir -p $W/syswow64/drivers && cp dx7/gm16.dls $W/syswow64/drivers/gm.dls
   wine reg add 'HKLM\Software\Microsoft\DirectMusic' /v GMFilePath /t REG_EXPAND_SZ /d '%SystemRoot%\system32\drivers\gm.dls' /f /reg:32
   ```

   Leave `dsound` on Wine's builtin; the DX7 one needs Windows drivers. The patched `d3dim700.dll` from `BIN` is unused under Wine (its `ddraw` implements Direct3D 7 itself) and can stay.

5. **Play** with the rez list the installer would have set up, plus Modernizer:

   ```sh
   cd ~/Games/nolf/NOLF
   WINEPREFIX=~/Games/nolf/pfx wine lithtech.exe \
       -rez NOLF.REZ -rez NOLF2.REZ -rez nolfu003.rez -rez NOLFCRES003.REZ -rez NOLFGOTY.REZ \
       -rez Custom/Modernizer.rez +windowed 1
   ```

   Drop `+windowed 1` for fullscreen. Settings and saves live in the game directory (`autoexec.cfg`, `Save/`), and Modernizer writes a `Debug.log` there that's worth checking if something goes wrong.

### Pose shots (comparing with a port)

For holding the original's rendering against a port's, the same camera poses can be photographed in both:

- `SavePose [name]` (console, `~`) appends the camera's pose to `poses.txt` in the game directory: `name world x y z pitch yaw`, world units and degrees, world like `worlds/m01s01`. Unnamed poses are `<world>_<line>`. Fly around with `mpclip` (typed into the console).
- `+ShotPoses <file>` with `+runworld <world>` photographs the world's poses of that file and quits: `<name>.bmp` next to the file, square pixels 90 degrees across a 4:3 screen (wider across on a wider one, as the game's view), without interface, weapon or player. `+ShotDelay <seconds>` (default 3) is the wait after the level starts. `+ShotWeapon <weapon>` (a weapons.txt name, like `P38`) keeps the player view model in the shots, with that weapon drawn (every weapon is given, as by the full weapons cheat). `+ShotFovX <degrees>` takes them through the field of view the game itself sets at that FovX instead: 90 is the normal view (FovY 78, so not square pixels), 20, 7 and 2 the scope's zoom levels.

  ```sh
  wine lithtech.exe -rez ... +windowed 1 +BitDepth 32 +screenwidth 1024 +screenheight 768 \
      +runworld worlds/m01s01 +ShotPoses shots/poses.txt
  ```

The engine's own screenshot (F8, `Screenshot<n>.bmp`, base name from `SSFile`) comes out black under Wine, so the shots read the frame back through the client API. Starting a world with `+runworld` rewrites `Save/Reload.sav` and `Save/Summary.sav`; back `Save/` up first.

## Contributing

Simply fork and submit a PR (preferbly with a matching issue ticket!) 

Try to keep to the original coding style, with descriptive commit messages. (Unlike some of my original commits!)

## Localization

There have been community efforts to localize Modernizer into other languages. And while I don't have the time to directly help in these efforts, here are some steps you can do you to localize and distribute your localization patch!

First off modify the string table located in CRes.dll (Client Resource). This can be done with the latest version of Visual Studio 2019 and this source code. You may also attempt to use other programs to modify the string table directly in the dll. 

Secondly there are some additional strings in Jukebox.txt located here: https://github.com/haekb/nolf1-modernizer/blob/master/ASSETS/Attributes/Jukebox.txt) 

Finally compile your new CRes.dll and the modified attribute file into its own rez using LithRez.exe (from the SDK) and make sure it loads after Modernizer.rez.

## D3D and 2048 pixel limit

You can grab a patched d3d.ren, and d3dim700.dll from the ASSETS folder or from a release of NOLF Modernizer.

## DGVoodoo2

I don't recommend using this application. I've fixed the majority of the slowdowns caused by old d3d techniques. And (at least on my machine) DGVoodoo2 would cause dynamic lights to absolutely destroy my framerate!

## License

This code is still bound to the original EULA found in the NO ONE LIVES FOREVER Source Code v1.003. This can be viewed in the readme.txt file.