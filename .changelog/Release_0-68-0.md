# Release Changelog


## [0.68.0] - 2020-02-17 - New Color Editor & Borderless Fullscreen

### Added
- AAAAA Grade, Midgrades, and SSRNormPercent prioritization - [#698](../../../pull/698)
- Borderless Fullscreen & better Display Options - [#697](../../../pull/697)
- ScreenColorEdit has been rewritten to allow for very fine control - [#695](../../../pull/695)
- X11 Delete Window handler - [#677](../../../pull/677) [75e7aa5](../../../commit/75e7aa5c13c9ecea1e7bbbc80012a36e68a9bd18)

### Changed
- C exceptions causing crashes should give a more informative crash reason - [#687](../../../pull/687)
- Default Judgment Textures updated to be higher resolution - [#674](../../../pull/674)
- Upgraded Libtomcrypt to 1.18.2 - [#673](../../../pull/673)

### Fixed
- Crashed due to stack overflow when logging giant strings - [#681](../../../pull/681)
- Fallback Lua errors when leaving out certain things - [#684](../../../pull/684)
- Icons in ScreenSelectMaster never see the GainFocusCommand - [4048c28](../../../commit/4048c284dd76c1f0167219caee85758df1ace891)
- Pack Downloader buttons always returned the mirror even if not clicking mirror - [9c00f34](../../../commit/9c00f3415955a390edd4bfd20bbe92a8d7d4a441)
- Playlist creation overlay used to stop music - [7c0ddb9](../../../commit/7c0ddb993a2cbfb72e0a6635e61ebe5b5547596e)
- Resolved around 200 simple coverity defects - [#683](../../../pull/683)
    