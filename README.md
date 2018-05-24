Etterna
=========

Etterna is an advanced cross-platform rhythm game focused on keyboard play.

|       Mac         |       Linux-clang |       Linux-gcc   | Windows 7         | Windows 10        | Coverity          |
|-------------------|-------------------|-------------------|-------------------|-------------------|-------------------|
| [![build1-trav][]][build-link-travis] | [![build2-trav][]][build-link-travis] | [![build3-trav][]][build-link-travis] | [![build1-app][]][build-link-app] | [![build2-app][]][build-link-app] | [![build1-cov][]][build-link-cover] |

[build1-trav]: https://travis-matrix-badges.herokuapp.com/repos/etternagame/etterna/branches/develop/1
[build2-trav]: https://travis-matrix-badges.herokuapp.com/repos/etternagame/etterna/branches/develop/2
[build3-trav]: https://travis-matrix-badges.herokuapp.com/repos/etternagame/etterna/branches/develop/4
[build1-app]: https://appveyor-matrix-badges.herokuapp.com/repos/Nickito12/etterna/branch/develop/1
[build2-app]: https://appveyor-matrix-badges.herokuapp.com/repos/Nickito12/etterna/branch/develop/2
[build1-cov]: https://img.shields.io/coverity/scan/12978.svg
[build-link-travis]: https://travis-ci.org/etternagame/etterna
[build-link-app]: https://ci.appveyor.com/project/Nickito12/etterna
[build-link-cover]: https://scan.coverity.com/projects/etternagame-etterna

## Installation
### From Packages

For those that do not wish to compile the game on their own and use a binary right away, be aware of the following issues:

* Windows users are expected to have installed the [Microsoft Visual C++ x86 Redistributable for Visual Studio 2015](http://www.microsoft.com/en-us/download/details.aspx?id=48145) prior to running the game. For those on a 64-bit operating system, grab the x64 redistributable as well. [DirectX End-User Runtimes (June 2010)](http://www.microsoft.com/en-us/download/details.aspx?id=8109) is also required. Windows 7 is the minimum supported version.
* macOS users need to have macOS 10.6.8 or higher to run StepMania.
* Linux users should receive all they need from the package manager of their choice.

### From Source
https://etternagame.github.io/wiki/Building-Etterna.html


## Resources

* Website: https://etternaonline.com/
* Discord: https://discord.gg/ZqpUjsJ
* Lua for SM5: https://dguzek.github.io/Lua-For-SM5/
* Lua API Documentation can be found in the Docs folder.
* ETTP docs: https://github.com/Nickito12/NodeMultiEtt/blob/master/README.md

## Licensing Terms

In short- you can do anything you like with the game (including sell products made with it), provided you *do not*:

1. Sell the game *with the included songs*
2. Claim to have created the engine yourself or remove the credits

For specific information/legalese:

* All of the our source code is under the [MIT license](http://opensource.org/licenses/MIT).
* Any songs that are included within this repository are under the [Creative Commons license](https://creativecommons.org/).
* The [MAD library](http://www.underbit.com/products/mad/) and [FFmpeg codecs](https://www.ffmpeg.org/) when built with our code use the [GPL license](http://www.gnu.org).
