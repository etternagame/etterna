# Using Google Crashpad with Etterna

Etterna now uses Google Crashpad for crash handling, which gives developers significantly more
information when it comes to discovering why the program crashed but also affects users when it
comes to submitting crash reports.

## Table of Contents

- [Overview](#overview)
- [What is a Minidump](#what-is-a-minidump)
- [Preparing System](#preparing-system)
  - [Processing Tools](#required-processing-tools)
  - [Compiling Processing Tools](#compiling-processing-tools)
    - [Building on Linux](#building-on-linux)
    - [Building on macOS](#building-on-macos)
    - [Building on Windows](#building-on-windows)
- [Generating Symbols](#generating-symbols)
  - [Windows Symbol Generation](#windows-symbol-generation)
  - [macOS Symbol Generation](#macos-symbol-generation)
- [Storing Symbols](#storing-symbols)
- [Decoding Minidumps](#decoding-minidumps)

## Overview

Using Crashpad changes the CI process. For each release, we need to generate symbols for the build.
These symbols contain information about each binary, which lets us decode and find exactly what line
of code and function was being run at the time of the crash. When the game starts, a "crashpad
handler" executable is run in the background and watches the game waiting for it to crash. When it
crashes, it generates a minidump. We can process the minidump with the symbols to see how
the game crashed.

## What is a Minidump

According to [Sentry](https://docs.sentry.io/platforms/native/guides/minidumps/), minidumps (.dmp files) are "files containing the most important memory
regions of a crashed process." This includes the runtime stack, CPU register values, CPU
architecture, and operating system. Since it is exactly what is contained within RAM, some personal
information like usernames, passwords, and anything stored in RAM may be stored in the minidump. It
would take a motivated attack to be able to determine those values in the chance that personal
information is stored within the crash dump. If you would rather not send the Etterna team your
minidump file for debugging the crash you experienced, learn how you can decode
the minidump yourself in the [Decoding Minidumps](#decoding-minidumps) section.

## Preparing System

Users must install specific tools developed by Google before attempting to compile crashpad/breakpad, and their processing tools.

### Required Processing Tools

The following tools are necessary:

- `dump_syms`: Generates a text file with all the symbols (variables, functions, line numbers) included to allow for a relationship to be made between the `.dmp` file and the compiled executable. Commands will usually look like the following:

   ```bash
   dump_syms Etterna.pdb > Etterna.sym                                        # Windows
   dump_syms Etterna.dbg Etterna-debug > Etterna-debug.sym                    # Linux
   dump_syms -g Etterna.dsym Etterna.app/Contents/MacOS/Etterna > Etterna.sym # macOS
   ```

- `minidump_stackwalk`: Decodes the `.dmp` file and outputs a stack trace of what error caused the generated `.dmp` file. One of the arguments is a directory to where the symbols get stored. Those symbols must be organized in a specific manner, which is described below. Command will usually look like the following:

   ```bash
   minidump_stackwalk generated_crash_file.dmp EtternaSymbols/ > stacktrace.txt
   ```

### Compiling Processing Tools

Written in a step-by-step process:

1. Get [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up).
   This is a collection of tools which google created to help with the compilation of their projects.
   It must be installed as written on that page and added to your user/system path accordingly.
   After it is added to the path (and restart the terminal session if necessary), you can run `gclient`
   to ensure the path has been properly modified.

2. Get [breakpad](https://chromium.googlesource.com/breakpad/breakpad#getting-started-from-main). While crashpad is the crash-reporting system that Etterna uses, it is the successor to breakpad, and the decoding tools have remained the same.
   It is recommended to create a folder for the breakpad project before downloading it. Once cloned, there will be a `src` folder with the breakpad source code. Commands are as follows:

   ```bash
   mkdir breakpad && cd breakpad
   fetch breakpad
   cd src
   ```

#### Building on Linux

In the `src` directory, run the following commands:

```bash
./configure && make
```

Assuming you have your compiler installed, this will build, and place the tools in the following locations:

*Note: There is another `src` directory in the `src` you are in when you ran the above command. The locations
are relative to the command you run `make` in.*

- `dump_syms`: `src/tools/linux/dump_syms/dump_syms`
- `minidump_stackwalk`: `src/processor/minidump_stackwalk`

#### Building on macOS

In the `src` directory, run the following commands:

```bash
CXXFLAGS="$CXXFLAGS -std=c++17" ./configure && make # Build minidump_stackwalk
xcodebuild -project src/tools/mac/dump_syms/dump_syms.xcodeproj -target dump_syms CLANG_CXX_LANGUAGE_STANDARD=c++17 # Build dump_syms
```

This will build and place the tools in the following locations:

- `minidump_stackwalk`: `src/processor/minidump_stackwalk` 
- `dump_syms`: `src/tools/mac/dump_syms/build/Release`

##### dump_syms

In the `src` directory, run the following commands:

```bash
CXXFLAGS="$CXXFLAGS -std=c++17" ./configure && make
```

#### Building on Windows

Currently `minidump_stackwalk` is not available on Windows. The following steps are
for compiling `dump_syms.exe`

1. Open `src\tools\windows\dump_syms\dump_syms.sln`. It will ask if you want to perform a one-way upgrade.
   Click "Ok", and allow it to upgrade.

2. Select the `Release` build at the upper-left area of the IDE.
  
3. The "Solution Explorer" should show on the right side. There is a folder called `(tools)`. Open the
   dropdown next to that folder, and do the same for the folder inside called `(dump_syms)`. There will
   be a Visual Studio project named `dump_syms`. Right-click it, then click it, then click "Build."
   If you get any errors complaining about `std::unique_ptr`, double-click the error, and add
   `#include <memory>` at the top of the file. The build should work after the includes.
The `dump_syms.exe` will be located in the `src\tools\windows\dump_syms\Release\dump_syms.exe`. At this point,
the executable will run, but it will not generate symbol output until the following step is completed.

4. Open an administrator command prompt, and navigate to the following directory in your Visual Studio
   install.

    ```bash
    cd "C:\Program Files (x86)\Microsoft Visual Studio\2019"
    ```

   Depending on what version of Visual Studio you have installed, you may see `Community`, `BuildTools`, `Professional`, etc
   in this directory. Select any of them and continue into the following directory:

    ```bash
    cd "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\DIA SDK\bin"
    ```

    Run the command

    ```bash
    regsvr32 msdia140.dll
    ```

    This is a DLL which is installed by Visual Studio, but does not get registered to the system,
    so we must register it ourselves. Once that line executes in an admin command prompt, `dump_syms.exe`
    should properly dump symbols.

## Generating Symbols

Symbol files relate instructions in the compiled binary file to the source code which created it.
You don't need the source code; it's all within the symbol file (which could be upwards of 30-40MB).

### Linux Symbol Generation

1. Enter these commands in this order, after building Etterna with `Debug` or `RelWithDebInfo`.

    ```bash
    objcopy --only-keep-debug Etterna Etterna.debug
    dump_syms Etterna.debug Etterna > Etterna.sym
    ```

### Windows Symbol Generation

1. Using Visual Studio, build `Debug` or `RelWithDebInfo`. A `.pdb` file will be generated alongside
   the executable. All that needs to be done is running `dump_syms` stored in
   `etterna/extern/crashpad/breakpad/`

    ```powershell
    cd etterna/Program
    dump_syms Etterna.pdb > Etterna.sym
    ```

### macOS Symbol Generation

0. TL;DR - Enter these commands in this order, after building Etterna with `Debug` or `RelWithDebInfo`.

```bash
cd etterna/
dsymutil -o Etterna.dsym Etterna.app/Contents/MacOS/Etterna
dump_syms -g Etterna.dsym Etterna.app/Contents/MacOS/Etterna > Etterna.sym
```

1. Before we can generation symbols, we need to build the game with debug information. That means
setting `CMAKE_BUILD_TYPE` to `Debug` or `RelWithDebInfo` and building the game. The debug
information is stored within the binary, and we'll first want to extract it into a separate file.

    ```bash
    cd etterna/
    dsymutil -o Etterna.dsym Etterna.app/Contents/MacOS/Etterna
    ```

    - `-o Etterna.dsym`: A `.dSYM` on macOS is an Xcode debugging symbol folder. This option lets us
      choose the output folder. We include this, otherwise, the folder will be generated in the
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
    - `Etterna.app/Contents/MacOS/Etterna`: We give the executable file to the `dump_syms` command,
       so it knows what binary to relate the `.dsym` symbols with.
    - `> Etterna.sym`: Redirect output to a file, as `dump_syms` will just output to the console.

## Storing Symbols

### Summary

1. Open your `Etterna.sym` file, and look at the first line. It should look similar to:

```text
MODULE windows x86_64 7E72B03B469446899BB92B0EB45174FA2f Etterna-RelWithDebInfo.pdb
```

2. Take note of two parts of the above string:

- Build ID: `7E72B03B469446899BB92B0EB45174FA2f`
- Module ID: `Etterna-RelWithDebInfo.pdb`

With the above parts, we must create a directory structure that looks like the following

```text
mkdir -p EtternaSymbols/Etterna-RelWithDebInfo.pdb/7E72B03B469446899BB92B0EB45174FA2f/Etterna.sym
```

Use the `EtternaSymbols` folder when running `minidump_stackwalk`.

### Explanation

Now that we have the `Etterna.sym` file, we have everything we need to be able to debug a minidump.
For the decoder to read the symbols, it must be in a specific folder format called
"Breakpad Directory Structure." (I don't know if that is the official name, but that is what I'm
going to refer to it as.) That format is `EtternaSymbols/<module-id>/<build-id>/<sym_name>`. For
Etterna, you can expect symbols to look like `EtternaSymbols/<module-id>/<build-id>/Etterna.sym`
where `build-id` will be the random string `MODULE` line of the `.sym` file, and `module-id`
will be the last part of the `MODULE` line of the `.sym`. That is how the  minidump decoder will
find what symbol file should be used for a particular minidump. The `EtternaSymbols` folder name
can be whatever you want, that is just the root folder for Etterna symbols.

## Decoding Minidumps

### Linux and macOS

Crashpad comes with a tool called `minidump_stackwalk` that reads the minidump and symbols, then produces
a stack trace for the developer. Pass in the minidump file, then the symbol folder as parameters,
and you will get a stack trace.

```bash
minidump_stackwalk minidumpfile.dmp EtternaSymbols/
```

### Windows

Decoding minidump on Windows is not as easy, but since minidumps have a universal format, you can
decode a Windows minidump on Linux and macOS, as long as you have the corresponding PDB file. You
can find these files on the corresponding releases pages.

You can open a minidump on Windows using `WinDbg` or Visual Studio. `WinDbg` can be found either on the Windows Store
or within the Windows 10 SDK. Opening the minidump alone will provide information about the exception,
but not much else that is readable to a human. For all of the symbol-related features, you must point your
debugging program of choice to the exact `.exe` and associated `.pdb` which caused the crash. Access to the Etterna
source files will also give additional information about the source lines which caused the crash.
