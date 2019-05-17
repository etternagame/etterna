# Release Changelog


## [0.62.1] - 2018-11-14 - Hotfix

Windows x64, Windows i386, and Mac installer release.

### Changed
- Optimize ActorFrames updating their children by changing an iterator loop to a range loop - [d116c02](../../../commit/d116c02c92317431f36be7d12d9a5aeadea9b948)
- Optimize NoteDisplays updating their resources by changing an iterator loop to a range loop - [df7c12b](../../../commit/df7c12b57f5120c4504a27987dec0f5f3e94bdd8)
- Optimize the Chord Density Graph by using ActorMultiVertex instead of a lot of little Quads - [dd02cfa](../../../commit/dd02cfa48b57deecfe21c3b26eeb27686a961042) [5c0fed2](../../../commit/5c0fed2e8c798c8f818632758daa1eec397831ae)
- Set one of the lines on the Title Screen to be color-configable - [8bfd6d3](../../../commit/8bfd6d32d89c928e1e4cd9c9da77816b7aad3e98)
- StepsList shouldn't be so slow - [bf5411f](../../../commit/bf5411ffc1144a100ad4e37905850af4a5ac4b04)
- Try not to load a Noteskin Resource for every color - [08798a2](../../../commit/08798a28d87e24ed98d362c292c8919ea8dbeeac)
### Fixed
- CDTitle & StepsList was in the way of Chart Preview in 4:3 - [f550b00](../../../commit/f550b00f93b3b6df70aae10c9b9fe44320c8865d)
- Chord Density Graph was too large for 4:3 - [86eafd0](../../../commit/86eafd0ff425690e9dc5785e47336d4b05e78cc9)
- Division by 0 error in Offset Plots when the length of time was 0 - [56aaf5f](../../../commit/56aaf5fd397b5a26000a2df4420b51e3b3a56eb3)
- Invalid Difficulty Enum Crashed in Score saving. Fixed by defaulting instead of crashing - [2c81ea0](../../../commit/2c81ea0fa82819ab4a0aadaffffb32300c24d79f)
- Leaderboard Grades didn't color correctly - [28d63b5](../../../commit/28d63b500fda09bda64f620ff6ba63aba4617928)
- Mouse was not visible in Fullscreen in CustomizeGameplay & Replays - [6c1790e](../../../commit/6c1790e1b0995f8fabcac8ac590de99c3930da6e)
- NoteField Y axis movement was still busted when switching between Reverse and Upscroll - [b0a3735](../../../commit/b0a3735aedb4966f21af751c7ae5bbf56776941a)
- StepsList appeared if changing tabs while not on a Song - [546a1dd](../../../commit/546a1dd7fcc1612aeab343bd0d290147f106a3f8)
