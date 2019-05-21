# Release Changelog


## [0.57.1] - 2017-11-15 - Last Big MSD Update & Bug Fixes

Windows installer release. Due to merging, some features of this version and the previous one may be confused.

### Added
- Allow ScoreManager to purge scores - [8da6004](../../../commit/8da60041ea588dcda204bf65877c9c2d47a0242a)
- Allow pasting from the clipboard in various places - [f0c1bc0](../../../commit/f0c1bc0474bd035a25dfa753b8d8d444d9651725)
- Hilarious splash screen - [19ea4af](../../../commit/19ea4af11c54b0498947b568e95132c1a435a35d)
### Changed
- Discord connection properly closes when closing the game - [31e0210](../../../commit/31e0210af83f913d841304219032d2d9ec4c70cf)
- Don't care about input debounce for the mousewheel - [c5a97e3](../../../commit/c5a97e3d2c209c0bbd12fd52e302992af669031d)
- Don't let Lua mess with internal rescoring functions - [2fc75f9](../../../commit/2fc75f90f990e66e162ae6d6c772ca2c4d913866)
- Drop the lowest skillset when calculating Player Overall - [063f5e0](../../../commit/063f5e0224fa4d97bb8d9027ccda6230fe3a6ce8)
- Initialize MessageManager earlier in the creation process due to undefined behavior - [dc484e6](../../../commit/dc484e68bc792821b00c913a0a427e63522f21c6)
- JackStamina shouldn't be ignored in Player Overall Calculation - [2201b59](../../../commit/2201b5928f6f2b81288c9f42200efbb72deef6f4)
- JackStamina has been replaced with Chordjacks - [3065bb5](../../../commit/3065bb579eaa189f3b403d80734dfb94eb59711c) [d29f7a9](../../../commit/d29f7a93563735cad9cbb363a5ceb7cfb3e8b840) [b80925b](../../../commit/b80925b06070eb894abbcf4131fe59b8c6b39148)
- Multiplayer improvements to input redirection & room searching - [0b00b5a](../../../commit/0b00b5a64bd8813a895182830708a1bba24bcdd7) [505ab41](../../../commit/505ab4121293d0266886178b2ad36f286096c63e)
- Replays after Chord Cohesion changes go into a different directory - [02d839f](../../../commit/02d839f1acbac4f15bdd3cc21786eacde47b88ad)
### Removed
- Levels - [0b00b5a](../../../commit/0b00b5a64bd8813a895182830708a1bba24bcdd7)
- Unused theme file - [55935e7](../../../commit/55935e7c057290c4f50f857758e07a771ca8c5b6)
### Fixed
- 180ms window cap didn't work on Replay-Loaded plots - [903375c](../../../commit/903375c33e3bf714de01c730515551adebf381d9) [2349890](../../../commit/234989037e2b81bd168f6009d4196612bc6228b3)
- Loading ReplayData failed and caused crashes - [88f2262](../../../commit/88f2262b7bcd33069618bdf9d46091b67540d890)
- RoomWheel was broken - [6b33880](../../../commit/6b33880c28cfdbc820cd646964e03ab6e4347cfc)
- Somewhere along the line Profile Name & Rating disappeared from the frame - [26db3d4](../../../commit/26db3d41472b2328d76b0272aa7dc84d5013d6c0)
- Up didn't reset Judge conversions in Evaluation - [d791e9f](../../../commit/d791e9fbc98738123a9ca9e21421f95804271dc2)
