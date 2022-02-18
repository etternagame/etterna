# Release Changelog


## [0.71.2] - 2022-??-?? - Hotfix

### Added
- Hold explosions added back to SubtractByZero to match DivideByZero and MultiplyByZero
- Rebirth customize gameplay now allows changing the judgment counter size and spacing

### Changed
- Moved nowplaying.txt generation to fallback scripts to let other themes do it in a standard way
- Rebirth help updated to mention flams as an alternate name to graces

### Removed
- MaxInputLatencySeconds Preference (this had no effect on anything in the game)

### Fixed
- Linux related build issues for debug purposes
- MacOS <= 10.13 could not open the game or use input due to translocation API utilization
- Rebirth artist song sorting was in alphabetical order of title rather than artist
- Rebirth Discord Rich Presence was not working and now it is. Standardized the functions used and moved them to fallback scripts
- Rebirth nowplaying.txt was not emptying after Gameplay
- Rebirth song search metadata tag search was case sensitive (did not allow aUTHoR=)