# Building Etterna

Interested in contributing to Etterna? This guide is the place to start!
Once you have the necessary [dependencies](#Universal-Dependencies), you can begin coding on Linux, macOS, amd Windows. Typically, we work off of the `develop` branch, therefore all pull requests should be made towards `develop`.

## Universal Dependencies

- [CMake](https://cmake.org/download/) (Minimum version 3.14.0) - It is reccomended to get this package from the CMake website as many package managers do not have the latest version. Check yours before trying.
- [OpenSSL](https://www.openssl.org/) (Version 1.1.0)
  - Debian: `apt install libssl-dev`
  - Fedora: `dnf install openssl-devel`
  - macOS: `brew install openssl`
  - Windows: A CMake compatible version of OpenSSL is available at [Shining Light Productions](https://slproweb.com/products/Win32OpenSSL.html) website. You will need the 32bit and 64bit installers. Direct links: [32bit](https://slproweb.com/download/Win32OpenSSL-1_1_0j.exe), [64bit](https://slproweb.com/download/Win64OpenSSL-1_1_0j.exe)

### Linux Dependencies

While all dependencies for macOS and Windows are included in the repo, there are some linux libraries which cannot be included in the repo.

- Debian: `apt install libcurl4-openssl-dev libxtst-dev libxrandr-dev libpulse-dev`
- Fedora: `dnf install libXtst-devel libXrandr-devel pulseaudio-libs-devel`

### Windows Dependencies

- [Visual Studio](https://visualstudio.microsoft.com/downloads/) - Currently only `Visual Studio 15 2017` is supported.
- [DirectX Runtimes](https://www.microsoft.com/en-us/download/details.aspx?id=8109) (June 2010)
- [DirectX SDK](https://www.microsoft.com/en-us/download/details.aspx?id=6812)
- [Microsoft Visual C++ Redistributables](http://www.microsoft.com/en-us/download/details.aspx?id=48145) - Both 32bit and 64bit
- [Windows 10 Development Kit](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)

### macOS Dependencies

macOS has no other unique dependencies.

## Project Generation

First, ensure you have forked Etterna, cloned it to your system, and checked out `develop`. Make a folder called "build" within the root of the project. This folder is where all build artifacts will be generated. At the moment, Etterna has game resources in the root of the project, so the output binary is either placed in the root of the project *(Unix)* or in the `Program` folder in the project root *(Windows)*. All generated object files will be placed within the build directory.

To generate project files, run the following CMake command:

```bash
cmake -G "GENERATOR" -DOPENSSL_ROOT_DIR="DIRECTORY" ..
```

We activley support the following CMake generators

- macOS: `Ninja`, `Xcode`, `Unix Makefiles`
- Linux: `Ninja`, `Unix Makefiles`
- Windows: `Visual Studio 15 2017`

For the `OPENSSL_ROOT_DIR` parameter, set the directory for where ever the openssl root directory is located. Here are possible options

- Windows: `C:/OpenSSL-Win32` or `C:/OpenSSL-Win64` if followed above install instructions for OpenSSL
- macOS: `/usr/local/opt/openssl`
- Linux: This parameter is not necessary on linux. (CMake can find it on it's own)

### Sample CMake Commands

```bash
cmake -G "Ninja" ..                                                                 # Linux Ninja
cmake -G "Unix Makefiles" ..                                                        # Linux Makefiles
cmake -DOPENSSL_ROOT_DIR="/usr/local/opt/openssl" -G "Xcode" ..                     # macOS Xcode
cmake -DOPENSSL_ROOT_DIR="/usr/local/opt/openssl" -G "Ninja" ..                     # macOS Ninja
cmake -DOPENSSL_ROOT_DIR="/usr/local/opt/openssl" -G "Unix Makefiles" ..            # macOS Ninja
cmake -DOPENSSL_ROOT_DIR="C:/OpenSSL-Win32" -G "Visual Studio 15 2017" -A Win32 ..  # 32bit Windows
cmake -DOPENSSL_ROOT_DIR="C:/OpenSSL-Win64" -G "Visual Studio 15 2017" -A x64 ..    # 64bit Windows
```

## Compiling

### Ninja

The ninja command used is exactly the same across all operating systems. It should be noted that Ninja can only build 64bit binaries ([unless you would like to compile Ninja yourself](https://github.com/ninja-build/ninja/issues/1339)).

To install ninja, use one of the following commands

- Debian: `apt install ninja-build`
- Fedora: `dnf install ninja-build`
- macOS: `brew install ninja`

To start compiling, run the cmake command with the Ninja generator, then run `ninja`.

### Linux

Run `make` or `ninja` corresponding to the CMake generator you used.

### macOS

#### Xcode Editor

Open the `Etterna.xcodeproj` file generated within the build directory, select the Etterna target, and you are ready to start coding.

#### Xcode CLI

```bash
xcodebuild -project Etterna.xcodeproj -configuration Release
```

Due to the extreme verbosity of `xcodebuild`, we recommend [xcpretty](https://github.com/xcpretty/xcpretty) to clean up the output.

### Windows

#### Visual Studio Editor

Open the `Etterna.sln` file generated within the build directory, and you are ready to start coding.

#### Visual Studio CLI

If you prefer the command line, these commands should be what you are looking for. Make sure you run the proper visual studio command prompt.

```bash
msbuild Etterna.sln /p:Platform="Win32" /p:Configuration="Release"  # Only for 32bit CMake generator
msbuild Etterna.sln /p:Platform="x64" /p:Configuration="Release"  # Only for 64bit CMake generator
```
