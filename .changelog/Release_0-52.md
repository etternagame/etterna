# Release Changelog


## [0.52.0] - 2016-12-14 - Song Favoriting

Windows installer release.

### Added
- D3D Render Target support - [a367c05](../../../commit/a367c05eb67b3975c7bcd8813fd79740b72a6ae8) [577845e](../../../commit/577845efca1bf9b0d4d4409e0852cc0b3f3b71cd) [ed45d5b](../../../commit/ed45d5b439f81822863df53f66c7e19604db84b4) [f1cd28b](../../../commit/f1cd28b5c09e0b42864bbdcbca2ac710f33982c6) [0b7e06d](../../../commit/0b7e06dbbecb6e1bc1744bfadc8c0c17959bf2c1) [b3efd1d](../../../commit/b3efd1d0778706f3c0946720c7c74083a51c2a9d)
- Judge Conversion & Chartkey History - [80c7ceb](../../../commit/80c7cebd529cfd502d78f6a182b7991f5ebe5bda)
- Judgment Cutoff Lines on Offset Plots - [39e5fa8](../../../commit/39e5fa8a3e5ea6a00af686fa2233143e46398efb)
- Song Favoriting - [7f6f053](../../../commit/7f6f05337ca42f8874e5397631eb64b79714bf8a) [717e175](../../../commit/717e175a40bc365f415bd12dce2703c0e679ef9f)
- Song Search can now search by Artist & Author - [4599f9f](../../../commit/4599f9f6032e9937a91d11403750558af1f6a66c)
- SongUtil function for getting Simfile Authors - [43d3392](../../../commit/43d3392d86f56be2e10957872645cd0c47d3e7d1)
- Sorting by Favorites - [428bf72](../../../commit/428bf7222341e56418b2dab3534292c073006d43)
- Target Tracker can be set to Personal Bests & Target Percents - [cd890f5](../../../commit/cd890f57a090fbe8321a34860cf076f60886e541)
### Changed
- ArrowVortex is mentioned ingame as a better editor in the Edit menus - [149945c](../../../commit/149945c77b5ca89e0c49c54ee79b95c8508b39a2)
- Clean up DivideByZero Noteskin Rolls - [d9bbd95](../../../commit/d9bbd9523dde2910efec5ebf121cd307030e72c2)
- Don't reset search strings when entering Song Search. Press Delete instead. - [8b1acaf](../../../commit/8b1acafee7aeb62a13895e63871e581bdc1c510e)
- FailOff/FailAtEnd will not produce SSRs - [8737a14](../../../commit/8737a1442ded55ff76d220c17b943fdb1f3aa852)
- Improve text character rendering - [e2041b0](../../../commit/E2041B0478d766c2d7fed4ce08c5213bee56b990) [a04b891](../../../commit/a04b891677012fa412296641f5155cc89e9f5977) [35d7c53](../../../commit/35d7c5336722c853825d3bb055106bb577d8f736)
- Moved Wife2 function to RageUtil from Player - [868f329](../../../commit/868f3291638d394ae457679f5e41e900ff45a067)
- More typecasts change to static casts - [75354e8](../../../commit/75354e8581acc57f6583e5922c37708930063d1b)
- MSD Calc updates. - [01d58ad](../../../commit/01d58ad0c7616186e249ef9a1e9e37251e280be3)
- Music display shows file length, not audio length - [b696378](../../../commit/b69637866178d928d7ef9b7cd8c8892708a90c40)
### Removed
- Writing old Groove Radar data for HighScores - [e04f7e0](../../../commit/e04f7e0d5cb0f257b5f7099c2b22d840799c51af)
### Fixed
- DWI files crashed on first load - [1b2294c](../../../commit/1b2294cb4cfdaca3e6be169ed58f9405e9fad32b)
- Edit Mode crashed immediately on playback - [f1e4b28](../../../commit/f1e4b280739a27cb0af624a3f088af64a481d2ee)
- Linux could not compile due to some type issues - [eb76405](../../../commit/eb7640534de58d4226d1e4cc1b8eb4cb2059a21c)
- Wife Percent in various locations had bad decimal placement - [f29df05](../../../commit/f29df050bf4b3cd02442f5a21ad30b284679542b) [d45e482](../../../commit/D45E482924f6d81a558e764eabaad358d0d61ab0)