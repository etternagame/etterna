# Release Changelog


## [0.72.1] - 2023-01-09 - Hotfix

### Added
- Github Actions automatically produces game documentation for every commit - [1a106a7](../../../commit/1a106a787ad11fb34ddf0e29116f2be663aafbb3) [d90e86e](../../../commit/d90e86ef8358311abd4d2681a0ce7c09b589607d) [a07683c](../../../commit/a07683c31e8f1a2cd563862773895cf090f42a89)
- Holding Shift while pressing the random song button brings you to the previous song you got by pressing random. This only goes backwards 1 song - [8dd3f80](../../../commit/8dd3f80d184f8f5ddb2c84277094b862d62d16ab)
- NoteskinManager has a Lua binding to add a Noteskin element directly to an ActorFrame as a child - [8b43403](../../../commit/8b43403e3a5bb9f304f3608a0fc846900ce56e72)

### Changed
- Cleartypes changed to not grant SDP/SDG if any CB is present - [a12a825](../../../commit/a12a82573bf10ddcdcf26aa1eaae24763a9a7263) [1fd615c](../../../commit/1fd615cc5f6024b1da12a559c5e511d3be752d22)
- Judge difficulties 1-3 are now finally invisible. They haven't functioned for a long time - [d0173f6](../../../commit/d0173f6f32d115293797ad16f3feeebc41342fe2)
- Noteskin Preview optimized for loading. Should greatly reduce screen load times for Rebirth SelectMusic and all PlayerOptions screens - [bfbccfb](../../../commit/bfbccfbec7cffeb0ef1e19032f3953f586cc39ef)
- Updated websocketpp to 0.8.2 - [18419c5](../../../commit/18419c56ba3f4ed6c0d26c300e7762d416b590a8)

### Fixed
- Binding a letter to the Operator key doesn't cause it to be untypeable - [482ac7e](../../../commit/482ac7ea34b76f0587a10ea7d22c58ffe8acaac4)
- Downloading a pack containing filenames or paths not compatible with your current locale used to partially or fully break the extract - [eaaafb9](../../../commit/eaaafb91919029b8824d972a7e45d47eaafca025)
- Files loaded from .osu should no longer crash due to 0 length holds - [fee6baa](../../../commit/fee6baab9b7664186a7291d7a972b49e1b0b4454)
- Legacy Options input debounce option used to be broken and nonfunctional - [f5c7588](../../../commit/f5c75881bc4842a13f42471f550c431dba6b9969)
- Mac users sometimes could not run the game properly for ssh related reasons - [1292209](../../../commit/1292209508c2ae6293ef6e6b354ec7b2ea9714d0)
- Replays used to drop the first notes of playback if they were hit early - [9addca6](../../../commit/9addca67febb570654c7d80ca33cd799ff862aa1)
- Replays used to never delete the decompressed input data after reading it and failing - [a178233](../../../commit/a178233ee83b2f9a49924e6fdb6ec16a185f2be5)
- Replays used to play back very incorrectly if you have multiple charts with the same chartkey and different audio or different offsets - [7572298](../../../commit/7572298349890a69fd4faeb810a331bc21f53919)
- Til' Death combo and judgment animations broke customize gameplay resizing - [b39486e](../../../commit/b39486e1375045529eafe5723864f53804fbef10)
- Websocketpp preventing super modern compilers from compiling due to C++20 standards - [18419c5](../../../commit/18419c56ba3f4ed6c0d26c300e7762d416b590a8)