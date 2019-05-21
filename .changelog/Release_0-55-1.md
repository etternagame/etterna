# Release Changelog


## [0.55.1] - 2017-09-18 - Hotfix: Things We Didn't Notice

Windows, Mac, and Linux release.

### Added
- Rates up to 3x are available in Player Options - [0ff5102](../../../commit/0ff510201bd2f20a3fff2f17d91790ba1734f73d)
### Changed
- Don't disable Gameplay Sync controls when Autoplay is on - [8b9a92a](../../../commit/8b9a92a2c8c8aaadfe3e5440954d24bb9935925a)
- Doubly verify a Score is obtained using Chord Cohesion off by counting the hit Taps - [2208f4e](../../../commit/2208f4e5b1155e935c0cdce47853e856de1ac5d3)
- Fallback to basic old MusicWheel sorting for broken sorting methods - [1b78562](../../../commit/1b7856217680e9257d24ed72ae2374f4caf95aae)
- Keep track of PBs, specifically PBs which are Chord Cohesion Off - [49c669f](../../../commit/49c669f3853ff6daf487bbee82ec132b2783b56c) [c7addfc](../../../commit/c7addfc4e62ba2b4897eccbf812db4e1cf39ec12)
- NoteData must exist when writing SSC files for sync changes - [286645e](../../../commit/286645e7ebb3dbaf739a62e7f928eaa7d9d01bf3)
- Set the video troubleshooting URL to a real informational Stepmania video instead of Google.com - [acd1ceb](../../../commit/acd1ceb443dbc4cbe202a0109e345179a11d89d1)
### Deprecated
- Deprecating GAME:CountNotesSeparately() in Lua in favor of GAMESTATE:CountNotesSeparately() - [7585ee8](../../../commit/7585ee82ef48c8cd0b115f388bae442eea21a90a)
### Fixed
- Changing offset messed with MSD calculations; fix by normalizing the first row to time 0 - [8d0454f](../../../commit/8d0454f6bb7254cff0acd27e48a367450874fe70)
- Crashed when loading a playlist with no charts - [4279a10](../../../commit/4279a1077e82497af62cefb2307626831361f2bd)
- F9-F12 didn't work for syncing due to a logic error - [263b797](../../../commit/263b797e9fd09548a35095b81227508fbab1aab7)
- Mines counted in the NotesPerSecond counter - [42207be](../../../commit/42207bec8878168092a1caaf3e54d942a5eb9f4b)
