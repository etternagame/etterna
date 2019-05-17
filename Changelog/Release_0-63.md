# Release Changelog


## [0.63.0] - 2018-11-23 - Performance Boost

Windows x64 and Windows i386 installer release. Mac zip included.

### Added
- Main Skillset for a Score in the Scores tab - [1dab361](../../../commit/1dab3619c56a5a925738562483d2d8873951bdeb)
- Replay Watching for ReplayData made from 0.60 forward hits Taps using offset values - [2003f69](../../../commit/2003f6936e230d684511159046ca6dc9ae1b11d6) [f63ea82](../../../commit/f63ea82f1aa6a6975ec8b57d1874396d0d8b750a) [a6563b0](../../../commit/a6563b08049c208f3c79a0668964d8d67d37ae96) [5f18c63](../../../commit/5f18c630b12b8dcad605bb5321316f9d74d753da) [9181723](../../../commit/91817235e88287693c54147877a4824dd339cf49) [2efb3fa](../../../commit/2efb3fac3d7369529d58722382c197b10278ec85) [ae000d7](../../../commit/ae000d780f5979f7ea2abbf8b4e9d16cebe5f30e) [3736163](../../../commit/373616359d38dcebb6d27efbffca3779aba6ab5a)
### Changed
- Banners don't load if the Banner isn't visible - [20cd6fb](../../../commit/20cd6fb333ab358cffa93aeade8daba1982d7cc1)
- Banners don't load if moving the MusicWheel fast - [92e5359](../../../commit/92e53596f189ae7988b90d97368917a6148a409e)
- Banners moved to the General tab Lua - [e0054b8](../../../commit/e0054b8ed9a2f11d8e0e613dfbf1093b2dacd75e)
- Banner-specific updating logic in Lua is reworked to try to reduce memory leaks - [a189b74](../../../commit/a189b74db87c1f6446b5904d5295856220189a90)
- Clear cached Chart Leaderboard data after Gameplay - [fc3c4e7](../../../commit/fc3c4e79e5020d004d3a8ae29ee458b161d328eb)
- Don't update DownloadManager in Gameplay - [01710d7](../../../commit/01710d71bb7467b1bd4f158207aa5307ed8476da)
- Garbage Collect from Lua (this should be called at least once somewhere) - [0bdd05f](../../../commit/0bdd05fb7570b066bcc392e1b7acd61c9855e3e4)
- General tab Lua refactored for speed (this reaches all SelectMusic Lua) - [cce801a](../../../commit/cce801a9981c0b40049d00064138623aca4737b6) [e0054b8](../../../commit/e0054b8ed9a2f11d8e0e613dfbf1093b2dacd75e) [0bcf406](../../../commit/0bcf406553347843b455e14d6db16f0c5b40e44d) [d793ad6](../../../commit/d793ad6356f093c1404cc4727a690b22ebe4dca3) [92fb0e1](../../../commit/92fb0e170da00820172034e4834e8f6556a946cd) [4ac9453](../../../commit/4ac9453051d4953b707e6ff464ac4e8a9bd21ef4) [cfc8edc](../../../commit/cfc8edc28b0e4cd9b8ec70410c5ecf02c791ca9e)
- Move Lua functions to get Display Scores (PBs by rate) to Scripts - [5c9b126](../../../commit/5c9b126645e7108073f53eab10399cf4dbf3c9c8)
- Refactor highlight updates for Actors in ScreenSelectMusic - [c37d492](../../../commit/c37d492557c9c30255804c131106df633c455814) [0bcf406](../../../commit/0bcf406553347843b455e14d6db16f0c5b40e44d)
- Restrict max number of difficulties in a Chart to 12 - [c37d492](../../../commit/c37d492557c9c30255804c131106df633c455814)
- Screenshots on the Evaluation Screen can be taken by pressing `Select` & don't save as low resolution in a weird folder - [508f50b](../../../commit/508f50b56f9579e3543703ffa5d8fe9143ebaa41)
- Stop trying to store Online ReplayData directly from Chart Leaderboard Requests - [d4eda50](../../../commit/d4eda50e337490de69b8687c03d2200dc874389e)
### Removed
- 2 Part Selection, a 2 Player feature - [731289c](../../../commit/731289cc5e01f1c18f7f6d5760fa9822b9a0b23b)
- Unused or unimportant internal ScreenSelectMusic Actors - [e01ab8a](../../../commit/e01ab8af140fd654fd15bcd9f5c276588ab4e912)
### Fixed
- Chart Leaderboards were being requested too much due to inherited Actor Commands - [2cdb5b5](../../../commit/2cdb5b5de93286d06ae34b65917ffb5ac81b6a5d)
- Chart Preview was placed badly in 4:3 - [1f54adf](../../../commit/1f54adf992441c8023d8ff0cea1c0012485caab9)
- Coverity Scan found some issues
  - Null pointers - [cc09585](../../../commit/cc0958578883cb5284a2770125563261200f0753) [a6f9269](../../../commit/a6f92693269f9fe2ae29daf058af7af776687c51) [a269a3b](../../../commit/a269a3bcba84cc4a3fb59e163bcbb594c244bb80)
- Lua return values crashed everything but 32bit Windows - [b93f093](../../../commit/b93f09380de951532f0ced05751f13c3a121c802) [fb5ca86](../../../commit/fb5ca860f154b66a8e7f3292111d3e37939343b4)
