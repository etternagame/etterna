# Changelog
All releases of Etterna are listed in this file as well as all of the major changes for each.

## [0.50.0] - 2016-12-04 - Public Beta Release

Windows installer release meant to be installed in its own clean directory.

### Added
- Installer will install VS2015 C++ x86 redistros & portable.ini - [cc8add3](../../commit/cc8add3af0041be3bca33124b2975757fa8fe103) [5a5eec8](../../commit/5a5eec8681705e4d3ecc6b201190b31526fe0808)
- Uninstaller will uninstall all proper files - [5abe56f](../../commit/5abe56f64b7d07328f23aae5f1e61bbdce535fe3)
- Predictive Frame Limiter (read the commit messages for a long explanation) - [8fbd324](../../commit/8fbd32486587ae5bfaf7e9386a8ba5c81fb5d45f) [b27d673](../../commit/b27d673159969efbe94ea4a4540c0f5e42886b0c) [3d0f895](../../commit/3d0f8955f419d9dce968aff57d6d588a27e57201) [4a1dca9](../../commit/4a1dca90bd321c25367b2f2ed0957ccb0fe4774b) [edbb2df](../../commit/edbb2df59893a942cd9d76f6935c4e9edd03fda9) [a77e8b7](../../commit/a77e8b7af40aaf12f3f573d77e122ac0c96375dc)
- Millisecond Scoring system (MSS & Wife) - [0e02141](../../commit/0e0214155a23570f23bd9a68332dd6f7d30aaf11) [0c1eaf3](../../commit/0c1eaf311c05fcf7e1498f691386df1a505c7aa8) [bc26e1d](../../commit/bc26e1dd2ccda6a317ff9696228676238fe2af7e) [92f3398](../../commit/92f33981e6449141bf07b88bdf5990e11a12ae6f) [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9)
- Mina Standardized Difficulty (MSD) - [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9) [065aef4](../../commit/065aef43d98e8dcf3738028b3d533e756af159ed)
- Preference for Allowed Lag in milliseconds - [87a8085](../../commit/87a80858e21969ea52bcda603503cd29b3876a28)
- SongFinished message for Fail Grades - [70a432f](../../commit/70a432f58be30ff2a02d786d6141dc8d038f5766)
- Polling on X11 Input Thread - [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9)
- The Til Death theme & many optimized Noteskins are now shipped with Etterna - [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9)
### Changed
- Default preferences & bindings are more the commonly used ones - [16d03b2](../../commit/16d03b212440002ab4a6292fcc28f59818070d67) [e8964fc](../../commit/e8964fCB5753BA9734ed978e8950f4a7af31acb9) [1b49619](../../commit/1b496196d8a98b9b9996d1e88242f93919d213bb) [053a655](../../commit/053a655f2a43f0aa6eaffa1b6061074c79391c58)
- DirectX is the default renderer on Windows - [5a5eec8](../../commit/5a5eec8681705e4d3ecc6b201190b31526fe0808)
- C++ Standard is now C++11. CMake standard is now 3.2.0. - [89eda0d](../../commit/89eda0d5c1d5efbf13bc8f8e447565a093dacce1) [b0821f8](../../commit/b0821f858c328c4f4793ce8b2a3d7b8a8bd23839)
- NoteDisplay updates every frame again, since the game runs faster - [150325c](../../commit/150325ca733e0765099a934bf91ee8ef7ec0b771)
- Improve song load speed by optimizing a for loop - [10808b4](../../commit/10808b4b707c4e2908e0ffffd3af618f6606bb73)
- Don't calculate the NonEmptyRow Vector elapsed times every time NoteData is loaded - [1f1b75c](../../commit/1f1b75c102c8cc74c9f0050639c6a05ecaf1c8b0)
- Generate Chartkeys in the load from disk to cache process - [c21c490](../../commit/c21c4909ca9c2cf087a46247db924dde60ea5466) [c154cb8](../../commit/c154cb87c6af0ecef6e046ab93c2fc5cc0eb42e6)
- Optimize FindEvent function in TimingData - [dd6cb56](../../commit/dd6cb560b5699b44247f0167f6d90888213c19b8)
- RStrings and some other things are now almost always passed by reference - [dac3888](../../commit/dac3888b71d368dbcbf3bca520f8b511afe43070) [3ef472b](../../commit/3ef472bcca457f04e006c856640ef4f8104beefd)
- Pulled SM 5.1 changes for cache writing & loading some SSC files - [2196aa0](../../commit/2196aa00de6a75f1cecbb76f2b74e95d055263cb)
- Pulled SM 5.1 changes to remove the custom Mersenne Twister, replace it with std::shuffle - [c158f80](../../commit/c158f80bd5e745a0ee89162a997965650e5e17b2)
- DancingCharacters & Lights are allowed to update - [fa2870d](../../commit/fa2870d7c32d3dc1a9fc139cdbbcd4295434748f) [956b81c](../../commit/956b81c6357ce625bd3dbead9a52900ae0c49862)
- CrossedBeat Messages can be sent - [e973335](../../commit/e973335c4a877814c8363f2a32b7e45c8a854bc9)
- D3D Display Resolutions are now unsigned ints because they can't be negative - [aec2955](../../commit/aec295548e28ffc46a3d0f212c92a8bc8b6b410d)
- Replace Rage toUpper/toLower functions with std functions for a 32% speed boost on song loading & optimizations for Screens with lots of text - [d4331d8](../../commit/d4331d806df4fd2ce2d3748684ec2e2c140e856d) [e778a35](../../commit/e778a359c418d3c3e8618de270e4e52cbd937e6e)
- Don't get a song cache hash unless needed (songload optimization) - [2b3722c](../../commit/2b3722ccdbdffe2ee1d3c3f0b2a442c98a020cf7)
- Don't autogenerate lights data by default - [d973824](../../commit/d9738246f231f1364be43a20289d7c001bf7b270)
- Don't autogenerate courses by default - [935d764](../../commit/935d764d24e70c1286e7612029794355ad954f35)
- Improve stat calculation lag & use std::chrono for timers such as device timestamps and fDeltaTime - [8bec801](../../commit/8bec801c67bb4a70ad8919ceec57aade10b08bfc) [57bc635](../../commit/57bc63571aead7b8bfdbf8c06faffe8a6058cd3b) [d59c4eb](../../commit/d59c4ebc9af13992422af7f39cb8d08aebb2bbe4) [3a1cab2](../../commit/3a1cab21551e4c45A21801FAb7906d052327794e) [33802bf](../../commit/33802BF5104a2fc0e469c85d49a05a076d9a281c)
- FPS only updates per rendered frame - [6404aee](../../commit/6404aeef183b5a74fbb261d90c98a5bea1b8be28)
- Calculate instead of search for visible Beats for Taps - [cc491ff](../../commit/cc491ff538bbaf8de99c83ffd6228931d58aa3d2) [028b8f2](../../commit/028b8f2ee3f1a8d5e3d9c60cba1ea06cd0f51072) [1e16e08](../../commit/1e16e087f43655780508749ed892ebcd1ac36084) [26b8064](../../commit/26b80644b59bce730b793bdd22cc401ad14531ba) [28cce6d](../../commit/28cce6dbd244894b011b14be6c52168a0bd9581c) [7efa383](../../commit/7efa3834adee23242e1b4c7a8d504870d762c4d0)
- Caching and Uncaching Noteskins no longer calls MakeLower several times - [f7f2e52](../../commit/f7f2e52a1a2c605cbf96bbfea9ce5d25010ae11e)
- Optimize calls to BeatToNoteRow by replacing it with a function that uses the calculated timestamps from the Non Empty Row Vectors - [de1ebe0](../../commit/DE1EBE095C44dbc951c34292e24d3fd3ff6296aa)
- Invalid (autoplay & negative points) scores don't save - [de0eb09](../../commit/de0eb0947dbe3699aa8becbd37c4f629ba49e92a)
### Removed
- Unneeded Hold body rendering overshoot - [dd0c1f9](../../commit/dd0c1f9023bbf37f7b1634dfc61054e4100ff3d7)
- Redundant NoteDisplay IsOnScreen function only checked what is already checked for all cases elsewhere - [0de46d1](../../commit/0de46d1737515665aa52efb8c172008fa9ce553a) 
### Fixed
- The installer not working - [46bdf4b](../../commit/46bdF4BD7DFED965135e481dfbf395816aa3f1a2)
- Improper rendering for Holds in skins which don't flip the head/tail - [b337430](../../commit/b33743081eee80693a7cd320ba6d7d6809887e7f) [acdefc2](../../commit/acdefc2805c134cfec7396ff9b45f70945035f10)
- Wrongly flipped Hold tails - [aca2bd4](../../commit/aca2bd4fc3b4475f22bf91287dfa0ca4f2f5489c)
- Unlocked Gameplay FrameLimiter locked sometimes - [2b4d67e](../../commit/2b4d67e0c075f6f4a54247602d020a06891f03dc)
- Edit Mode crash on Judgments - [b5be683](../../commit/b5be683a86fa17b192d261357892c6d28a3648ad)



