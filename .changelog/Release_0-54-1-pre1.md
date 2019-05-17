# Release Changelog


## [0.54.1] - 2017-01-29 - Pre-release: DP to Wife Conversion

Windows installer pre-release.

### Added
- DP scores are converted to Wife on load - [3d5d780](../../../commit/3d5d780d7d9dfb93acd353c8b6c443d8e1c9cd08) [a7c2077](../../../commit/a7c207700d2eda16982c6759779b9c4a7b0d4201) [770dd77](../../../commit/770dd7772a16b88dabcf405407ca5dc62f687631)
- Game Version Getter - [0ad919a](../../../commit/0ad919a3c9a161f916f587f4260c0174e5972e4b)
- Instant restart button in Gameplay - [ba98107](../../../commit/ba981076a1de0b2e103447d4bfccda3d05a7ddc6)
- Mouse clicking to scroll on the MusicWheel - [79bc414](../../../commit/79bc414b307790b916f0e42c2d4674abf11d9bbb) [48e0fd8](../../../commit/48e0fd856d4ce722a68c2f30bb41d6ff6ae47e7d)
- Mouse wheel scrolling on the MusicWheel - [daabe76](../../../commit/daabe763cd59a6cb32ef8d3ce4da97b56643385f) [7acb1fd](../../../commit/7acb1fdfb78f35a5ade191a60add3e96cd631123) [126f08c](../../../commit/126f08c62c7368ae678154ea81fcd753b0ebe84b) [e24fded](../../../commit/e24fdede98789d4cc23d6e1552f0205bb2f5ee45) [a8eb1f7](../../../commit/a8eb1f74b3ab2cf962669a5454cde64bd81ec45f)
- "Validate All" button in the Profile tab - [3850176](../../../commit/38501760679dc8bea44899373c108b39dc250ed7) 
### Changed
- Changing Skillsets on the Profile tab resets the page number - [3fa475f](../../../commit/3fa475f0bd674e25bd4cf236090c5213d46d1144)
- DP to Wife conversions use mid-window values instead of worst case values - [d6aea48](../../../commit/d6aea48d58bd45212b4a0debd23888449d052b6b)
- Early unreleased Etterna Multiplayer Protocol manages Chartkeys, hashes, Noterow counts - [c4ba026](../../../commit/c4ba0261286414ef2b43761ebc857d0f90e866a0) [19fdf65](../../../commit/19fdf65ba4770294846ee82098595eb38c761ac6) [8333509](../../../commit/83335090eaaeaf5d666c88a97083703481d20414)
- Let SSR calculation functions work for any key that matches a loaded chart - [3b0fd08](../../../commit/3b0fd08a0e3ba705f21fae256aa702bdf41c4180)
- MinaCalc.a is linked to Linux CMake - [c796589](../../../commit/c79658957ef88b52017b968f7a576b779ffcbdcf)
- MSD Calc loads in a threaded way - [b25ccdc](../../../commit/b25ccdc45a6bf91cac8994d2ba1a83ea0571e995)
- Multiplayer Chatbox Scrolling - [2b557c9](../../../commit/2b557c964a86c7556b74108955844f5a24b2ee08)
- Only recalculate SSR/MSD values for certain files if they are valid - [dd92b88](../../../commit/dd92b881f902ea9f7988516fe951c400bd0e1a4d) [d296982](../../../commit/d296982a28e91b801bd39f9b155cd6673be55cbe) [ab67d00](../../../commit/ab67d00e1ba19e2951c1aeca97a15f749c142672) [f0b3bb9](../../../commit/f0b3bb9acdfaacab07fdfdd2c061a0e759f4cbdc)
- Optimize GetWifeGrade by not implicitly typecasting between floats and doubles - [cc8942d](../../../commit/cc8942d315264c99af45bf2270873021b8ca384d)
- Optimize the recalculation of SSRs using yet another mapping technique with Chartkeys - [3008dca](../../../commit/3008dca9a816968c29737b881a1d22a90b6f8a8b) [720ab62](../../../commit/720ab62c7324e6eba2f2cb4cba31aad2d5e3ed7d) [90612b8](../../../commit/90612b89844f07bebbf8de8ff2da087e7fd2ccce) [1a398dc](../../../commit/1a398dc36b4d130f2e8c8622a691c4f0df3411ef) [0bd7207](../../../commit/0bd7207ef1cfa25d29efe903b0cea693f68c3fe9) [988a0a3](../../../commit/988a0a3e856c5947c6fdaee0666cf73ff13886a1) [f4cb23d](../../../commit/f4cb23ddb2dfdbf39d452010f4a849f78650682e) [7f0278f](../../../commit/7f0278f29f41102d4b0075c512526bc0ed4ddc09) 
- Replay Data Loading returns success information - [846ad3c](../../../commit/846ad3cfea316153dbb977bb879688880ee9a45b)
- Wife-based Grades replace old Grades for HighScoreLists - [30b2fcc](../../../commit/30b2fcc51f025b8b8fd4f8449173299af56d96ee)
### Removed
- TopSSRSongIDs and TopSSRStepsIDs due to being replaced by a more efficient use of Chartkeys - [13dedd6](../../../commit/13dedd66eeffa084f3f85e934ec624e44464bd8d)
### Fixed
- Dropped or missed Holds rescored wrong - [6d24a62](../../../commit/6d24a62109acfe5654726979fec01a098b67cc37)
- Getting skillsets for rates outside of 0.7x to 2x caused crashes - [31676c4](../../../commit/31676c4eb0c11fed672dff2f1159deb7058a36fc) [597e3ba](../../../commit/597e3ba541777c9d9302f91d0f85d6fe67b12e76)
- Rate displays on Song Select were weird and inconsistent - [51b22c2](../../../commit/51b22c2b6bb404c3ccd26e3832eec5965713eb9a)
- Welcome window appeared twice in the installer - [951a524](../../../commit/951a524f06197fec4f2567c42c43a60f0c52d3df)
