# Release Changelog


## [0.68.1] - 2020-02-19 - Hotfix

### Removed
- Til Death P2 Evaluation ComboGraph (unused) - [8a77c73](../../../commit/8a77c73361b5bc874cf5a716af2a35dee193f0eb)
- Til Death Evaluation GradeDisplays (unused) - [45f3051](../../../commit/45f3051194cc7512ae8904fd2311c80167ff0b42)

### Fixed
- Evaluation had major discrepancies when using SSRNorm and different Judge windows - [9b7e769](../../../commit/9b7e769510d261308f8ce3cfcef4b8d59b55eea0) [891b74e](../../../commit/891b74e999cff42f32650efbb02421b8cbf0c46d)
- Grades were not consistently named across languages causing weirdness - [83a08c6](../../../commit/83a08c60b41b4efefefc50198b233b23d82884ce)
- Personal Best shown on the General Tab was incorrect - [2733031](../../../commit/2733031a324848bc4724c91644261ea6737e1012)
- PlayerStageStats had some references to Grades that were newly incorrect - [664ffaa](../../../commit/664ffaac4c52f21f869fda7fda9ddd440f43da76)
- Scores in lists did not sort by SSRNorm when using SSRNormSort - [1e283ae](../../../commit/1e283aeeac9615c769808501ea12088358183779)
- Win32 GraphicsWindow could set invalid display settings overall - [#705](../../../pull/705)
