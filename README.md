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
* Mac OS X users need to have Mac OS X 10.6.8 or higher to run StepMania.
* Linux users should receive all they need from the package manager of their choice.

### From Source

Etterna can be compiled using [CMake](http://www.cmake.org/). More information about using CMake can be found in both the `Build` directory and CMake's documentation.

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
