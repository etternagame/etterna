## [0.03.0] - 2016-09-09 - More FPS Improvements

Binary only release.

### Changes
- CrossedBeat messages for every gameplay update are gone - [151c550](../../commit/151c55046812d4754db897aac910a3d57bddb389)
- Add chartkey and lists of non-empty songrows to NoteData objects - [b6c0d03](../../commit/b6c0d03580667ddb637ab748c7c9eaf31087ad05)
- Lights and DancingCharacters no longer update - [99fab7c](../../commit/99fab7c1eee68e389b143cafda1101af30890f03) [fb2266f](../../commit/fb2266ff4930af00884ec6011a6560115046e8dd)
- Don't update DebugOverlay if it's not visible - [35a33b1](../../commit/35a33b16c137b242279f09465532d83564206792)
- When clearing FileSets, only iterate through what we need - [4a36892](../../commit/4a3689216321d0fa6616e514b9299493e838377c)
- MusicWheelItems show the highest grade for a file - [14a6bed](../../commit/14a6bed739193154852c53935755beb34f425aa8)
- Don't update NoteDisplay every frame (twice) - [568d9fe](../../commit/568d9fe9181d9930fa4252fd340001d123dc40ed)
- Don't care about setting frames if there are no frames to animate on NoteSkinAnimations - [a100fc0](../../commit/a100fc00add95d933be939af4554aa45b040ddf6)
- Don't update Tornado/Beat/Invert when they aren't on - [aabcc15](../../commit/aabcc1539b16776d02436876466cda3074af26dc)
- Optimize calls to get the elapsed time for beats or noterows - [a6f9831](../../commit/a6f9831afc6a1ba33182f872ff4cab9e5c4c2fcc) [41aafa4](../../commit/41aafa4cd3e0ebeb160569ef0ac12a17bdba09a8) [ddf6bc6](../../commit/ddf6bc6c3708e5e08895b332ae4fca4123214c56)
- Replace many typecasts with static casts - [cf92975](../../commit/cf92975dcb871909e1a856a58a391084404dc640) [69e5251](../../commit/69e5251e5ab8aa15d10ef3f125cc08c18352d807) [8d9d23a](../../commit/8d9d23a45cac5f81194bf4558358a2bbac213196)
- Fix stuttering in DirectSound-sw caused by duplicate positions returned from sound drivers - [427f9f0](../../commit/427f9f0ff2648a4d2281995f814cde331f66e372)
- Pointerise GraphcsWindow's GetParams for an FPS boost - [563eb77](../../commit/563eb77c3d5f87a4519b6bcede732420725a2b23) 
- Add FrameLimit & FrameLimitGameplay - [c7107ef](../../commit/c7107efbd6cf90e1728fd4d3f8bb84e8bc90593b) [68fbbc0](../../commit/68fbbc09e4e3dd04068809052c21a2308e8b917f) [4872f09](../../commit/4872f09a4fd5969830401d705e97db68855fdb13)
- Add a preference for PitchRates - [7526783](../../commit/75267835ada422ea014be09b408a78bdc98f0752)


## [0.02.0] - 2016-08-31 - Early Experimental FPS Improvements

Binary only release.

### Changes
- For Windows, use D3DX instead of RageMath for some calculations - [2d4c053](../../commit/2d4c0538a683da87c0a09ac68304f953b04542fa)
- FPS with Holds on screen improved by 37% - [3ba8bd5](../../commit/3ba8bd503a207e285880b199d30887d9b74ade01) [434eaa8](../../commit/434eaa846f5e8b43b139589132ec26a37d335cf0) [d62c422](../../commit/d62c42280f2620c286c45590a9e5bbfb83a72e0a) [bbd0be0](../../commit/bbd0be03edffc47e7a926bbe12f134c6e82ec535)
- Remove the need for Windows Aero for VSync Off in D3D - [8e0a94f](../../commit/8e0a94f0c7806a850d84df65750189a8d5e95ef7)
- Reduce the internally bloated nature of the LifeMeterBar (improve FPS) - [6b05c2b](../../commit/6b05c2b89ee2bcbb6ec6fabce09a3e953a0c09e7) [a2129a3](../../commit/a2129a39e9747e2bcf5d2a8b5f5e0a2017693352)
- Replace nearly all lrintf calls with lround - [a559386](../../commit/a5593868c478a1b6c66b628f28805190818f8bb3)
- Truncate instead of round in a critical "float to char" function - [2faa10b](../../commit/2faa10b023a66745fe23aa0a8aed96fd0341264a)


## [0.01.0] - 2016-08-26 - Early Experimental Release

Binary only release.

### Changes
- Groove Radar Calculations Removed - [fa53caf](../../commit/fa53cafb80ee8f450cad4baf6fbcc0d2156d71aa)
- Don't use D3D Multithreaded - [e0f4d7c](../../commit/e0f4d7c43c649f3f83d703f35779a0ff53553ba6)
- Various minor FPS improvements - [eed2f6e](../../commit/eed2f6e7c2ebb36af7b31b3d1cc4ba5992a88ba0) [e3e3460](../../commit/e3e346075f6411b648eed7b8fdf940287333a855) [a7ca8c7](../../commit/a7ca8c7a5ec955430cd3fa55f056ba408bffa10f) [e490364](../../commit/e4903649377257957728e907f313705dc4f18858)
- Force the start of songs to be constant, independent of playback rate - [63c5efe](../../commit/63c5efe778efc7c853c0636641e9e7d5c1570d2e)
- Try to allow VSync off using Aero in D3D - [d51205b](../../commit/d51205b174ced006aace4ac9a7d44affa0bfe872)
