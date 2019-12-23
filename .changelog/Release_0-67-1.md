# Release Changelog


## [0.67.1] - 2019-12-23 - Hotfix

### Added
- Lua has access to the Display Refresh Rate - [4c5dec0](../../../commit/4c5dec0687b368019b678bb3922fbeff23c2e050)
- Lua has access to the Replay Judge on Evaluation - [9a212ce](../../../commit/9a212ce9496936b17e32bba89afd20038dfabeeb)
- Music Wheel keeps track of filtered charts overall and by group - [3065367](../../../commit/3065367bec6b9c3f784a2f3bb8b630edfee9fd7c)

### Changed
- Random Song pickers (global and group) consider filters and search - [efdda19](../../../commit/efdda19a3c9769b78fcab0d409e72ad8c7fc1152)

### Removed
- AutoSyncTempo - [f00203f](../../../commit/f00203fb79829bf251562ae8b8afea282969af95)
- BPM Adjustments during Gameplay, manually - [f328290](../../../commit/f3282903c831ff946cb1c0a16ca319d27eb65a0b)

### Fixed
- Autosync Song stopped working for some Songs for unknown reasons - [c7093ed](../../../commit/c7093edf371ae6c0f23adf5b80fc43e828101871)
- CDTitles didn't update correctly depending on the Chart Preview state if changing Style - [ba396d9](../../../commit/ba396d988733a50f380b8381c5d28ed8e807c777)
- Chart Preview died completely if changing difficulty while changing song after having Chart Preview turned on and visible and the Style didn't change - [0d20da9](../../../commit/0d20da9631e501ef8da78c92580cc12e644fa6b2)
- Chart Preview disappeared when unfavoriting a Song - [21546b2](../../../commit/21546b2035956aaac641665fe34fe47d8ab1304f)
- Chart Preview restarted when favoriting a Song - [956be6f](../../../commit/956be6f1f338f998505fd7550e28d9f62b150b98)
- Chart Preview didn't work right when toggling - [97396a4](../../../commit/97396a44c443949a4e5881f436e157bedf5dff94)
- Evaluation didn't set Judge correctly for Replays, so now it kind of does better by setting to the Judge of the Replay - [b6dffb7](../../../commit/b6dffb7b500d05f700633ECE05DC6d410ee2fcd3)
- NoteSkin Previews always failed in non-Dance Games - [a7c80ac](../../../commit/a7c80acf37e7ae888000c8436da207e1e432758d)
- Player 2 was still working in Gameplay and could still hit Mines - [9991f49](../../../commit/9991f4901c29539ae375467ebd0565034f986c9c)
- Playmodes crashed the game oddly when messing with the main menu and autosync - [f9136ce](../../../commit/f9136ced71c97e1f138852e5f92a9b72880282b2)
- Replays can desync in some cases. When that happened before, any tap could be hit in any order from any distance - [913b0d8](../../../commit/913b0d8fb04c241f3ddf939b56a819f8a3d59a51)
- Replays didn't apply Mirror when Mod Emulation was turned off - [190eead](../../../commit/190eeade1ac3c0a64ae08436e127f5328fc3c5ed)
- Toasty makes a sound but did not appear again in Practice after resets - [66642ba](../../../commit/66642bacf66af709f73f0ad3fcf3f4e7e38a2e6a)
