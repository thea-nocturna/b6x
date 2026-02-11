#  B6X Development Documentation

Welcome to the technical documentation for developing on the **B6X** - an alternative system based on the UXN architecture.

This document is intended for developers who want to create games and software for B6X, and provides a detailed description of the platform’s key aspects.

Here you will find code examples, memory maps, command formats, and important notes on compatibility and performance. The document also explains how to sign a plain UXN ROM using the `b6xzp` utility to make it valid for B6X.

It is recommended to familiarize yourself with the basics of UXN and the UXNTAL language before using this documentation. For a quick start, refer to the [README.md](../README.md), which covers building, running, and the main features of the emulator.

## BIOS

A distinctive feature of B6X is the presence of a BIOS - software that runs on every system startup and reset. It initializes hardware, verifies the ROM and its metadata, and then transfers control to the code within the ROM itself.

Unlike user-loaded ROMs, the BIOS begins execution from address `0x0000`, i.e., from the zero page of memory. This is an important characteristic of B6X to keep in mind. Official UXN documentation states that the zero page is reserved and initialized at boot, persisting across soft resets. In B6X, however, you must initialize the zero page **yourself** before using it, as it contains remnants of the BIOS code.

The typical B6X BIOS sequence is:

1.  Device initialization.
2.  Verification of the ROM signature.
3.  Verification of the ROM's target system.
4.  Verification of the ROM's target version compatibility.
5.  Verification of the ROM checksum.
6.  Display of error or other information if issues are detected.
7.  Loading the ROM into memory and transferring control upon success.

When transferring control to user ROM code, the BIOS jumps to address `0x0100` - the standard reset vector in UXN systems.

The BIOS code itself is embedded in the emulator and located in the header file `include/bios.h`. The BIOS source code is not currently included in the repository because building it requires a modified version of `uxnasm` with an unlocked zero page.

## Zero Page

A valid ROM for UXN/B6X has the following zero page structure:

| Offset   | Purpose                                           |
| :------- | :------------------------------------------------ |
| `0x0000` | UXN ROM signature. Always `UXNR` (4 bytes).       |
| `0x0004` | ROM target system. Always `B6X` (string).         |
| `0x000E` | ROM target version. Defined by developer (word).  |
| `0x0010` | ROM title. Defined by developer (string).         |
| `0x0040` | ROM author. Defined by developer (string).        |
| `0x0060` | ROM checksum from the first page onward (word).   |
| `0x0062` | Number of pages in the ROM (word).                |
| `0x0070` | Unix timestamp of ROM signing.                    |
| `0x0080` | Reserved.                                         |
| `0x0100` | Reset vector, end of zero page.                   |

Most developers will likely use the uxntal language, whose assembler locks the zero page for writing and excludes it from the resulting ROM, or another language targeting the UXN ecosystem which behaves similarly. For this scenario, B6X provides a utility:

```
$ b6xzp -h

Usage: b6xzp [flags] <input> <output>
Append B6X header to input UXN ROM.
Extend ROM to the nearest whole page.

Flags:
  -h            Show this help message
  -t  <title>   Title (up to 48 chars)
  -c  <author>  Author string (up to 32 chars)
  -v  <version> Target B6X version (16-bit HEX, default: 0000)
 [-i] <input>   Input ROM (use '-' for stdin)
 [-o] <output>  Output ROM (required)
```

This utility will add a header/zero page to your plain UXN ROM and extend it to a whole number of pages, making it valid for B6X.

```
$ uxnasm sprites.tal sprites.rom
$ b6xzp -t "Sprites Demo" -c "Thea N." sprites.rom sprites.b6x
$ b6x sprites.b6x
```

## Devices

Like any other UXN system, B6X operates with devices, communicating through I/O ports. Its set of ports is concise and significantly less complex compared to UXN/Varvara: all devices fit within 16 ports.

| Port | Purpose   | Port | Purpose    | Port | Purpose  | Port | Purpose    |
| :--- | :------   | :--- | :--------- | :--- | :------- | :--- | :--------- |
| `00` | `----`    | `04` | `WST`      | `08` | `ROM`(H) | `0C` | `VDP` (H)  |
| `01` | `----`    | `05` | `RST`      | `09` | `ROM`(L) | `0D` | `VDP` (L)  |
| `02` | `SND` (H) | `06` | `META` (H) | `0A` | `CTL`(H) | `0E` | `DEBUG`    |
| `03` | `SND` (L) | `07` | `META` (L) | `0B` | `CTL`(L) | `0F` | `----`     |

In the scheme above, "(H)" denotes the high byte and "(L)" the low byte. Some devices use 2 ports and operate with 16-bit values.

### Video Display Processor