## [0.03.0] - 2016-09-09 - More FPS Improvements

Binary only release.

### Added
- Chartkey and lists of non-empty songrows to NoteData objects - [b6c0d03](../../commit/b6c0d03580667ddb637ab748c7c9eaf31087ad05)
- Preference for PitchRates - [7526783](../../commit/75267835ada422ea014be09b408a78bdc98f0752)
- Preference for FrameLimit & FrameLimitGameplay - [c7107ef](../../commit/c7107efbd6cf90e1728fd4d3f8bb84e8bc90593b) [68fbbc0](../../commit/68fbbc09e4e3dd04068809052c21a2308e8b917f) [4872f09](../../commit/4872f09a4fd5969830401d705e97db68855fdb13)
### Changed
- Lights and DancingCharacters no longer update in Gameplay - [99fab7c](../../commit/99fab7c1eee68e389b143cafda1101af30890f03) [fb2266f](../../commit/fb2266ff4930af00884ec6011a6560115046e8dd)
- MusicWheelItems show the highest grade for a file - [14a6bed](../../commit/14a6bed739193154852c53935755beb34f425aa8)
- Don't update NoteDisplay every frame (twice) - [568d9fe](../../commit/568d9fe9181d9930fa4252fd340001d123dc40ed)
- Don't care about setting frames if there are no frames to animate on NoteSkinAnimations - [a100fc0](../../commit/a100fc00add95d933be939af4554aa45b040ddf6)
- Don't update Tornado/Beat/Invert when they aren't on - [aabcc15](../../commit/aabcc1539b16776d02436876466cda3074af26dc)
- Don't update DebugOverlay if it's not visible - [35a33b1](../../commit/35a33b16c137b242279f09465532d83564206792)
- Optimize calls to get the elapsed time for Beats or NoteRows - [a6f9831](../../commit/a6f9831afc6a1ba33182f872ff4cab9e5c4c2fcc) [41aafa4](../../commit/41aafa4cd3e0ebeb160569ef0ac12a17bdba09a8) [ddf6bc6](../../commit/ddf6bc6c3708e5e08895b332ae4fca4123214c56)
- Replace many typecasts with static casts - [cf92975](../../commit/cf92975dcb871909e1a856a58a391084404dc640) [69e5251](../../commit/69e5251e5ab8aa15d10ef3f125cc08c18352d807) [8d9d23a](../../commit/8d9d23a45cac5f81194bf4558358a2bbac213196)
- Pointerise GraphcsWindow's GetParams for an FPS boost - [563eb77](../../commit/563eb77c3d5f87a4519b6bcede732420725a2b23) 
- When clearing FileSets, only iterate through what we need - [4a36892](../../commit/4a3689216321d0fa6616e514b9299493e838377c)
### Removed
- CrossedBeat messages for every Gameplay update - [151c550](../../commit/151c55046812d4754db897aac910a3d57bddb389)
### Fixed
- Stuttering in DirectSound-sw caused by duplicate positions returned from sound drivers - [427f9f0](../../commit/427f9f0ff2648a4d2281995f814cde331f66e372)

