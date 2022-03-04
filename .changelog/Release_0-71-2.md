# Release Changelog


## [0.71.2] - 2022-??-?? - Hotfix

### Added
- Hold explosions added back to SubtractByZero to match DivideByZero and MultiplyByZero - [0a76556](../../../commit/0a765560edcaec4dc38b0f73a59e381e1bda92f1)
- Rebirth customize gameplay now allows changing the judgment counter size and spacing - [4089658](../../../commit/40896582bad30257ef20137853e9fd4eff6e6044) [59b20e4](../../../commit/59b20e40d72e170ccef16a09ce89740279e90fe6)

### Changed
- Moved nowplaying.txt generation to fallback scripts to let other themes do it in a standard way - [c07ed90](../../../commit/c07ed905afad8444c7644bb3c670ec1b3a4c9007)
- Late miss window now properly 180ms minimum - [5b87dec](../../../commit/5b87decb380ded3f9d4e0972b3365d6c6a449a01)
- Rebirth help updated to mention flams as an alternate name to graces - [6c694d3](../../../commit/6c694d317305dcd6c49b40b0ad28cd69d3db2dff)

### Removed
- MaxInputLatencySeconds Preference (this had no effect on anything in the game) - [5b87dec](../../../commit/5b87decb380ded3f9d4e0972b3365d6c6a449a01)

### Fixed
- Beat default noteskin alignment issues - [d8b2c55](../../../commit/d8b2c556b3918f6016e755ee9da3eac30dc38d41)
- Linux related build issues for debug purposes - [2bd8b0f](../../../commit/2bd8b0f2766d8661a5a909e473cd732a72113bb1)
- MacOS <= 10.13 could not open the game or use input due to translocation API utilization - [40c2ecd](../../../commit/40c2ecd5d5b6473e1296fb76ac7f04068a349b1f)
- Rebirth artist song sorting was in alphabetical order of title rather than artist - [ba89bba](../../../commit/ba89bba4486274e955b37324b04f66ad2a970453)
- Rebirth Discord Rich Presence was not working and now it is. Standardized the functions used and moved them to fallback scripts - [e01aea9](../../../commit/e01aea95982001776afdf6281bfa94a429b0526b)
- Rebirth nowplaying.txt was not emptying after Gameplay - [bbf1f1b](../../../commit/bbf1f1bae8c82d7f8f9fbf6b1f3edd0faf48e8c7)
- Rebirth song search metadata tag search was case sensitive (did not allow aUTHoR=) - [78fac46](../../../commit/78fac4636abd4b9289d6b3c6a3cd63aed31dc15f)