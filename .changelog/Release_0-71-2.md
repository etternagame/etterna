# Release Changelog


## [0.71.2] - 2022-03-14 - Hotfix

### Added
- Actor GetTrueXYZ, GetTrueRotationXYZ - [99da45d](../../../commit/99da45d8e57ad421801ed59e3220053f78b237b3)
- Hold explosions added back to SubtractByZero to match DivideByZero and MultiplyByZero - [0a76556](../../../commit/0a765560edcaec4dc38b0f73a59e381e1bda92f1)
- Preference AllowStartToGiveUp to toggle holding Start to fail a song - [7196513](../../../commit/719651388ee54023224b32990946ef4c61b9b917)
- Rebirth customize gameplay now allows changing the judgment counter size and spacing - [4089658](../../../commit/40896582bad30257ef20137853e9fd4eff6e6044) [59b20e4](../../../commit/59b20e40d72e170ccef16a09ce89740279e90fe6)
- Rebirth sortmode added: Tournament to sort songs within groups by their bracket prefix (mostly doesn't change anything) - [8c26152](../../../commit/8c261528f7bf16f009c2570763af488954e2f39d)

### Changed
- Moved nowplaying.txt generation to fallback scripts to let other themes do it in a standard way - [c07ed90](../../../commit/c07ed905afad8444c7644bb3c670ec1b3a4c9007)
- Late miss window now properly 180ms minimum - [5b87dec](../../../commit/5b87decb380ded3f9d4e0972b3365d6c6a449a01)
- Rebirth changeMusicRate function is refactored to allow easier changing of increments - [5db50a3](../../../commit/5db50a32b7e749208443396c52857c00a395ed18)
- Rebirth chart preview position was slightly moved - [7fc7e78](../../../commit/7fc7e788441a44a3dc77d295aa1e956f3f3aa880)
- Rebirth chart preview can always be accessed from clicking the banner if on the General tab - [96eb1a1](../../../commit/96eb1a17c34e7745063a48d260e3f2c9671b4bb7)
- Rebirth Error Bar Count option cap raised to 200 - [263326f](../../../commit/263326fd55a4c4389357351126ef951bbc447aec)
- Rebirth help updated to mention flams as an alternate name to graces - [6c694d3](../../../commit/6c694d317305dcd6c49b40b0ad28cd69d3db2dff)

### Removed
- MaxInputLatencySeconds Preference (this had no effect on anything in the game) - [5b87dec](../../../commit/5b87decb380ded3f9d4e0972b3365d6c6a449a01)
- Practice mode no longer allows access to the Evaluation screen - [3b10c20](../../../commit/3b10c20e34df5c71fc8d706af12bb142ff749dec)
- rootfs mounting - [ba56b43](../../../commit/ba56b43abf85d40001a5788a6a0e3715b1ba4e54)

### Fixed
- Beat default noteskin alignment issues - [d8b2c55](../../../commit/d8b2c556b3918f6016e755ee9da3eac30dc38d41)
- Input Debounce was completely broken - [1a93c5c](../../../commit/1a93c5c2ac01a86a50695f1a05d51b2f5650c7c8)
- Linux related build issues for debug purposes - [2bd8b0f](../../../commit/2bd8b0f2766d8661a5a909e473cd732a72113bb1)
- MacOS <= 10.13 could not open the game or use input due to translocation API utilization - [40c2ecd](../../../commit/40c2ecd5d5b6473e1296fb76ac7f04068a349b1f)
- Rebirth artist song sorting was in alphabetical order of title rather than artist - [ba89bba](../../../commit/ba89bba4486274e955b37324b04f66ad2a970453)
- Rebirth banner was not the expected 3.2 aspect ratio - [2620040](../../../commit/2620040d70fbcdf33e411ddb54bd47e970ac107d)
- Rebirth clicking main menu buttons with left and right click very quickly crashed - [651b6cb](../../../commit/651b6cbe9f77bf4c6748e374904ec1c1b678703e)
- Rebirth Discord Rich Presence was not working and now it is. Standardized the functions used and moved them to fallback scripts - [e01aea9](../../../commit/e01aea95982001776afdf6281bfa94a429b0526b)
- Rebirth gameplay leaderboard caused all elements to not load if the chart was unranked - [a9a8335](../../../commit/a9a83352a7a9cfdd5902c406bedd09c34ad973f6)
- Rebirth nowplaying.txt was not emptying after Gameplay - [bbf1f1b](../../../commit/bbf1f1bae8c82d7f8f9fbf6b1f3edd0faf48e8c7)
- Rebirth PlayerInfo Gameplay element positioning was wrong when not in 16:9 - [b6cec67](../../../commit/b6cec67a9825173c52b111ef839611756bd3ad17)
- Rebirth settings forced custom aspect ratios to be overwritten by a "sane" one - [833c33c](../../../commit/833c33c5168186c84a3d2dd58058180e58a47323)
- Rebirth song search metadata tag search was case sensitive (did not allow aUTHoR=) - [78fac46](../../../commit/78fac4636abd4b9289d6b3c6a3cd63aed31dc15f)
- Rebirth tag filtering caused inconsistent overfiltering and underfiltering - [aff7942](../../../commit/aff794265f41066faa9607a493ee1d28bac8a183)
- Til Death's PlayerOptions XMod scroll speed display was not correct for rates - [27aee32](../../../commit/27aee322c4ebbe675a04268d7b496e38fa630bde)