## [0.02.0] - 2016-08-31 - Early Experimental FPS Improvements

Binary only release.

### Changed
- For Windows, use D3DX instead of RageMath for some calculations - [2d4c053](../../commit/2d4c0538a683da87c0a09ac68304f953b04542fa)
- FPS with Holds on screen improved by 37%
  - Don't draw the Tap part of a hold within DrawHoldPart [3ba8bd5](../../commit/3ba8bd503a207e285880b199d30887d9b74ade01)
  - Don't glow the Hold until it should glow [3ba8bd5](../../commit/3ba8bd503a207e285880b199d30887d9b74ade01)
  - Don't draw the Hold body head if the head is past the tail [434eaa8](../../commit/434eaa846f5e8b43b139589132ec26a37d335cf0) 
- Reduce the internally bloated nature of the LifeMeterBar & StreamDisplay (for FPS) - [6b05c2b](../../commit/6b05c2b89ee2bcbb6ec6fabce09a3e953a0c09e7) [a2129a3](../../commit/a2129a39e9747e2bcf5d2a8b5f5e0a2017693352) [bbd0be0](../../commit/bbd0be03edffc47e7a926bbe12f134c6e82ec535)
- Replace nearly all lrintf calls with lround - [a559386](../../commit/a5593868c478a1b6c66b628f28805190818f8bb3)
- Truncate instead of round in a critical "float to char" function - [2faa10b](../../commit/2faa10b023a66745fe23aa0a8aed96fd0341264a)
### Removed
- Unnecessary check for judgeable NoteRows - [d412cbf](../../commit/d412cbf23aebe6f464a48aec7b9b6f5bf1795524)
- The need for Windows Aero for VSync Off in D3D - [8e0a94f](../../commit/8e0a94f0c7806a850d84df65750189a8d5e95ef7)


