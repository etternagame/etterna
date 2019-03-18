# Building

It should be noted that building Etterna on your own from the lastest commit is more likely to behave strangely(Resulting in crashes, profile wipes, etc). Do it at your own risk. It is recommended to use the installers or source files provided with every release.

1. [Windows](#Windows)
2. [Linux](#Linux)
3. [Mac](#Mac)

---

## Windows
## Dependencies(Required libraries)

You will need the following:

1. <a href="https://www.visualstudio.com/">Visual Studio</a>(preferably 2015) with c++ packages.
1. <a href="http://www.cmake.org/">CMake</a> (You can CMake GUI if you're not comfortable with the command window)
1. <a href="http://www.microsoft.com/en-us/download/details.aspx?id=8109">DirectX End-User Runtimes (June 2010)</a>
1. <a href="https://www.microsoft.com/en-us/download/details.aspx?id=6812">DirectX SDK</a>
1. <a href="http://www.microsoft.com/en-us/download/details.aspx?id=48145">Microsoft Visual C++ x86 Redistributable for Visual Studio 2015</a>(If on a 64 bit system grab both x86 and x64)
1. <a href="https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk">Windows 10 DK</a> Is probably needed if you're on Windows 10
1. Having <a href="https://git-scm.com/downloads">Git</a> is also necessary (For submodules), unless someone sent you a zip of the repo with submodules

## Downloading and building

Run this command in a cmd from the folder where you want to download etterna:

git clone --depth=1 https://github.com/etternagame/etterna.git

Make sure you're in the right branch(default is master). Find out which is the latest one by asking someone. As of 7/17 this is "develop". To change your branch to that one open a cmd in a folder inside the repo you cloned(Inside /etterna) and do the following commands.
```
git checkout develop
git submodule update --init
```
Change develop to the branch you want (master is usually the stable branch and develop the playground, unstable one). Remember to run git submodule.

Run cmake (from cmd or cmakeGUI) configured for VS 2015 and open the project files generated(/Build/Etterna.sln). If not using cmakeGUI, the command should look like this(Run from a cmd in /Build/, you might have to change the Visual Studio version to the one you have). There are two possible commands to use, either to setup a 32-bit or 64-bit environment.

```bash
cmake -G "Visual Studio 14 2015" ..         # 32-bit
cmake -G "Visual Studio 14 2015 Win64" ..   # 64-bit
```

Note that an error like this:

![](https://cdn.discordapp.com/attachments/326225923240230923/337716512758562817/unknown.png)

While compiling is completely normal.

In order to build an installer first compile and then run stepmania.nsi(Right click+ run NSIS script) to make the installer.
To play without installing simply compile and open etterna/Program/Etterna.exe or run the Etterna project from VS.

NOTE: You may need to make sure you're compiling as Release(Defaults to Debug)
![](https://cdn.discordapp.com/attachments/326225923240230923/337715335480475650/unknown.png)

NOTE 2: If you get an error like this:
![](http://vivide.re/863GDX4n.png)
Try adding this to StepMania.cpp:

```cpp
#include <Windows.h>
#include <stdio.h>
int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;
```

---

## Linux

### 1-a: Prepare dependencies(Debian Based systems)

Run the following commands on your system depending on your distribution:

#### Debian Based systems

```
sudo apt-get install build-essential git cmake
sudo apt-get install libmad0 libmad0-dev libssl-dev libcurl4-openssl-dev mesa-common-dev libglu1-mesa-dev libglew1.5-dev libxtst-dev libxrandr-dev libpng12-dev libjpeg-dev zlib1g-dev libbz2-dev libogg-dev libvorbis-dev libc6-dev yasm libasound-dev libpulse-dev binutils-dev libgtk2.0-dev libudev-dev libva-dev
```

#### Fedora Based systems

```
dnf install http://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm http://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
dnf install libXrandr-devel libXtst-devel libpng-devel libjpeg-devel zlib-devel libogg-devel libvorbis-devel yasm alsa-lib-devel pulseaudio-libs-devel bzip2-devel jack-audio-connection-kit-devel libva-devel pcre-devel gtk2-devel glew-devel libudev-devel
```

### 2: Clone the etterna git and compile etterna

Open a terminal and:

```bash
git clone --depth=1 https://github.com/etternagame/etterna.git
cd etterna
git submodule update --init
mkdir Build
cd Build
cmake .. -G 'Unix Makefiles'
make
```

Also note you probably want to do "git checkout" to the branch you want. Ask around to find out which one it is(develop is the development branch as of 2/2018)

If it doesn't work you can look at how travis does it(https://travis-ci.org/etternagame/etterna)

### 3: Making a Launcher

If you want to run etterna from a launch button like some desktop environments have, make a shell script like this and set the launch button to run the shell script. This assumes that the etterna folder is ~/etterna. If you don't know already, "~/" is shorthand for the home folder of the current user on Linux.

Make a new emtpy text document and add the following:

```bash
#!/bin/bash
cd ~/etterna
./etterna
```

Save it as etternalauncher.sh or something similar

right click it and make it executable in properties>permissions

### 4: Configuration

Install songs in ~/.etterna/Songs/

Install themes in ~/.etterna/Themes/

Install noteskins in ~/.etterna/NoteSkins/

Preferences are in ~/.etterna/Save/Preferences.ini

Profiles are in ~/.etterna/Save/LocalProfiles/

### 5: Updating

When you want to update your copy of SM5:

cd into the etterna folder you cloned(usually ~/etterna), and run a git pull in terminal:

```
git checkout master
git pull origin master
```

Replacing master(twice) for the git branch you want to update to. Then build again(As instructed above).

---

## Mac
### 1: Download

First we download/clone the repository from github:

``` bash
git clone https://github.com/etternagame/etterna.git
cd etterna
git submodule update --init
```

### 2: Libraries

First, if you don't have [Homebrew](https://brew.sh/) you can install it with the following command
```bash
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

```
brew install openssl nasm zlib
brew install --HEAD libuv
```

### 3: Final configuration

We finish configuring the project:

```
mkdir build && cd build
cmake -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -G "Xcode" ..
```

### 4: Building

Finally, we build the project:

```bash
xcodebuild ARCHS="x86_64" -project Etterna.xcodeproj -target Etterna -destination 'platform=OS X,arch=x86_64' -xcconfig ../Xcode/conf.cnf
```
