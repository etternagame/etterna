# Release Changelog


## [0.50.0] - 2016-12-04 - Public Beta Release - MSD & Wife

Windows installer release meant to be installed in its own clean directory.

### Added
- Installer will install VS2015 C++ x86 redistros & portable.ini - [cc8add3](../../../commit/cc8add3af0041be3bca33124b2975757fa8fe103) [5a5eec8](../../../commit/5a5eec8681705e4d3ecc6b201190b31526fe0808)
- Millisecond Scoring system (MSS & Wife) - [0e02141](../../../commit/0e0214155a23570f23bd9a68332dd6f7d30aaf11) [0c1eaf3](../../../commit/0c1eaf311c05fcf7e1498f691386df1a505c7aa8) [bc26e1d](../../../commit/bc26e1dd2ccda6a317ff9696228676238fe2af7e) [92f3398](../../../commit/92f33981e6449141bf07b88bdf5990e11a12ae6f) [bd9ad63](../../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
- Mina Standardized Difficulty (MSD) - [bd9ad63](../../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0) [065aef4](../../../commit/065aef43d98e8dcf3738028b3d533e756af159ed)
- Polling on X11 Input Thread - [e8964fc](../../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9)
- Predictive Frame Limiter - [8fbd324](../../../commit/8fbd32486587ae5bfaf7e9386a8ba5c81fb5d45f) [b27d673](../../../commit/b27d673159969efbe94ea4a4540c0f5e42886b0c) [3d0f895](../../../commit/3d0f8955f419d9dce968aff57d6d588a27e57201) [4a1dca9](../../../commit/4a1dca90bd321c25367b2f2ed0957ccb0fe4774b) [edbb2df](../../../commit/edbb2df59893a942cd9d76f6935c4e9edd03fda9) [a77e8b7](../../../commit/a77e8b7af40aaf12f3f573d77e122ac0c96375dc)
- Preference for Allowed Lag in milliseconds - [87a8085](../../../commit/87a80858e21969ea52bcda603503cd29b3876a28)
- Some new songs come shipped with Etterna - [bd9ad63](../../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
- SongFinished message for Fail Grades - [70a432f](../../../commit/70a432f58be30ff2a02d786d6141dc8d038f5766)
- The Til Death theme & some optimized Noteskins are now shipped with Etterna - [bd9ad63](../../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
- Uninstaller will uninstall all proper files - [5abe56f](../../../commit/5abe56f64b7d07328f23aae5f1e61bbdce535fe3)
### Changed
- C++ Standard is now C++11. CMake standard is now 3.2.0. - [89eda0d](../../../commit/89eda0d5c1d5efbf13bc8f8e447565a093dacce1) [b0821f8](../../../commit/b0821f858c328c4f4793ce8b2a3d7b8a8bd23839)
- Caching and Uncaching Noteskins no longer calls MakeLower several times - [f7f2e52](../../../commit/f7f2e52a1a2c605cbf96bbfea9ce5d25010ae11e)
- Calculate instead of search for visible Beats for Taps - [cc491ff](../../../commit/cc491ff538bbaf8de99c83ffd6228931d58aa3d2) [028b8f2](../../../commit/028b8f2ee3f1a8d5e3d9c60cba1ea06cd0f51072) [1e16e08](../../../commit/1e16e087f43655780508749ed892ebcd1ac36084) [26b8064](../../../commit/26b80644b59bce730b793bdd22cc401ad14531ba) [28cce6d](../../../commit/28cce6dbd244894b011b14be6c52168a0bd9581c) [7efa383](../../../commit/7efa3834adee23242e1b4c7a8d504870d762c4d0)
- CrossedBeat Messages can be sent - [e973335](../../../commit/e973335c4a877814c8363f2a32b7e45c8a854bc9)
- D3D Display Resolutions are now unsigned ints because they can't be negative - [aec2955](../../../commit/aec295548e28ffc46a3d0f212c92a8bc8b6b410d)
- DancingCharacters & Lights are allowed to update - [fa2870d](../../../commit/fa2870d7c32d3dc1a9fc139cdbbcd4295434748f) [956b81c](../../../commit/956b81c6357ce625bd3dbead9a52900ae0c49862)
- Default preferences & bindings are more the commonly used ones - [16d03b2](../../../commit/16d03b212440002ab4a6292fcc28f59818070d67) [e8964fc](../../../commit/e8964fCB5753BA9734ed978e8950f4a7af31acb9) [1b49619](../../../commit/1b496196d8a98b9b9996d1e88242f93919d213bb) [053a655](../../../commit/053a655f2a43f0aa6eaffa1b6061074c79391c58)
- DirectX is the default renderer on Windows - [5a5eec8](../../../commit/5a5eec8681705e4d3ecc6b201190b31526fe0808)
- Don't autogenerate courses by default - [935d764](../../../commit/935d764d24e70c1286e7612029794355ad954f35)
- Don't autogenerate lights data by default - [d973824](../../../commit/d9738246f231f1364be43a20289d7c001bf7b270)
- Don't calculate the NonEmptyRow Vector elapsed times every time NoteData is loaded - [1f1b75c](../../../commit/1f1b75c102c8cc74c9f0050639c6a05ecaf1c8b0)
- Don't get a song cache hash unless needed (songload optimization) - [2b3722c](../../../commit/2b3722ccdbdffe2ee1d3c3f0b2a442c98a020cf7)
- FPS only updates per rendered frame - [6404aee](../../../commit/6404aeef183b5a74fbb261d90c98a5bea1b8be28)
- Generate Chartkeys in the load from disk to cache process - [c21c490](../../../commit/c21c4909ca9c2cf087a46247db924dde60ea5466) [c154cb8](../../../commit/c154cb87c6af0ecef6e046ab93c2fc5cc0eb42e6)
- Improve song load speed by optimizing a for loop - [10808b4](../../../commit/10808b4b707c4e2908e0ffffd3af618f6606bb73)
- Improve stat calculation lag & use std::chrono for timers such as device timestamps and fDeltaTime - [8bec801](../../../commit/8bec801c67bb4a70ad8919ceec57aade10b08bfc) [57bc635](../../../commit/57bc63571aead7b8bfdbf8c06faffe8a6058cd3b) [d59c4eb](../../../commit/d59c4ebc9af13992422af7f39cb8d08aebb2bbe4) [3a1cab2](../../../commit/3a1cab21551e4c45A21801FAb7906d052327794e) [33802bf](../../../commit/33802BF5104a2fc0e469c85d49a05a076d9a281c)
- Invalid (autoplay & negative points) scores don't save - [de0eb09](../../../commit/de0eb0947dbe3699aa8becbd37c4f629ba49e92a)
- NoteDisplay updates every frame again, since the game runs faster - [150325c](../../../commit/150325ca733e0765099a934bf91ee8ef7ec0b771)
- Optimize calls to BeatToNoteRow by replacing it with a function that uses the calculated timestamps from the Non Empty Row Vectors - [de1ebe0](../../../commit/DE1EBE095C44dbc951c34292e24d3fd3ff6296aa)
- Optimize FindEvent function in TimingData - [dd6cb56](../../../commit/dd6cb560b5699b44247f0167f6d90888213c19b8)
- Pulled SM 5.1 changes for cache writing & loading some SSC files - [2196aa0](../../../commit/2196aa00de6a75f1cecbb76f2b74e95d055263cb)
- Pulled SM 5.1 changes to remove the custom Mersenne Twister, replace it with std::shuffle - [c158f80](../../../commit/c158f80bd5e745a0ee89162a997965650e5e17b2)
- RStrings and some other things are now almost always passed by reference - [dac3888](../../../commit/dac3888b71d368dbcbf3bca520f8b511afe43070) [3ef472b](../../../commit/3ef472bcca457f04e006c856640ef4f8104beefd)
- Replace Rage toUpper/toLower functions with std functions for a 32% speed boost on song loading & optimizations for Screens with lots of text - [d4331d8](../../../commit/d4331d806df4fd2ce2d3748684ec2e2c140e856d) [e778a35](../../../commit/e778a359c418d3c3e8618de270e4e52cbd937e6e)
### Removed
- Redundant NoteDisplay IsOnScreen function only checked what is already checked for all cases elsewhere - [0de46d1](../../../commit/0de46d1737515665aa52efb8c172008fa9ce553a) 
- Unneeded Hold body rendering overshoot - [dd0c1f9](../../../commit/dd0c1f9023bbf37f7b1634dfc61054e4100ff3d7)
### Fixed
- Edit Mode was crashing upon any Gameplay Judgments - [b5be683](../../../commit/b5be683a86fa17b192d261357892c6d28a3648ad)
- Holds in skins which don't explicitly flip the head/tail were improperly rendered - [b337430](../../../commit/b33743081eee80693a7cd320ba6d7d6809887e7f) [acdefc2](../../../commit/acdefc2805c134cfec7396ff9b45f70945035f10)
- Hold tails were wrongly flipped - [aca2bd4](../../../commit/aca2bd4fc3b4475f22bf91287dfa0ca4f2f5489c)
- Installer didn't work - [46bdf4b](../../../commit/46bdF4BD7DFED965135e481dfbf395816aa3f1a2)
- Unlocked Gameplay FrameLimiter froze the game sometimes - [2b4d67e](../../../commit/2b4d67e0c075f6f4a54247602d020a06891f03dc)