The Video Display Processor, abbreviated `VDP` in the context of B6X, is the device that powers the graphics subsystem. It performs the full rendering cycle and produces the on-screen image. The VDP handles sprites, tile layers, text, and colorization of on-screen content. It renders at a resolution of **320x224** with a refresh rate of **60 FPS** and a **12-bit color depth**.

#### VDP Memory Overview

The graphics subsystem uses the following memory areas for its operation:

| Memory          | Size/Count | Type  |
| :-------------- | :--------- | :---- |
| Registers       | 16         | Words |
| CRAM (Palettes) | 64         | Words |
| VRAM            | 65536      | Bytes |
| CGRAM (Font)    | 1024       | Bytes |

Sixteen registers serve various functions, three of which are intended for user use. Each stores a 16-bit value.

| #   | Purpose                                     |
| :-- | :------------------------------------------ |
| `0` | Command (cleared after execution)           |
| `1` | Graphics subsystem mode and state           |
| `2` | User register A                             |
| `3` | User register B                             |
| `4` | User register C                             |
| `5` | H-Blank vector address in RAM               |
| `6` | H-Blank vector trigger row                  |
| `7` | V-Blank vector address in RAM               |
| `8` | Text buffer address in VRAM                 |
| `9` | Sprite Attribute Table address in VRAM      |
| `a` | Nametable address for tile layer A in VRAM  |
| `b` | Horizontal scroll for layer A in VRAM       |
| `c` | Vertical scroll for layer A in VRAM         |
| `d` | Nametable address for tile layer B in VRAM  |
| `e` | Horizontal scroll for layer B in VRAM       |
| `f` | Vertical scroll for layer B in VRAM         |

#### VDP I/O Specification

Interaction with the listed memory areas is performed using DEO and DEI operations, passing commands and data through the ports. In the VDP, one command is executed via two DEO/DEI calls in short mode (i.e., operating with 16-bit values). To illustrate the VDP command set, let's introduce the following convention:

```tal
%VDPO { #0c DEO2 #0c DEO2 }
%VDPI { #0c DEO2 #0c DEI2 }
```

##### Register Writing

| Command              | Description                  |
| :------------------- | :--------------------------- |
| `#0000 #0R #00 VDPO` | Clear register               |
| `#HHLL #0R #01 VDPO` | Set register to value `00LL` |
| `#HHLL #0R #02 VDPO` | Set register to value `00HH` |
| `#HHLL #0R #03 VDPO` | Set register to value `LLHH` |

Where `R` is the register number (`0..f`), `HH` is the high byte, and `LL` is the low byte.

##### Palette/CRAM Writing

| Command              | Description                    |
| :------------------- | :----------------------------- |
| `#0000 #ID #04 VDPO` | Clear CRAM entry               |
| `#0BGR #ID #05 VDPO` | Set CRAM entry to value `00GR` |
| `#0BGR #ID #06 VDPO` | Set CRAM entry to value `000B` |
| `#0BGR #ID #07 VDPO` | Set CRAM entry to value `0BGR` |

Where `ID` is the index of the CRAM entry (`00..3f`), `0BGR` is a 16-bit color value.

##### VRAM Writing

| Command              | Description                             |
| :------------------- | :-------------------------------------- |
| `#ADDR #00 #08 VDPO` | Clear VRAM byte                         |
| `#HHLL #0D #09 VDPO` | Write low byte to VRAM                  |
| `#HHLL #0D #0a VDPO` | Write high byte to VRAM                 |
| `#NNNN #SD #0b VDPO` | Copy from RAM to VRAM                   |
| `#NNNN #0D #0c VDPO` | Fill VRAM with zero                     |
| `#HHLL #ND #0d VDPO` | Fill VRAM with low byte                 |
| `#HHLL #ND #0e VDPO` | Fill VRAM with high byte                |
| `#HHLL #ND #0f VDPO` | Fill VRAM with repeating pattern `HHLL` |

Where `S` is the number of the VDP register storing the source address in RAM, `D` is the number of the VDP register storing the destination address in VRAM, `N` is the number of the VDP register storing the size of the data block in bytes, `ADDR` is the VRAM address (16-bit value), `HH` is the high byte, `LL` is the low byte, and `NNNN` is the size of the data block.

##### Setting System Font

`#ADDR #00 #10 VDPO`  -  Copy 1024 bytes from RAM (at `ADDR`) to CGRAM.

##### Reading Commands

| Command        | Description                 |
| :------------- | :-------------------------- |
| `#0R #00 VDPI` | Get value of VDP register   |
| `#ID #01 VDPI` | Get value of CRAM entry     |
| `#0S #02 VDPI` | Get 16-bit value from VRAM  |

Where `S` is the number of the VDP register storing the source address in RAM, `ID` is the index of the CRAM entry (`00..3f`), and `R` is the register number (`0..f`).

---

> [!NOTE]
> **All memories, as well as copy and fill operations, are circular:** going beyond one end is equivalent to entering from the opposite end.

