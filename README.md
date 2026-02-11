# B6X

This is the main repository for UXN/B6X (or simply B6X), an alternative entertainment-oriented [UXN](https://wiki.xxiivv.com/site/uxn.html) system.

> [!WARNING]
> This project is in early development and represents a prototype.
> Many features are not yet implemented, and implemented ones are not finalized.
> Until the completed release version (1000), the specification, codebase, and features are subject to change.
> Until that time, compatibility between updates is not guaranteed.

Key features of UXN/B6X include:

*   An authentic graphics stack inspired by, and partially compatible with, the [Sega MegaDrive/Genesis VDP](https://segaretro.org/Sega_Mega_Drive/VDP_general_usage).
*   Dual-plane, multi-directional scrollable graphics at 320x224 resolution, supporting 64 colors per frame, a text buffer, and sprites of various sizes.
*   An advanced 8-channel sample-based sound device (Not Implemented Yet / TODO).
*   ROMs up to 16 MB in size.
*   Input device support for two players.

Main components:

*   `b6x` - a minimal system emulator for running B6X ROMs.
*   `b6xzp` - a utility for signing plain UXN ROMs (further details are available in the [developer documentation](DEVELOPMENT.md#zero-page)).

## Building

Currently, B6X supports two target systems: Windows and Linux. Building requires a classic C toolchain (`gcc`/`clang`, `make`) as well as the [MiniFB](https://github.com/emoon/minifb.git) library (a build with hardware acceleration enabled is recommended).

### Linux/Unix

Linux environments additionally require X11 and OpenGL libraries. Build by running the following command from the repository root:

```
$ make
```

Optionally, you can install B6X system-wide by also running as an administrator:

```
# make install
```

### Windows

Building on Windows is performed via MINGW64. Unlike on Linux, before building B6X, you must place the headers and the compiled static MiniFB library (`libminifb.a`) in the `lib/minifb/` directory relative to the repository root. Afterwards, you can build in the same manner:

```
$ make BACKEND=minifb_win32
```

After a successful build, the `build` directory within the project repository will contain the executable files, ready for use.

## Usage

### Running

At this stage, the UXN/B6X emulator does not require special arguments. Simply run it by specifying a filename or, depending on your graphical shell, drag and drop the ROM file directly onto the emulator executable.

```
$ b6x some-game.b6x
```

Alternatively (if a rebuild-and-run is required):

```
$ make run ROM=some-game.b6x
```

If the emulator is started without a ROM, or if an incorrect, incompatible, corrupted, or non-existent file is selected as the ROM, the [B6X BIOS](DEVELOPMENT.md#bios) will display an error message on screen.

### Controls

| Player 1 Keys | Player 2 Keys | Controller Button |
| :------------ | :------------ | :---------------- |
| Enter         | Numpad 0      | Start             |
| A             | Z             | A                 |
| S             | X             | B                 |
| D             | C             | C                 |
| Arrow Up      | Numpad 5      | Up                |
| Arrow Down    | Numpad 2      | Down              |
| Arrow Left    | Numpad 1      | Left              |
| Arrow Right   | Numpad 3      | Right             |

## Developing for B6X

If you intend to develop your own game or other software that uses B6X as a platform, you can learn the basics about [developing for B6X and its technical aspects](DEVELOPMENT.md).

## TODO

The following tasks and plans are to be completed before the release of UXN/B6X 1000:

*   Specification and implementation of the sound subsystem.
*   Reimplementation of the B6X BIOS with a visual intro when run without a ROM.
*   Inclusion of the B6X BIOS source code in the main repository and build process.
*   B6XDK - standalone assembler, linker, debugging, and other tools.
*   wxB6X - a cross-platform emulator version with an enhanced wxWidgets-based GUI, debugging tools, and more.

## Resources

*   [What is UXN?](https://wiki.xxiivv.com/site/uxn.html)
*   [TAL - the Forth-like assembly language used in the UXN ecosystem.](https://wiki.xxiivv.com/site/uxntal.html)
*   [Varvara - another UXN system.](https://wiki.xxiivv.com/site/varvara.html)