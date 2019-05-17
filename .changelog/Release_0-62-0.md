# Release Changelog


## [0.62.0] - 2018-11-11 - Chart Preview

Windows x64 & i386 installer release only.

### Added
- Chart Preview & Chord Density Graph - [677e13e](../../../commit/677e13e398e725eca38538d66045681636882dd2) [802720c](../../../commit/802720cba868b5c645261339d084a5c41016bd7b) [6a2affb](../../../commit/6a2affb0d5e8c73f6b7a5bfd4413ed44a4c897fe) [f60c413](../../../commit/f60c413e572fa48d6524ba733c44985c1da5fd77) [195efd5](../../../commit/195efd576a9598f2fa045ba7927f2c871aacd7d9) [fcb659a](../../../commit/fcb659a12d7d59df10f059624bc3d0d3c1da04c1) [611908b](../../../commit/611908beb011ea3e517c0f1e845d95cef9368b69) [6f09311](../../../commit/6f09311f0deb7dd5be17bb205fcd05df52f89672) [25e1da4](../../../commit/25e1da4eceb2917c7593ad096a67b3e6658dc599) [b562ae8](../../../commit/b562ae85e2c3ee504465c94fa0596df6038b890c) [9cece4f](../../../commit/9cece4f04f8f4fac37d79c36353917ec36920644) [0ac88ee](../../../commit/0ac88ee4dce8177e8dfb9f700549540229d99ba3) [0275096](../../../commit/02750966213be55d9d6a4d13353794ec5d891961) [6915211](../../../commit/6915211f843546a30dce8f9f5c4bddc2b4300557) [d3dd947](../../../commit/d3dd9473407843346a800904ee7e067afba1af9c) [64c7f05](../../../commit/64c7f052e7fc79688781b95c1e0bbacdd0ecad00) [14002c8](../../../commit/14002c8ef11582c560ea617b7e6cfc005abede83) [6ca9123](../../../commit/6ca91234ca80647a5251bffe01bd8a6855823f9f) [2c2b762](../../../commit/2c2b762cbaa5fc5e1e9998c1f52c0789d4808c1d) [ed014c7](../../../commit/ed014c7606e4e71a584db7334d33ea2c31791579) [08c6ba9](../../../commit/08c6ba9a68adb539283378bd8051224da60e5884) [9ef02b1](../../../commit/9ef02b1fbc0231c6233eaef99a6e509814f64874) [cd5827f](../../../commit/cd5827fb929010c638d0ac3312f89ff09d49b184) [c8f675a](../../../commit/c8f675a725ee77fec2db5b92ef8ff529eee8bdb3) [4d2d6da](../../../commit/4d2d6da69ff1470682167124f391444187da2b85)
- Chinese Translation - [abfeeed](../../../commit/abfeeeda22996a1e20882e5c58d3e83117a8a38a) [8f2804f](../../../commit/8f2804f98cbba1ec0471efee3059cb86091e00ad) [4ec4b26](../../../commit/4ec4b267898147003a28e8783e775b65d64f7bd1)
- ClearType display on the General tab - [1446cba](../../../commit/1446cba91f6c15b3bb4b99b6cd59716a302349e7) [b94bcb7](../../../commit/b94bcb70fffcede5ba8109b2ed3116a2cf9153f0) [eeef67e](../../../commit/eeef67ebe2b8f509fab6938fc7739f26770c583b)
- Hotkey for reloading Pack from Disk - [aa94450](../../../commit/aa9445054cc2afc2a5fb76d64c47ce6af33b65dd)
- Judge display on the Scores tab - [212995d](../../../commit/212995defcdcd24f54fbcba70150fad4dbbd2084)
- Language support for some Til Death Options - [4ec4b26](../../../commit/4ec4b267898147003a28e8783e775b65d64f7bd1)
- Lifebar added to CustomizeGameplay - [7ba8f36](../../../commit/7ba8f365826aa0b36b87b7b0f122163c184b8323)
- Lua access to the currently playing music TimingData - [1ebb47f](../../../commit/1ebb47f86486d56726d93f599a3c5eeb4969eb3a)
- Lua access to Volume - [8c89d91](../../../commit/8c89d91d444b3b499058515242ef06685681eb45)
- Minanyms - [0e76024](../../../commit/0e7602481eb63fad80b6f98cb7c6e7b6ec379aa7) [c0c6660](../../../commit/c0c666033f116d41543ac606b94a6ad920ed6d6c) [8c0c084](../../../commit/8c0c0840363c2d92cd152d9e197755578381af42)
- Mouse support, Leaderboard, Replay buttons, bounding boxes added to CustomizeGameplay in a big rewrite - [5321555](../../../commit/53215552a957b60aa9dc0c83451b246b8a5e4f1f) [83744a5](../../../commit/83744a5dff8183f7247d64a4d7a9584db43d6a93) [ba32793](../../../commit/ba32793195829aa321f04bef70fbc52c5b8f7108) [6d5cddd](../../../commit/6d5cdddbafb6992792edaea3749d0fba8c5851d8) [5ee5026](../../../commit/5ee50260bf26d0a8161d257c5ce1958ba05afade) [81aaec4](../../../commit/81aaec47bff47676289d968be30bbabcd66f54b7) [302cc29](../../../commit/302cc29d867e5aa5eabbcbb85f9f0a181d8e7e01) [bc3d7c6](../../../commit/bc3d7c69956b439b435bf93e8ac24f1813367ff2) [0564664](../../../commit/0564664411c5b298583c7daafe8297f76c5d5a5b)
- Preference to reduce the verbosity of Log Trace outputs - [4ef582b](../../../commit/4ef582bc40198c4cc279444d370752659569522b)
- RageSounds can have a defined start point ignoring the 0 time - [9761618](../../../commit/9761618904ce37d31fae20b1d68413fef63237ee)
- Right click to pause Music (this is a bug but it turned into a feature) - [c8f675a](../../../commit/c8f675a725ee77fec2db5b92ef8ff529eee8bdb3)
### Changed
- Changing Rate should update the Chart Leaderboards if set to CurrentRate appropriately - [16b7e75](../../../commit/16b7e75e816f83d7bae4ea6a2e54d7d4f364ceb1)
- Color Config options for the Main Menu - [414705f](../../../commit/414705f9ae677fd1653349bf10d5162f9dbf1784) [a9c4bf9](../../../commit/a9c4bf9a58acce30ddf49d73aaf71115da54cee0)
- Default buttons for Dance mode should be the arrow keys, not random other buttons - [d367cf2](../../../commit/d367cf2dc775d9b1af3476df0e84b8867ab096f2)
- Favorite Song count is replaced by a Refresh Songs button to trigger Differential Reload - [0a027fe](../../../commit/0a027fe8c5d53b567495a4646f12907aec4178d0)
- Reduce the amount of times disk ReplayData is checked for just doing nothing on SelectMusic - [1004525](../../../commit/1004525232c812b9d6aa55c11b3b6002984870ce) [abbacec](../../../commit/abbacecbb62f73eeb2388201059bc78eb1a4af3a)
- Reduce memory issues in ReplayData checking - [b2989a9](../../../commit/b2989a920aafd8463a14d06590eb41dc1b828eb9)
- Refactor a lot of Lua to remove Player 2 code & improve readability or speed - [0c1f72a](../../../commit/0c1f72a5c34f04afd99fa45d70daebc6bf66e90f) [9daacf6](../../../commit/9daacf6d89bbc16c70896480084abec8eafbed2d) [247c3a7](../../../commit/247c3a7c641b53e49f68508497de843d55cc1bdd)
- Rewrote default Player Config Lua Scripts for clarity - [bc3d7c6](../../../commit/bc3d7c69956b439b435bf93e8ac24f1813367ff2)
- Setting the position of a RageSound is done by restarting the RageSound with a new start point - [86086cb](../../../commit/86086cbe5d4550b3f3de9c72300be1a586eb0eae)
- StageAwards don't exist so don't use them to calculate ClearTypes - [9b8dbeb](../../../commit/9b8dbeb12ce5e8636b8036b7339a836cdb929f0c)
- StepsList moves up off screen instead of left - [7a53904](../../../commit/7a53904a3ab8a28f68ab01893b05ed40a12f01dd)
- Text in the Abort/Retry/Ignore dialog should match the buttons - [edd3676](../../../commit/edd367654b1fbdbb180711ef7831fb34f362a35d)
### Removed
- Default Player 2 back button - [76fd569](../../../commit/76fd56974049f73f985ea7ce3f10bba2020919d2)
- "Press Start To Join" on Profile Select - [eac7298](../../../commit/eac729835aedcc7b52f2c34301cc2941aa56311e)
### Fixed
- Avatars didn't appear in Profile Select - [217509a](../../../commit/217509a666d3aa098d094354c0012f66071317e9)
- Backgrounds didn't change when loading from Cache - [abe6986](../../../commit/abe69864e32f2c864d220be627683cd72ae84eac)
- Crashed when backing out of Profile Select - [4c234f3](../../../commit/4c234f3792052684d0e48e3b649c4b39b30470db)
- Crashed when saving a Score after making changes to a file - [60e28ca](../../../commit/60e28cad6cb6a1eb693f02881f7c993fc5622f34)
- Crashed when saving a Score sometimes. Try to catch it, but we don't know how to fix it - [941884e](../../../commit/941884e8636011d67b198c861663c2f9f48b88cf) [4475e60](../../../commit/4475e6028c1ce70e995d34c8028a593e3b534dcd)
- Game Update button was badly placed - [3e6d22e](../../../commit/3e6d22eb47a4c921ec4c79ad741ce5859b316c62)
- Linux didn't build due to unfinished changes from the previous version - [5f72a70](../../../commit/5f72a70b1fd27f4ee16e56cea4c8df507ff40d61)
- Mac didn't build due to asm vs C compilation - [709b405](../../../commit/709b405b1773a1915fb249e80d32653daad5ca97)
- Permamirror broke Replay Watching - [92e9868](../../../commit/92e9868337f7d12d38b7b6060ed897cee0118ccb)
- StepsList display didn't hide correctly when hovering packs - [7ffd402](../../../commit/7ffd4023ac6595afec77eeee8156be9a90f29198)
- Style didn't get set when changing Difficulty in some rare situation - [5a003a1](../../../commit/5a003a191795cf9e20ad7b4db873f64cdf224e23)
### Note
- Temporary (reverted) change: button remap lock/rewrite - [b6f0621](../../../commit/b6f06215ed7837774e7b49c7234b6933ea9840ae) [0135f67](../../../commit/0135f6793fe2a5a3e52cfdea1144f6983b71a818) [933f2c6](../../../commit/933f2c64752f736dbde2aa8626bb591a266811b6) [8939993](../../../commit/8939993d7a9b32f64b664f9685698cac88d2663a) [c3a6dcc](../../../commit/c3a6dcc6f9dc2aa60ce6e843cac5fa0837e2a214) [c6b3bee](../../../commit/c6b3bee52a24df430a60da1b6a88f573f61eba27) [f4d1d55](../../../commit/f4d1d5533a9e3f991bcfe73216ce6a72c2bdf002) [3c0c05c](../../../commit/3c0c05cdaf3413469a680212df758b0c95290b4e)