#### VDP Operating Mode

The VDP mode is set by register `1`, which has the following fields:

| Mask     | Bit  | Field                            |
| :------- | :--- | :------------------------------- |
| `0x0001` | 0    | Display text buffer              |
| `0x0002` | 1    | Display sprites                  |
| `0x0004` | 2    | Display tile layer A             |
| `0x0008` | 3    | Display tile layer B             |
| `0x0030` | 4-5  | Palette row for background color |
| `0x0040` | 6    | Enable H-blank vector            |
| `0x0080` | 7    | Enable V-blank vector            |
| `0x1000` | 12   | Register write indicator         |
| `0x2000` | 13   | VRAM write indicator             |
| `0x4000` | 14   | CRAM write indicator             |
| `0x8000` | 15   | CGRAM write indicator            |

> [!WARNING]
> Write indicators are used by the VDP during the rendering cycle. It is strongly advised not to change or overwrite them.

#### Palette

During the rendering cycle, the VDP operates with four palettes of 16 colors each, totaling 64 colors. Only 15 colors from each palette are available for use because the zero entry is treated as transparent when rendering sprites and tiles.

The color of the zero entry from one of the palettes is used as the background color. It is displayed if the rendering of all objects results in a transparent pixel. The palette used for the background color is set via the corresponding field in register `1`.

A palette entry is a 16-bit value with the format `0x0BGR`, where `G` corresponds to the green component, `R` to red, and `B` to blue. This provides a color depth of 12 bits, or 4096 possible colors.

#### Rendering Order

The VDP enforces the following rendering order (from nearest to farthest):

1.  Text buffer.
2.  Sprites with the priority flag set.
3.  Layer A tiles with the priority flag set.
4.  Layer B tiles with the priority flag set.
5.  Sprites without the priority flag.
6.  Layer A tiles without the priority flag.
7.  Layer B tiles without the priority flag.
8.  Background color.

#### Patterns

A pattern is a unit of raster graphics in VRAM, encoded as 32 bytes (4 bits per pixel). Each pattern is 8x8 pixels in size. Patterns start at the first byte of VRAM. VRAM can hold a total of 2048 patterns, including the zero pattern, which is typically ignored during rendering.

#### Sprites

The Sprite Attribute Table (SAT) is a table in VRAM describing all sprites on screen. The table's location in VRAM is set by register `9`. Each sprite is described by four words (8 bytes) with the following structure:

|           | 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
| :-------- |:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
| **BASE1** | PR | PL | PL | VF | HF | ID | ID | ID | ID | ID | ID | ID | ID | ID | ID | ID |
| **BASE2** |    |    |    |    | VS | VS | HS | HS |    | LN | LN | LN | LN | LN | LN | LN |
| **X-POS** |    |    |    |    |    |    |    | XP | XP | XP | XP | XP | XP | XP | XP | XP |
| **Y-POS** |    |    |    |    |    |    |    |    | YP | YP | YP | YP | YP | YP | YP | YP |

| Field | Meaning                                   |
| :---- | :---------------------------------------- |
| `ID`  | Pattern ID in VRAM                        |
| `HF`  | Flip tile/sprite horizontally             |
| `VF`  | Flip tile/sprite vertically               |
| `PL`  | Palette used for rendering (0-3)          |
| `PR`  | Priority flag                             |
| `LN`  | Link  -  Index of next SAT entry            |
| `HS`  | Horizontal size of sprite (0-3)           |
| `VS`  | Vertical size of sprite (0-3)             |
| `XP`  | X-position of sprite on screen (0-511)    |
| `YP`  | Y-position of sprite on screen (0-255)    |

Horizontal and vertical sizes are set as a number from 0 to 3, corresponding to `(SIZE+1)*8` pixels or `SIZE+1` patterns. **The order of patterns in the sprite is column-major.**

The link field allows organizing sprites into chains. The VDP traverses sprite chains during rendering. If the link points to the current entry in the SAT (`SAT[IDX].LINK == IDX`), the chain is terminated.

Sprites are positioned on screen within 0 to 255 pixels vertically and 0 to 511 pixels horizontally, inclusive. The sprite rendering space is circular: exceeding a boundary on one side is equivalent to entering from the opposite side.

> [!NOTE]
> The VDP can render no more than **80** sprites per frame and no more than **32** per line.

#### Tile Layers

Tile layers A and B use a nametable. Each entry in the table is one word (16 bits) with a structure identical to `BASE1` from the SAT. The location of tile layers in VRAM is set by registers `a` and `d`.

Tile layers are organized into a grid of 64x32 tiles (with a tile size of 8x8, this yields 512x256 pixels). Layer scrolling is performed through registers `b`, `c`, `e`, and `f`. The scroll space for layers is circular.

#### Text Buffer

