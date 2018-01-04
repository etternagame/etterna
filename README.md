Etterna
=========

Etterna is an advanced cross-platform rhythm game focused on keyboard play.

[![Travis CI Build Status](https://travis-ci.org/etternagame/etterna.svg?branch=master)](https://travis-ci.org/etternagame/etterna)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/lgdsvx101i45d71k?svg=true)](https://ci.appveyor.com/project/martensm/etterna)
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/12978.svg)](https://scan.coverity.com/projects/etternagame-etterna)

## Installation
### From Packages

For those that do not wish to compile the game on their own and use a binary right away, be aware of the following issues:

* Windows users are expected to have installed the [Microsoft Visual C++ x86 Redistributable for Visual Studio 2015](http://www.microsoft.com/en-us/download/details.aspx?id=48145) prior to running the game. For those on a 64-bit operating system, grab the x64 redistributable as well. [DirectX End-User Runtimes (June 2010)](http://www.microsoft.com/en-us/download/details.aspx?id=8109) is also required. Windows 7 is the minimum supported version.
* macOS users need to have macOS 10.6.8 or higher to run StepMania.
* Linux users should receive all they need from the package manager of their choice.

### From Source

Remember to do
```git module update --init```
Before building

#### Install CMake


First CMake needs to be installed on your system.

The common way of installing CMake is to go to [CMake's download page](http://www.cmake.org/download/). At this time of writing, the latest versions are 3.3.0-rc3 and 3.2.3. Either version will work: the minimum version supported at this time is 3.1.

If this approach is used, consider using the binary distributions. Most should also provide a friendly GUI interface.


#### CMake Command Line


If you are unfamiliar with cmake, first run `cmake --help`. This will present a list of options and generators.
The generators are used for setting up your project.

Once in the root directory for StepMania create a directory named Build and change into it.

For the first setup, you will want to run this command:

`cmake -G {YourGeneratorHere} .. && cmake ..`

Replace {YourGeneratorHere} with one of the generator choices from `cmake --help`. As an example, macOS users that want to have Xcode used would run `cmake -G "Xcode" .. && cmake ..` in their Terminal program.

On Linux the basic command used would be `cmake -G "Unix Makefiles" .. && cmake .. && make -j4`.

If you are building on Windows and expecting your final executable to be able to run on Windows XP, append an additional parameter `-T "v140_xp"` (or `-T "v120_xp"`, depending on which version of Visual Studio you have installed) to your command line.

If any cmake project file changes, you can just run `cmake .. && cmake ..` to get up to date.
If this by itself doesn't work, you may have to clean the cmake cache.
Use `rm -rf CMakeCache.txt CMakeScripts/ CMakeFiles/ cmake_install.txt` to do that, and then run the generator command again as specified above.

The reason for running cmake at least twice is to make sure that all of the variables get set up appropriately.

Environment variables can be modified at this stage. If you want to pass `-ggdb` or any other flag that is not set up by default,
utilize `CXXFLAGS` or any appropriate variable.

#### Release vs Debug


If you are generating makefiles with cmake, you will also need to specify your build type.
Most users will want to use `RELEASE` while some developers may want to use `DEBUG`.

When generating your cmake files for the first time (or after any cache delete),
pass in `-DCMAKE_BUILD_TYPE=Debug` for a debug build. We have `RelWithDbgInfo` and `MinSizeRel` builds available as well.

It is advised to clean your cmake cache if you switch build types.

Note that if you use an IDE like Visual Studio or Xcode, you do not need to worry about setting the build type.
You can edit the build type directly in the IDE.




## Resources

* Website: TBA
* Discord: discord.gg/ZqpUjsJ
* Lua for SM5: https://dguzek.github.io/Lua-For-SM5/
* Lua API Documentation can be found in the Docs folder.

## Licensing Terms

In short- you can do anything you like with the game (including sell products made with it), provided you *do not*:

1. Sell the game *with the included songs*
2. Claim to have created the engine yourself or remove the credits
3. Not provide source code for any build which differs from any official release which includes MP3 support.

For specific information/legalese:

* All of the our source code is under the [MIT license](http://opensource.org/licenses/MIT).
* Any songs that are included within this repository are under the [Creative Commons license](https://creativecommons.org/).
* The [FFmpeg codecs](https://www.ffmpeg.org/) when built with our code use the [LGPL license](http://www.gnu.org).
