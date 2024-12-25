<p align="center">
    <img src="Docs/images/etterna-logo-dark.svg" width=250px>
</p>

<p align=center>
    <a href="https://github.com/etternagame/etterna/actions"><img src="https://github.com/etternagame/etterna/workflows/Etterna%20CI/badge.svg"/></a>
    <a href="https://scan.coverity.com/projects/etternagame-etterna"><img src="https://img.shields.io/coverity/scan/12978.svg"/></a>
    <a href="https://github.com/etternagame/etterna/releases"><img src="https://img.shields.io/github/downloads/etternagame/etterna/total.svg?label=total%20downloads"/></a>
    <a href="https://github.com/etternagame/etterna/releases"><img src="https://img.shields.io/github/downloads/etternagame/etterna/latest/total.svg?label=latest%20downloads"/></a>
    <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-blue.svg?label=license"/></a>
</p>

<p align="center">
    <a href="https://discord.gg/etternaonline"><img src="https://img.shields.io/discord/339597420239519755.svg?color=7289DA&label=Etterna%20Community&logo=Discord"/></a>
    <a href="https://discord.gg/ZqpUjsJ"><img src="https://img.shields.io/discord/261758887152058368.svg?color=7289DA&label=Etterna%20Dev%20Group&logo=Discord"/></a>
</p>

Etterna is a cross-platform rhythm game similar to [Dance Dance Revolution](https://en.wikipedia.org/wiki/Dance_Dance_Revolution). It started as a fork of [StepMania 5](https://github.com/stepmania/stepmania) (v5.0.12), with a focus on keyboard players. Over time, Etterna evolved into its own game, with in-game multiplayer, the online scoreboard [Etterna Online](https://etternaonline.com/), and a community of over 4,000 players.

## Table of Contents

- [Installing](#Installing)
  - [Windows and macOS](#Windows-and-macOS)
  - [Linux](#Linux)
- [Building](#Building)
- [Documentation](#Documentation)
- [Bug Reporting](#Bug-Reporting)
- [Contributing](#Contributing)
- [License](#License)
- [Special Thanks](#Special-Thanks)

## Installing

### Windows, macOS, and Linux

Head to the [GitHub Releases](https://github.com/etternagame/etterna/releases) page, and download the relevant file for your operating system.

For Windows, run the installer, and you should be ready to go.

For macOS, first follow the below instructions. *After* doing them, mount the DMG and copy the Etterna folder to a location of your choice. Run the executable, and you are ready to go.

For Linux, there should be no extra steps. If it does not work, try to follow the build instructions to install the necessary dependencies.

### macOS

This macOS binary is not signed, so before it can be installed it must be de-quarantined by executing this command in the same directory (likely your downloads folder) as the Etterna dmg.

`xattr -d com.apple.quarantine ./Etterna*.dmg`

## Building

All details related to building are in the [Building.md](Docs/Building.md) file. Since Etterna is cross-platform, you should be able to build on any recent version of Windows, macOS, or Linux.

## Documentation

Etterna uses Doxygen and LuaDoc. Both still need a lot of work before being having decent coverage, though we still have them hosted at the following links.  

Any commit to the develop branch immediately updates these documentation sites which can be found [here](https://docs.etterna.dev/).

- Latest C++ documentation: [https://docs.etterna.dev/doxygen/html/index.html](https://docs.etterna.dev/doxygen/html/index.html)
- Latest Lua documentation: [https://docs.etterna.dev/ldoc/index.html](https://docs.etterna.dev/ldoc/index.html)

## Bug Reporting

We use GitHub's [issue tracker](https://github.com/etternagame/etterna/issues) for all faults found in the game. If you would like to report a bug, please click the `Issues` tab at the top of this page, and use the `Bug report` template.

## Contributing

If you want to contribute to the Etterna client, please read [Building](Docs/Building.md) for instructions on how to get started. We have a variety of different tasks which would help the development of this game as a whole, all of which we have listed at [Contributing.md](Docs/Contributing.md). if you are more interested in helping with the in-game multiplayer, the nodejs server, along with its documentation, is hosted [here](https://github.com/etternagame/NodeMultiEtt). You will still need the Etterna client built and running on your system.

If there is something else you want to work on that we don't have listed here, feel free to join [Etterna Dev Group](https://discord.gg/ZqpUjsJ), our development discord, and let us know what you want to add. The developers and contributors there would be able to give you a hand as to where you could start doing what you want to do.  

## License

Etterna uses the MIT License, as is required since we are derivative of StepMania 5. See [LICENSE](LICENSE) for more details.

In short, you are free to modify, sell, distribute, and sublicense this project. We ask that you include a reference to this GitHub repository in your derivative, and do not hold us liable when something breaks.

Etterna uses the [MAD library](http://www.underbit.com/products/mad/) and [FFMPEG codecs](https://www.ffmpeg.org/). Those libraries, when built, use the [GPL license](http://www.gnu.org).

## Special Thanks

- All original SM devs/contributors
- [Jet Brains](https://www.jetbrains.com/?from=Etterna) for giving us free licenses
- [Coverity](https://scan.coverity.com/) for giving us free scans