The text buffer is a special display layer rendered on top of the graphics. It is used for debugging, menus, and interfaces. The location of the text buffer in VRAM is set by register `8`.

The text buffer has a size of 40x28 characters. Each character contains a 7-bit character code and 1 bit for color inversion. Characters are rendered with a background color corresponding to the background color from the palette, and the character itself has the background color inverted. Null characters are ignored.

#### H-Blank and V-Blank

Historically, V-blank and H-blank were periods when the electron beam of a cathode-ray tube returned from the end of a line (H-blank) or the end of a frame (V-blank) to the beginning of the next line or frame. In modern emulators and graphics systems, these concepts remain as important timing marks for synchronizing application logic with screen updates.

The H-blank vector is called at the beginning of a screen line (before rendering the pixels of that line). This allows code execution with line precision, useful for:
*   Dynamic palette changes horizontally.
*   Changing scroll parameters mid-screen.
*   Updating VDP registers on the fly.

The V-blank vector is called at the beginning of each frame, before starting to render a new frame. This is the ideal place for:
*   Updating game logic.
*   Preparing graphical data for the next frame.
*   Updating sprite positions.
*   Loading data into VRAM/CRAM.

Vector setup is done through registers `5`, `6`, and `7`. Before use, the corresponding flags in register `1` must also be set.

> [!IMPORTANT]
> Set the flags before exiting the reset vector.

> [!IMPORTANT]
> It is not recommended to rely on the H-blank vector as a frame clock. The H-blank vector is called only when display content is being rendered, not for every frame.

### Controller

B6X supports the use of two 8-button controllers as input devices. Their button mapping is as follows:

| Button | Code (P1) | Code (P2) |
| :----- | :-------- | :-------- |
| Start  | 0         | 8         |
| A      | 1         | 9         |
| B      | 2         | 10        |
| C      | 3         | 11        |
| Up     | 4         | 12        |
| Down   | 5         | 13        |
| Left   | 6         | 14        |
| Right  | 7         | 15        |

To handle button presses and releases, you must set up the corresponding vector via the `0a` port, designated as `CTL`. This can be done with a single DEO2 write.

```tal
|100 ( -> )
    ;input #0a DEO2 ( Set the input vector )
    BRK

@input ( -> )
    !{ &code $1 }
    #0a DEI ;&code LDA ( Get the event code )
    BRK
```

As shown in the example, a vector is set up to handle controller events. The moment the vector triggers is the appropriate time to retrieve the event code (via a single DEI request).

The event code has the structure `0bP000BBBB`, where `P` is the button press flag (absent for release events) and `BBBB` is the button code from 0 to 15. The mapping of codes to buttons and controllers is described in the table above.

### Read-Only Memory

B6X uses ROMs (or ROM cartridges in the context of a physical embodiment) as its medium. These ROMs contain metadata, executable code, and game resources. Access to the `ROM` is performed via port `08` using three DEO2 writes, enabling the reading of a block of data from the ROM.

To describe this operation, let's introduce the following convention:

```
%ROM { #08 DEO2 #08 DEO2 #08 DEO2 }
```

Loading data from ROM is done with the command `#NNNN #DDDD #SSSS ROM`, where `NNNN` is the number of bytes to read, `DDDD` is the address in RAM where the read bytes will be written, and `SSSS` is the ROM page from which the data block is read.

From this description, it follows that the maximum amount of data that can be loaded in one call is 65535 bytes, and the maximum ROM volume addressable is approximately 16 MB (i.e., 65536 pages).

> [!NOTE]
> The ROM read operation is circular: exceeding one end is equivalent to entering from the other end, whether in ROM or RAM.

### Other and Emulator-Specific Ports

In addition to ports for accessing ROM, input devices, video, and audio subsystems, B6X features other ports for stack pointer management, metadata processing, and debugging.

#### Stack Pointer Ports

Writing to and reading from the `RST` and `WST` ports will get or set the value of the stack pointers, which corresponds to one byte. `RST` occupies port `04` and provides access to the return stack pointer, while `WST` occupies port `05` and is intended for the working stack pointer.

#### Metadata Port

The `META` port, occupying ports `06` and `07`, notifies the emulator that ROM metadata is located at the specified address in RAM. The emulator may use this information or ignore it. Typically, users do not need to use this port directly, as it is only required for the BIOS and is used by it accordingly.

#### Debug Port

The `DEBUG` port (`0e`) allows performing a debug action if a non-zero byte is passed to it with a single DEO write. The emulator or physical embodiment of the system may not implement this port, leaving it unused.
    
| Code | Action                                                                              |
| :--- | :---------------------------------------------------------------------------------- |
| `01` | Outputs the contents of the working and return stacks to `STDERR`.                  |
| `02` | Calls `getchar()`, which requests user input from the console and pauses execution. |

---
```
BCE 6YDET XOPOWO
```