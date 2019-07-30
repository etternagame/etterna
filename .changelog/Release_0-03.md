# Release Changelog


## [0.03.0] - 2016-09-09 - More FPS Improvements

Binary only release.

### Added
- Chartkey and lists of non-empty songrows to NoteData objects - [b6c0d03](../../../commit/b6c0d03580667ddb637ab748c7c9eaf31087ad05)
- Preference for PitchRates - [7526783](../../../commit/75267835ada422ea014be09b408a78bdc98f0752)
- Preference for FrameLimit & FrameLimitGameplay - [c7107ef](../../../commit/c7107efbd6cf90e1728fd4d3f8bb84e8bc90593b) [68fbbc0](../../../commit/68fbbc09e4e3dd04068809052c21a2308e8b917f) [4872f09](../../../commit/4872f09a4fd5969830401d705e97db68855fdb13)
### Changed
- Don't update NoteDisplay every frame (twice) - [568d9fe](../../../commit/568d9fe9181d9930fa4252fd340001d123dc40ed)
- Don't care about setting frames if there are no frames to animate on NoteSkinAnimations - [a100fc0](../../../commit/a100fc00add95d933be939af4554aa45b040ddf6)
- Don't update Tornado/Beat/Invert when they aren't on - [aabcc15](../../../commit/aabcc1539b16776d02436876466cda3074af26dc)
- Don't update DebugOverlay if it's not visible - [35a33b1](../../../commit/35a33b16c137b242279f09465532d83564206792)
- Lights and DancingCharacters no longer update in Gameplay - [99fab7c](../../../commit/99fab7c1eee68e389b143cafda1101af30890f03) [fb2266f](../../../commit/fb2266ff4930af00884ec6011a6560115046e8dd)
- MusicWheelItems show the highest grade for a file - [14a6bed](../../../commit/14a6bed739193154852c53935755beb34f425aa8)
- Optimize calls to get the elapsed time for Beats or NoteRows - [a6f9831](../../../commit/a6f9831afc6a1ba33182f872ff4cab9e5c4c2fcc) [41aafa4](../../../commit/41aafa4cd3e0ebeb160569ef0ac12a17bdba09a8) [ddf6bc6](../../../commit/ddf6bc6c3708e5e08895b332ae4fca4123214c56)
- Pointerise GraphcsWindow's GetParams for an FPS boost - [563eb77](../../../commit/563eb77c3d5f87a4519b6bcede732420725a2b23) 
- Replace many typecasts with static casts - [cf92975](../../../commit/cf92975dcb871909e1a856a58a391084404dc640) [69e5251](../../../commit/69e5251e5ab8aa15d10ef3f125cc08c18352d807) [8d9d23a](../../../commit/8d9d23a45cac5f81194bf4558358a2bbac213196)
- When clearing FileSets, only iterate through what we need - [4a36892](../../../commit/4a3689216321d0fa6616e514b9299493e838377c)
### Removed
- CrossedBeat messages for every Gameplay update - [151c550](../../../commit/151c55046812d4754db897aac910a3d57bddb389)
### Fixed
- Gameplay was stuttering when using DirectSound-sw, caused by duplicate positions returned from sound drivers - [427f9f0](../../../commit/427f9f0ff2648a4d2281995f814cde331f66e372)