## [0.01.0] - 2016-08-26 - Early Experimental Release

Binary only release.

### Changed
- Replace BeatToNoteRowNotRounded with BeatToNoteRow & typecast instead of round in BeatToNoteRow, because rounding is slow - [eed2f6e](../../commit/eed2f6e7c2ebb36af7b31b3d1cc4ba5992a88ba0) 
- Set the start of songs to be constant, independent of playback rate - [63c5efe](../../commit/63c5efe778efc7c853c0636641e9e7d5c1570d2e)
- Try to allow VSync off using Aero in D3D - [d51205b](../../commit/d51205b174ced006aace4ac9a7d44affa0bfe872)
### Removed
- Groove Radar Calculations (nobody uses them) - [fa53caf](../../commit/fa53cafb80ee8f450cad4baf6fbcc0d2156d71aa)
- Multithreaded D3D (for FPS) - [e0f4d7c](../../commit/e0f4d7c43c649f3f83d703f35779a0ff53553ba6)
- 1ms sleep in the Frame Limiter on Windows - [e490364](../../commit/e4903649377257957728e907f313705dc4f18858)
### Fixed
- Rate System with Pitch - [b5f7cc7](../../commit/b5f7cc7707a5735bece3498ea2aac822ec484699)
- FPS drops in gameplay due to bpm changes - [e3e3460](../../commit/e3e346075f6411b648eed7b8fdf940287333a855)
- FPS drops exiting gameplay due to unnecessary looping -  [a7ca8c7](../../commit/a7ca8c7a5ec955430cd3fa55f056ba408bffa10f)
