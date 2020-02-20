# Release Changelog


## [0.68.1] - 2020-02-19 - Hotfix

### Added
- Evaluation Lua hooks have access to Replay Modifiers - [71db611](../../../commit/71db6110fa3139256ec70668b354f12b124b2f77)
- Lua has access to moving the game window on Win32 - [4f84089](../../../commit/4f84089e81bc08e6ecd22f179e6c2f78ac4262fb)

### Changed
- Evaluation Scoreboard has scrolling support - [6fbef3d](../../../commit/6fbef3d219c72c9853d654af1052666834523289) [2e78ca8](../../../commit/2e78ca83bd48440c0271e6f87b4000eb36c0bd90)
- Evaluation Scoreboard shows Wife J4 when using SSRNorm for consistency - [50bec4e](../../../commit/50bec4ec86858891e98edea41ed5135867a598bd)

### Removed
- Til Death P2 Evaluation ComboGraph (unused) - [8a77c73](../../../commit/8a77c73361b5bc874cf5a716af2a35dee193f0eb)
- Til Death Evaluation GradeDisplays (unused) - [45f3051](../../../commit/45f3051194cc7512ae8904fd2311c80167ff0b42)

### Fixed
- CustomizeGameplay showed up in ScreenGameplaySyncMachine - [87216ff](../../../commit/87216ffd1b812b910bd172e8a37af9f422a244a5)
- Evaluation had major discrepancies when using SSRNorm and different Judge windows - [9b7e769](../../../commit/9b7e769510d261308f8ce3cfcef4b8d59b55eea0) [891b74e](../../../commit/891b74e999cff42f32650efbb02421b8cbf0c46d)
- Grades were not consistently named across languages causing weirdness - [83a08c6](../../../commit/83a08c60b41b4efefefc50198b233b23d82884ce)
- Help Overlay did not cover up the session timer and system clock - [6a46310](../../../commit/6a46310542c060396ff5578cb8ddbd7a3973c5ec)
- Personal Best shown on the General Tab was incorrect - [2733031](../../../commit/2733031a324848bc4724c91644261ea6737e1012)
- PlayerStageStats had some references to Grades that were newly incorrect - [664ffaa](../../../commit/664ffaac4c52f21f869fda7fda9ddd440f43da76)
- Replay PlayerOptions String used to just be your own mods - [a534176](../../../commit/a53417623eb2a4fd13baacaf7c89099ea4c73fa6)
- Scores in lists did not sort by SSRNorm when using SSRNormSort - [1e283ae](../../../commit/1e283aeeac9615c769808501ea12088358183779)
- Win32 GraphicsWindow could set invalid display settings overall - [#705](../../../pull/705)
