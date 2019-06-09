# Release Changelog


## [0.66.1] - 2019-06-10 - Hotfix

Windows x64, Windows i386, and Mac installer release.

### Added
- MA/PA ratio indicators on the Evaluation Screen - [#569](../../pull/569)
- More specific information on Generators in the Building documentation - [#565](../../pull/565)
### Changed
- Chord Cohesion indicator text on the Evaluation Screen appears only when it's on - [#569](../../pull/569)
- Pressing Start on the Pack Downloader Screen shouldn't exit it - [18be076](../../commit/18be076d7698e054a1ebacab81c62b87ab467c42)
- Pressing Start on the Bundle Select Screen shouldn't exit it - [18be076](../../commit/18be076d7698e054a1ebacab81c62b87ab467c42)
- Random BG Preference is defaulted to Off since this apparently works now - [c4acf2a](../../commit/c4acf2a52edf2841ceb26f424e5959282289feb0)
- Random Song Pickers shouldn't seed every click with OS clock seconds because it produces the same number every time second - [151c76e](../../commit/151c76e6f9dd4a87c2bcf6df9d8c6a075150783b)
- Toasties in the Asset Picker shouldn't play the sound if the item is already selected - [0381772](../../commit/0381772cfa0c0588e21d339dccba03de9ec2e3ce)
### Fixed
- Build order for CMake caused issues for everything but Windows - [#561](../../pull/561)
- DownloadManager had broken logic which made updating Online rankings impossible - [68be106](../../commit/68be106bf14bdebe4b416b5abfe500fce3c3b61c)
- Linux builds failed due to a last minute CMake change - [#559](../../pull/559)
- Multiplayer Evaluation Scoreboard had lua errors due to a removed Actor - [03e3436](../../commit/03e3436e0b74b988b4b1081d193db342ca4bea46)
- Window Icon for the game was gigantic in the Windows Audio Mixer - [d740f49](../../commit/d740f499ea444943d58fbf5802639ead0da02379)
