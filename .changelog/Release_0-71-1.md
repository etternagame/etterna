# Release Changelog


## [0.71.1] - 2021-12-12 - Hotfix

### Added
- Rebirth Color Config options for Gameplay Text and Element BG - [e5b1d68](../../../commit/e5b1d688b22563c7a20cbac87fc92ece54c54f91)
- Rebirth Color Config options for the text that goes on Judgment Bars - [6adbb03](../../../commit/6adbb0334a495fa1edf340e9d5ed6b8b4dbd57f6)
- Rebirth Color Config and Theme Option added for setting a Single BG Color in SelectMusic - [04c37f2](../../../commit/04c37f21909487e6098f7fa8297a28952b39d089)
- Rebirth MeasureLines toggle option added to settings - [09eaf6c](../../../commit/09eaf6c0441c8c41d174ba6f102711e7586aef3a)
- Theme util for height resizing - [e1cc49c](../../../commit/e1cc49c9097c3af3cc3adb394b19c0dbbee9ea84)

### Changed
- Rebirth legacy option screen sizes are now scaled similar to older themes - [ac3551b](../../../commit/ac3551b04012308a0778e18498d45cf261fc0b4b)
- Rebirth locks all possible input when selecting a song - [9877c2a](../../../commit/9877c2a2eb1a788809ec76438996f725a224dcf6)
- Rebirth customize gameplay increments for Error Bar increased - [f321f3d](../../../commit/f321f3d3e293d11d679303446ace0c038005c61c)
- Rebirth evaluation text is slightly bigger - [3d0d5b6](../../../commit/3d0d5b685514eaae753faeed00fa60f097bc3f26)
- Rebirth Mirror option moved to Essential Settings - [2127beb](../../../commit/2127beb818d5fcbad7460c561149d26b6056a94d)
- Rebirth permamirror and favorite icons moved off of banners - [192affc](../../../commit/192affcc9960f583c7f8bf6e8a31ffdd52aa962c)
- Rebirth scores tab, local version Grade resolution is twice as big - [84dc562](../../../commit/84dc5625c15d034a48a39bfdbaa6618bf95dff6b)

### Removed
- unavailable_functions lua - [24c8c24](../../../commit/24c8c241bd853283b73a8f9d4db6ef15d990699e)

### Fixed
- Compile warning - [f183bef](../../../commit/f183bef061ff808e5056bec69e4f85825bb80738)
- Default pump skin Lua errors when opening pump on Rebirth - [c8585bb](../../../commit/c8585bbc4be6b2afc152ba509e528aa99f175ac3)
- Multiple instance detection on Windows detected folder names - [041532a](../../../commit/041532a1306ecada11e9a2b7b879cc19f224db8a)
- Rebirth customize gameplay broke Notefield Y position when using Reverse - [20f57ce](../../../commit/20f57ce745c3019c1abda8f8ec2d558bbc8bf26e)
- Rebirth custom resolutions didn't stick if you even touched the settings menu - [6af633e](../../../commit/6af633eaf9629a88c792c28f7137cfc08f845575)
- Rebirth difficulty display ghost cursors if song had >= 5 diffs - [12f55e2](../../../commit/12f55e2ae3a55e49be9c142d90a514f6bbcfe2e7)
- Rebirth sync machine leaked music rate from select music into GameplaySyncMachine - [ac85574](../../../commit/ac855747c96814a4296aa463ea7c3110c10cd583)
- Replay Snapshots did not consider beat Game transforms - [84fc4b3](../../../commit/84fc4b3f2980dc6079517030009c4b5ef094fa59)
- Til Death scores tab ghost scores when leaderboard has less than 10 or 13 scores - [6a2a963](../../../commit/6a2a963470018741a21b12c2da8ac6cc5bb7586f)
- Til Death scores tab unexpected nested tab switching - [b587803](../../../commit/b58780307046e830ee6368e13921a00e00e175e0)
- Til Death tags tab mouse hover logic was broken - [ce63606](../../../commit/ce636066660fd34cbb9487129bdd7515b9848c8c)