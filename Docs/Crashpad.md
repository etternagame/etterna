# Using Google Crashpad with Etterna

Etterna now uses Google Crashpad for crash handling, which gives developers significantly more
information when it comes to discovering why the program crashed, but also affects users when it
comes to submitting crash reports.

## Table of Contents

- [Overview](#Overview)
- [What is a Minidump](#What-is-a-Minidump)
- [Generating Symbols](Generating-Symbols)
  - [Windows Symbol Generation](#Windows-Symbol-Generation)
  - [macOS Symbol Generation](#macOS-Symbol-Generation)
- [Storing Symbols](#Storing-Symbols)
- [Decoding Minidumps](#Decoding-Minidumps)

## Overview

Using Crashpad changes the CI process. For each release, we need to generate symbols for the build.
These symbols contain information about each binary, which let us decode and find exactly what line
of code and function was being run at the time of the crash. When the game starts, a "crashpad
handler" executable is run in the background, and watches the game waiting for it to crash. When it
crashes, it generates a minidump. We can process the minidump with the symbols to see how
the game crashed.

## What is a Minidump

According to [Sentry](https://docs.sentry.io/platforms/native/guides/minidumps/), minidumps (.dmp files) are "files containing the most important memory
regions of a crashed process." This includes the runtime stack, CPU register values, CPU
architecture, and operating system. Since it is exactly what is contained within RAM, some personal
information like usernames, passwords, and anything stored in RAM may be stored in the minidump. It
would take a motivated attack to be able to determine those values in the chance that personal
information is stored within the crashdump. If you would rather not send the Etterna team your
minidump file for debugging the crash you experienced, learn how you can decode
the minidump yourself in the [Decoding Minidumps](#Decoding-Minidumps) section.

## Generating Symbols

Symbol files relate instructions in the compiled binary file to the source code which created it.
You don't need the source code; it's all within the symbol file (which could be upwards of 30-40mb).

### Windows Symbol Generation

1. Using Visual Studio, build `Debug` or `RelWithDebInfo`. A `.pdb` file will be generated alongside
   the executable. All that needs to be done is running `dump_syms` stored in
   `etterna/extern/crashpad/breakpad/`

    ```powershell
    cd etterna/Program
    dump_syms Etterna.pdb > Etterna.sym
    ```

### macOS Symbol Generation

1. Before we can generation symbols, we need to build the game with debug information. That means
setting `CMAKE_BUILD_TYPE` to `Debug` or `RelWithDebInfo` and building the game. The debug
information is stored within the binary, and we'll first want to extract it into its own file.

    ```bash
    cd etterna/
    dsymutil -o Etterna.dsym Etterna.app/Contents/MacOS/Etterna
    ```

    - `-o Etterna.dsym`: A `.dSYM` on macOS is an Xcode debugging symbol folder. This option lets us
      choose the output folder. We include this, otherwise the folder will be generated in the
      `Etterna.app/Contents/MacOS/` folder.

    - `Etterna.app/Contents/MacOS/Etterna` is the actual binary of Etterna. macOS `.app` folders
      contain more than just the binary, so we have to give the path to the executable itself.

2. With the generated `Etterna.dsym` file, we want to make it crashpad compatible. Breakpad
   comes with a tool called `dump_syms` which lets us turn the `.dSYM`
   folder into something that crashpad can understand. The tool can be found in
   `etterna/extern/crashpad/breakpad/`.

    ```bash
    dump_syms -g Etterna.dsym Etterna.app/Contents/MacOS/Etterna > Etterna.sym
    ```

    - `-g Etterna.dsym`: This option includes the dSYM in the symbol file.

    - `Etterna.app/Contents/MacOS/Etterna`: We give the executable file to the `dump_syms` command
      so it knows what binary to relate the `.dsym` symbols with.

    - `> Etterna.sym`: Redirect output to a file, as `dump_syms` will just output to the console.

## Storing Symbols

Now that we have the `Etterna.sym` file, we have everything we need to be able to debug a minidump.
In order for the decoder to read the symbols, it must be in a specific folder format called
"Breakpad Directory Structure." (I don't know if that is the official name, but that is what I'm
going to refer to it as.) That format is `EtternaSymbols/<debug_name>/<breakpad-id>/<sym_name>`. For
Etterna, you can expect symbols to look like `EtternaSymbols/Etterna/<breakpad-id>/Etterna.sym`
where `breakpad-id` will be the random string very first line of the `.sym` file. That is how the
minidump decoder will find what symbol file should be used for a particular minidump. The
`EtternaSymbols` folder name can be whatever you want, that is just the root folder for Etterna
symbols.

## Decoding Minidumps

### Linux and macOS

Crashpad comes with a tool called `minidump_stackwalk` that reads the minidump and symbols, and produces
a stacktrace for the developer. Pass in the minidump file, then the symbol folder as parameters,
and you will get a stacktrace.

```bash
minidump_stackwalk minidumpfile.dmp EtternaSymbols/
```

### Windows

Decoding minidump on Windows is not as easy, but since minidumps have a universal format, you can
decode a Windows minidump on Linux and macOS, as long as you have the corresponding PDB file. You
can find these files on the corresponding releases pages.

You can open a minidump on Windows using `WinDbg` or Visual Studio. `WinDbg` can be found either on the Windows Store
or within the Windows 10 SDK. Opening the minidump alone will provide information about the exception,
but not much else that is readable to a human. For all of the symbol related features, you must point your
debugging program of choice to the exact `.exe` and associated `.pdb` which caused the crash. Access to the Etterna
source files will also give additional information about the source lines which caused the crash.
