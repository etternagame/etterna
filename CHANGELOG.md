# Changelog
All releases of Etterna are listed in this file as well as all of the major changes for each. All changes for each version apply in supplement to the ones below it. Changes are not in chronological order, only versions are.

## [0.54.2] - 2017-03-19 - Unreleased Iteration

Source only unofficial release.

### Added 
- Level system for Multiplayer only - [4086fbf](../../commit/4086fbf9c2664899ef53a999ed81f2a934117be1) [c7f2221](../../commit/c7f2221e65e7bca16c69c3a3040be4357729c95c) [801fb81](../../commit/801fb815436e7ff47ceef48b67154f9e6e5addfb) [bcd0477](../../commit/bcd04776c6ca28c75e037460b4a04e3968721462) [44aa0d2](../../commit/44aa0d2588cae4348577ad94ba304cd0526b25d7) [78ff27d](../../commit/78ff27df9e8435f891890142786e09b8c5650e0d)
- Initial Customize Gameplay work. Movable Gameplay elements - [b77b548](../../commit/b77b548333f3656583db22a19f757a0ec288df40) [0a42a98](../../commit/0a42a980466ffde54aa5b147082d4280783c12a4) [d253366](../../commit/d2533665a65b9d4ce3a7ac6751cf33270ff95627) [a6e4f83](../../commit/a6e4f831c5481478fbbf3c879f04388d6b152703) [e977220](../../commit/e9772202c45700d93c05dd026bc82224c02bd837) [d31d1ef](../../commit/d31d1ef204b5c9b6dea2b447942635caa85ee051)
- Early support for Chord Cohesion Off - [599f08a](../../commit/599f08a38cc9c2a0eaf7d1c08a4e3bbf02e8b977) [7de6738](../../commit/7de6738db267fb34839f20da12ffa558ddfcbd57) [0b45e1f](../../commit/0b45e1f4473fb39e87554c9b03c5381be394bc05) [e6024e2](../../commit/e6024e2bb44cbdb97da0394ab6565d9eb535ea6c) [f5d6712](../../commit/f5d6712b62b676a48f10aa8f86a835df1dff788a) [5759c88](../../commit/5759c88ef1150f5b0724348fc4c2067a5a98ac2d) [0d2688c](../../commit/0d2688c2dac45632c284b779a3454d17f84b422b) [569bf34](../../commit/569bf34fed30ed4e24ba9cb49e45a65bf88b8571) [98302d3](../../commit/98302d32081834ce02d22f118913bd1fd2399e7f) [64a7df1](../../commit/64a7df183aa6d16ab0275bb5e11337aadbbafd81)
### Changed
- Font change on installer image - [e83ed4e](../../commit/e83ed4ee5b9a904c38bda6f30eea50f1a7a3c533)
- Modify Multiplayer visually to support more aspect ratios - [7ccdb63](../../commit/7ccdb63869c20d97161d624053ef56b435b7cabc) [40d1c9b](../../commit/40d1c9b7c2c8d28fd30f30c7d1d1f71d82ad576c) [e9c891e](../../commit/e9c891ee62d6eee08e67798a90f3efdd5442e5f0) [eeb2c4e](../../commit/eeb2c4e44dc3cdbf9eeb521bc810181561b3c999)
- CDTitles moved to a better position - [20816d3](../../commit/20816d38034aabe89dba7977994d3f23a67de6c8)
- Optimize difficulty calculation by serializing NoteData - [3e2509a](../../commit/3e2509a801f0d8761282ecbea141b08081dcaaf4) [204c764](../../commit/204c76449930841147cd969850bad1c2841c1810)
- Moved Filters to a new FilterManager for organization - [758fda7](../../commit/758fda7c4b8b8a1741ccbb10dd0607b9e635f1ad) [f4074a0](../../commit/f4074a0a5aab66edca28e00e2450c836f89f89a9)
- Improve Predictive Frame Rendering by using C++11s std thread sleep since it works better - [3040671](../../commit/3040671c5560c5caf09cc56a652d79793676eaff)
- Optimize bitmap text rendering to use 1 draw call for all characters of a texture page (FPS improvements for all renderers) - [0bf1a6a](../../commit/0bf1a6ae613fcb8f30b05721ffa6691a14656147)
- Generate Chartkeys slightly faster using fewer function calls - [db13407](../../commit/db1340700c2cce920eb06b9fd751396898ed1cc5)
- Set DivideByZero to resolutions divisible by 16 - [f66406c](../../commit/f66406c3196c2382a6413912ecfce6e8790ed493)
- Improve drawing polygon lines by calculating each line before drawing the necessary quads - [d3d2f66](../../commit/d3d2f6605abf421d4bb78a2533fb77b7328f5587)
- Don't clamp judging old Noterows to a certain window in the past, but still clamp the future - [3716264](../../commit/3716264818878249de087d9ac73f242f60ea3af6)
- Only allow D3D multisampling if supported & don't round corners if SmoothLines is off - [df8a577](../../commit/df8a5779d3d419fe444cddee5e2695c74c4c474f)
- Don't garbage collect textures in Gameplay - [b988947](../../commit/b9889476fc6f0ea84b0d4fe596505953b15d1bb4)
- MSD Calc short file downscaler is lightened up - [9a633cb](../../commit/9a633cbcb561dd057b22b06cdd32d597266e0072)
### Removed
- Unused theme files - [afa6375](../../commit/afa63754015a877165dcbef8c8e57afae9c92a97)
- Game dependencies in the MSD Calc - [395736f](../../commit/395736f260d2291596a2d159815bf6fb4dc71d68)
- Multiple Toasties - [e377aac](../../commit/e377aac0f19f2768380d0ad05f724c7dd6c8f465)
### Fixed
- Crashing when using the mouse wheel after entering a song from Multiplayer lobbies - [17eb440](../../commit/17eb4407d7d2762cda1c849c4b511ab97e4352aa)
- Unicode Fonts load very wrong sometimes - [349f056](../../commit/349f0561ca016f7840fbd2e3ed87e0d68a951fae)
- Mines hit in DP to Wife rescoring don't get considered - [87e1a59](../../commit/87e1a5992fbad403fa015903cb56f28448d8e135)


## [0.54.1] - 2017-01-30 - Release Candidate

Windows installer pre-release.

### Fixed
- Minor oversight in player rating calculations - [def0123](../../commit/def0123e7c4113cbc1d113c8794b5a48af100ee8)

## [0.54.1] - 2017-01-29 - Pre-release: DP to Wife Conversion

Windows installer pre-release.

### Added
- Instant restart button in Gameplay - [ba98107](../../commit/ba981076a1de0b2e103447d4bfccda3d05a7ddc6)
- Game Version Getter - [0ad919a](../../commit/0ad919a3c9a161f916f587f4260c0174e5972e4b)
- DP scores are converted to Wife on load - [3d5d780](../../commit/3d5d780d7d9dfb93acd353c8b6c443d8e1c9cd08) [a7c2077](../../commit/a7c207700d2eda16982c6759779b9c4a7b0d4201) [770dd77](../../commit/770dd7772a16b88dabcf405407ca5dc62f687631)
- "Validate All" button in the Profile tab - [3850176](../../commit/38501760679dc8bea44899373c108b39dc250ed7) 
- Mouse wheel scrolling on the MusicWheel - [daabe76](../../commit/daabe763cd59a6cb32ef8d3ce4da97b56643385f) [7acb1fd](../../commit/7acb1fdfb78f35a5ade191a60add3e96cd631123) [126f08c](../../commit/126f08c62c7368ae678154ea81fcd753b0ebe84b) [e24fded](../../commit/e24fdede98789d4cc23d6e1552f0205bb2f5ee45) [a8eb1f7](../../commit/a8eb1f74b3ab2cf962669a5454cde64bd81ec45f)
- Mouse clicking to scroll on the MusicWheel - [79bc414](../../commit/79bc414b307790b916f0e42c2d4674abf11d9bbb) [48e0fd8](../../commit/48e0fd856d4ce722a68c2f30bb41d6ff6ae47e7d)
### Changed
- Early unreleased Etterna Multiplayer Protocol manages Chartkeys, hashes, Noterow counts - [c4ba026](../../commit/c4ba0261286414ef2b43761ebc857d0f90e866a0) [19fdf65](../../commit/19fdf65ba4770294846ee82098595eb38c761ac6) [8333509](../../commit/83335090eaaeaf5d666c88a97083703481d20414)
- Multiplayer Chatbox Scrolling - [2b557c9](../../commit/2b557c964a86c7556b74108955844f5a24b2ee08)
- DP to Wife conversions use mid-window values instead of worst case values - [d6aea48](../../commit/d6aea48d58bd45212b4a0debd23888449d052b6b)
- MinaCalc.a is linked to Linux CMake - [c796589](../../commit/c79658957ef88b52017b968f7a576b779ffcbdcf)
- MSD Calc loads in a threaded way - [b25ccdc](../../commit/b25ccdc45a6bf91cac8994d2ba1a83ea0571e995)
- Changing Skillsets on the Profile tab resets the page number - [3fa475f](../../commit/3fa475f0bd674e25bd4cf236090c5213d46d1144)
- Only recalculate SSR/MSD values for certain files if they are valid - [dd92b88](../../commit/dd92b881f902ea9f7988516fe951c400bd0e1a4d) [d296982](../../commit/d296982a28e91b801bd39f9b155cd6673be55cbe) [ab67d00](../../commit/ab67d00e1ba19e2951c1aeca97a15f749c142672) [f0b3bb9](../../commit/f0b3bb9acdfaacab07fdfdd2c061a0e759f4cbdc)
- Optimize the recalculation of SSRs using yet another mapping technique with Chartkeys - [3008dca](../../commit/3008dca9a816968c29737b881a1d22a90b6f8a8b) [720ab62](../../commit/720ab62c7324e6eba2f2cb4cba31aad2d5e3ed7d) [90612b8](../../commit/90612b89844f07bebbf8de8ff2da087e7fd2ccce) [1a398dc](../../commit/1a398dc36b4d130f2e8c8622a691c4f0df3411ef) [0bd7207](../../commit/0bd7207ef1cfa25d29efe903b0cea693f68c3fe9) [988a0a3](../../commit/988a0a3e856c5947c6fdaee0666cf73ff13886a1) [f4cb23d](../../commit/f4cb23ddb2dfdbf39d452010f4a849f78650682e) [7f0278f](../../commit/7f0278f29f41102d4b0075c512526bc0ed4ddc09) 
- Let SSR calculation functions work for any key that matches a loaded chart - [3b0fd08](../../commit/3b0fd08a0e3ba705f21fae256aa702bdf41c4180)
- Replay Data Loading returns success information - [846ad3c](../../commit/846ad3cfea316153dbb977bb879688880ee9a45b)
- Wife-based Grades replace old Grades for HighScoreLists - [30b2fcc](../../commit/30b2fcc51f025b8b8fd4f8449173299af56d96ee)
- Optimize GetWifeGrade by not implicitly typecasting between floats and doubles - [cc8942d](../../commit/cc8942d315264c99af45bf2270873021b8ca384d)
### Removed
- TopSSRSongIDs and TopSSRStepsIDs due to being replaced by a more efficient use of Chartkeys - [13dedd6](../../commit/13dedd66eeffa084f3f85e934ec624e44464bd8d)
### Fixed
- Crashing on getting skillsets for rates outside of 0.7x to 2x - [31676c4](../../commit/31676c4eb0c11fed672dff2f1159deb7058a36fc) [597e3ba](../../commit/597e3ba541777c9d9302f91d0f85d6fe67b12e76)
- Rate displays on Song Select are weird and inconsistent - [51b22c2](../../commit/51b22c2b6bb404c3ccd26e3832eec5965713eb9a)
- Dropped or missed Holds rescore wrong - [6d24a62](../../commit/6d24a62109acfe5654726979fec01a098b67cc37)
- Welcome window shows twice in the installer - [951a524](../../commit/951a524f06197fec4f2567c42c43a60f0c52d3df)


## [0.54.0] - 2017-01-24 - Official v0.54 Release

Windows installer release.

### Added
- New Etterna installer icons [35de48f](../../commit/35de48f9a9ed34c45c64d7cae1264f0386ef3b97)
- Save Profile button in the Profile tab - [9f8c9a7](../../commit/9f8c9a7caf1fc763bcee630fc75f21373da4b8ee)
- Judge Conversion Button on Evaluation screen - [7544862](../../commit/7544862422db1585a1685ebfcc1ff8e8863d8ee2) [6d817de](../../commit/6d817ded4cf54163417e39e9d1f385b9c08f0958) [c2f6311](../../commit/c2f631136befdfa9c910aeb1ae4679631273f7d6)
- Early unreleased Etterna Multiplayer Protocol - [d54fb47](../../commit/d54fb4798ac78b106ba0315215912fdd8e338c3b) [3dcd1bd](../../commit/3dcd1bdd7fec749af3736d7f96ffd721c5c01d54)
### Changed
- Calculating SSRs is specific instead of calculating every single SSR for no reason - [de405af](../../commit/de405afb373bbe9f77d7937d8eea560e6c37cb46)
- Profile Tab properly shows nothing for Steps of invalid entries - [253d009](../../commit/253d009608db207c708e34351272dc98686d0ef1)
- Use std functionality instead of OpenMP for calculating Non Empty Row Vectors - [5a1a217](../../commit/5a1a21725cf0163b38de70e3c2f20ac8c0cb9e0a)
- Rates up to 3x are supported by TopSSRs - [f26235e](../../commit/f26235e7f6d1988ea2c59f67101efc4d14232ce8) 
- Optimize offset plot loading - [fce2e49](../../commit/fce2e494f57ca960382ab0ca2e9aa313d3d57fd1) [a83ab16](../../commit/a83ab166e484e72a079e20c7b72305d7222c56bd)
### Removed
- Old SM5 installer icons [b921cc3](../../commit/b921cc302deb58d8884d7bd6fa68c0e50a1bc427) [1c408a3](../../commit/1c408a375c99b1552325f7437184d11dcb4f5601)
- OpenMP - [5a1a217](../../commit/5a1a21725cf0163b38de70e3c2f20ac8c0cb9e0a)
- Theme Version Display - [35c8889](../../commit/35c8889a5707d0e11f409be2b2bf7867646256c0)
### Fixed
- Scrolling while opening the Search Tab is sticky - [8b45309](../../commit/8b453098175fae6ffac1365a771b8a414c7e2d67)
- Filter tab elements can be clicked in impossible ways - [7747210](../../commit/7747210461cee54d424441d13e9150f9d983541a)


## [0.54.0] - 2017-01-21 - Pre-release 2: Replay Data Saving

Windows installer pre-release.

### Added
- Filter Tab for Skillset, Length & Rate Filtering - [d614537](../../commit/d6145377c585efe19ef9635d1f6fbf8671afbfa0) [17520b4](../../commit/17520b47b14167e01655473636720d35c02275be) [2ae00cf](../../commit/2ae00cff074f427ff3a9b127f7663a4bfe899993) [76767bf](../../commit/76767bffde825e17d98bb8985a304f3bc0f5c896) [14fd529](../../commit/14fd529a8351c255969d67f2049564da3194f46a) [41cb039](../../commit/41cb039af168ac87f06325f690ce6d427bde0199) [9686b5e](../../commit/9686b5ee36b0a0cdc8044e49b8b4a698ec80d5bb) [af0ece8](../../commit/af0ece8b118484610888b62059b3f97bb5a1c885) [b53b286](../../commit/b53b286f5ed522a07e30ec467645d79df198472c) [42296ef](../../commit/42296ef8f8c40c3e593050c10bf01749a4ccf19f) [fcb31fc](../../commit/fcb31fc6ef6031571df505cc1c79202e3b203cd2) [43f9411](../../commit/43f9411625f874334d6dcc6bd4a7d5119787d192) [56c151e](../../commit/56c151ef2f8c613d3b6f57068143f102bf13bf1d) [502c56c](../../commit/502c56ccb6bd04119e2dfb6cabc87c0bc6dd158d) [8ed006e](../../commit/8ed006ed24b37e1d8bc4fe591b86a31654d4a4ab) [8516861](../../commit/85168610db443754082b667e80b5c675a62bee4b)
- Proper top 3 Skillset Display for songs - [a5ab25f](../../commit/a5ab25f129701ce17d209b2115ef7ff9d467a129)
- Preference for not being forced to reset video parameters when a new video device is detected - [332bc78](../../commit/332bc785cc59627dbdc48af9b506039f9ca178d3)
- Hollow DivideByZero Noteskin variant - [930e2f7](../../commit/930e2f7f261ec1031f126c6b13c2b91f4be12b2c)
- Replay Data Writing & Saving - [c348b88](../../commit/c348b88c3cc3036460ecf4c0cd276a071fb6eadd) [1762008](../../commit/1762008d90eda7bbea077f42724304a1292dce56)
### Changed
- Renamed the bar skin to SubtractByZero - [012fb8d](../../commit/012fb8d05fb5871eb4e82941de0bf4977d64e318)
- Changes to the Multiplayer visuals & functionality occur in line with offline changes - [b686dc5](../../commit/b686dc50de2b931d9514f4f1b9fce4ce743c1a38) [55828f7](../../commit/55828f7f540749b4307ee8e5908485b65b151718) [abff056](../../commit/abff05620ef3fd02ee5b000c6fa146caccdd699e) [eda999e](../../commit/eda999ea9a746ea105b371a4944bd132226ccbe2) [30c0d5f](../../commit/30c0d5f6da2b64ca82071c65a3a80e14cbc63add)
- LoadBackground on Sprites does not work as long as Tab is held - [176fa6d](../../commit/176fa6d9fd9c74df7b6b88f82d580967a28377bb)
- C++ handles index shifting for GetMSD calls instead of Lua - [26a2778](../../commit/26a2778933bdc66d02d3090dbd05d215e58b0dbc) [d974464](../../commit/d97446423595a76e38308830650f5a75549aa580) [070323c](../../commit/070323c3753ca834076b813831402757b532ade2)
- The uninstaller does not delete the Theme - [49aa617](../../commit/49aa617ce6e16c868a9b07a97afe4ec0434dcfc5)
- SSR rank lists reload after Evaluation screen properly - [72dd7a1](../../commit/72dd7a1b782a67c192dd4bda3693c9f055a07b14)
- Profile tab page number & visual update - [f03dfe2](../../commit/f03dfe26425f06c99c05667b97470a4dbed5a191) [118833c](../../commit/118833cbd8e37e4eabad3b863adfadfee7ff772d)
- MSD tab average Notes Per Second scales with rate - [d51f154](../../commit/d51f154ac8aec90fe3f564b24bf8a618dce4f80e)
- Optimized DivideByZero Noteskins by falling back to the original & remove some tap explosions - [e008a9b](../../commit/e008a9b65f1bf4ffbd307e45ec4e76a1337aae40)
- Optimized DivideByZero Noteskin by setting AnimationLength to 0 (for FPS) - [142c8d5](../../commit/142c8d5e9e91f632be04c207c624923ac2aa5fb7)
- Improve loading of the Profile tab for the first time - [34dbd39](../../commit/34dbd393e211709f78cfebf0c333608ad8d047ec) [a1f7923](../../commit/a1f7923a5f8a458784307937014182cb246494e3)
### Fixed
- GetHighestSkillsetAllSteps fails due to a typo - [bcae636](../../commit/bcae63699c015c0d8fb87427b66ff2ad5e946da4)
- Mini Player Option loads wrong because of the load order - [7092e5e](../../commit/7092e5ed69e5424af5b3ea2478a4ef8cea92bd9c)
- Background Type Player Preference doesn't persist - [f022192](../../commit/f022192714c6456c27bb6e23e5cefa4bb80d8046)
- Lane Cover BPM display sometimes has weird floating point display errors - [5e40bf5](../../commit/5e40bf54c8ac5ee8cb752db822f822fe66d7d32a)
- Favorites don't show up correctly - [5a1ca79](../../commit/5a1ca79beeb33889d41e3cd6f14b43ae7d681052)
- Obscure crashes when viewing profile due to invalid SSR values - [28a4129](../../commit/28a41294d1a4c151ff247bfa669f398b8ec199c0)
- Garbage information outputting from HighScore lists after new HighScores are made - [3f1b927](../../commit/3f1b927126ba262d3114a569b3c3368c32f12f03) [49433af](../../commit/49433aff6d70cbfcffb25705e508269b06b220b4)
- Crashing on rates outside 0.7x to 2.0x - [f8e746e](../../commit/f8e746e071a7f0a191b2a16678823cfe05fe4b81)
- Favorites can be duplicated - [4889947](../../commit/4889947a20dd6fa57ee8278b671d153b88f53129) [ae2a9e5](../../commit/ae2a9e5f7b5fe6e7a72e53a09cf803fb0286aae2)
- Halved & Semihalved DivideByZero Noteskins have pixel artifacting - [032564d](../../commit/032564df43d9d6a97432765dd9d602e0f306dd9a)


## [0.54.0] - 2017-01-12 - Pre-release: More Specific Skillsets

Windows installer pre-release.

### Added
- Skillset categories are split into more specific ones
  - New Technical Skillset - [1253f09](../../commit/1253f094c08fc77cd2422b1600c53e737c2e73c5) [58bb603](../../commit/58bb603b0dde34184f70378acb21a2531147aeec) [f2b8a6c](../../commit/f2b8a6c99c2574941f92724d89c6c5fd907b4a09) [57390b6](../../commit/57390b62ab89b901e627467201412e43aabdcefd) [86ebd28](../../commit/86ebd288474aa49d758159f0043679354d674283)
  - Jack is now JackSpeed & JackStamina - [854dc2a](../../commit/854dc2a418691b8b10ce773d86d51fed72fff917)
  - Speed is now Stream, Jumpstream & Handstream - [854dc2a](../../commit/854dc2a418691b8b10ce773d86d51fed72fff917)
- Song rates of 0.05x are allowed - [71ada86](../../commit/71ada86739b8cf215fac46c744d279664b9a8b5c)
- Halved version of the DivideByZero Noteskin - [c6bd0cc](../../commit/c6bd0cc4202666535898088098845b0c18c81dd4)
- Semihalved version of the DivideByZero Noteskin - [7e9ed57](../../commit/7e9ed571202008f586ef4f97c99aea3a22d9deda)
- Orb version of the DivideByZero Noteskin - [44a37a7](../../commit/44a37a7a511e637e51cab60ab42bc572285a52bd) [44b90bc](../../commit/44b90bc685cbf7acc9131bb344e2ad40b73b2076)
- Bar version of the DivideByZero Noteskin - [44b90bc](../../commit/44b90bc685cbf7acc9131bb344e2ad40b73b2076)
- A version of DivideByZero which supports 192nd coloring - [f2a7449](../../commit/f2a744948c056d4d0f80ff67f0664d682c06c280)
- Profile Page shows top scores of each Skillset - [75f5d50](../../commit/75f5d50ad8245abfafa64babf7b41e465550ac67) [7238dc7](../../commit/7238dc7571144f7feab2ff0a87e553b9b5e816c2) [f0468f5](../../commit/f0468f599163de5039b6ce6f8bfe2bb90a753c3e) [3ce8baa](../../commit/3ce8baac632f4b5cbc556b9cc9f9e8d572256bc5)
- Preference for Horizontal Scaling of NoteField - [c16d57d](../../commit/c16d57dc784fcb198cb0f35e13ab0a0e8fc79065)
- Invalidating scores from ingame via the Profile tab - [0a364c1](../../commit/0a364c14d52b7863ded223c924a487bbb72d4c2d)
- Appveyor & Travis - [a73fa11](../../commit/a73fa1109c0f597d95b1e48d1cb2088d80d0b387)
- Tangent Tectonics by Sinewave as a new "official" Etterna song file - [df8b6dd](../../commit/df8b6dde151e05f2cdcca43db7c317f0255b31bf) 
### Changed
- Many MSD calc updates - [c8a7c79](../../commit/c8a7c79563a526805b81ab6df52d22b6db80aca2) [d3b6c5d](../../commit/d3b6c5db3933975b78e65728a0ea41a30f943e83) [e0bbddd](../../commit/e0bbddd3c0ca0ecd8e7f8792eb1c9a27a5e51325)
- Heavily improved Multiplayer visuals and playability, to be brought more in line with offline play - [fe2bd47](../../commit/fe2bd473784f50e32389d6b17d1deaf76b1146d8) [0bc44b5](../../commit/0bc44b5d06dd8d41fabfa1677c25e143a4b19642) [aa10d39](../../commit/aa10d39c792254989a868c3a95bbb3e2ab490a09) [8d12f9c](../../commit/8d12f9ccfc8d530542556b649c553bff0e192594) [9360c18](../../commit/9360c188c8974c10dfc4a3a2e60eddd83e3aed1e) [11e4ff0](../../commit/11e4ff0eabde24a97249151642bef3452b5e58a0) [a0b2355](../../commit/a0b23551889de73d11e0e3b183022f0dde0a0e33) [eaca47a](../../commit/eaca47a2220882246f562027c0472405dfc848cf) [aa8383e](../../commit/aa8383e170a4419262306613b42f0a2fb9501394) [3c12c85](../../commit/3c12c855b16482f92ff80d308d781bed3741174f) [431ef6b](../../commit/431ef6b4028a6e5a106c35a8b5cc3cf6868bf8db) [ffe1856](../../commit/ffe18561815b2a8201a7711778ce7b51e3077fd0) [545f29a](../../commit/545f29a550503d9a185cb98a4db44aa498b700bb) [753b2bd](../../commit/753b2bd50591e37c58151d61e6c3af6611fab80b)
- MSD interpolates for non-standard rates (non 0.1x intervals) - [44a80fe](../../commit/44a80fe662160af2724f9dbce68a3afb82548fe6)
- Note Per Second vector generator re-enabled - [fc51138](../../commit/fc5113849066669dca22a64268300c58b10656e8)
- Skillsets are more generalized internally - [a625318](../../commit/a625318b6b61a805482ba57379abb9e7a10505a7) [df526ed](../../commit/df526ed9399504fbacfb841df0e037fea80203c3)
- Lua has access to Wife & Wife is more hardcoded - [5ee64fa](../../commit/5ee64fa06b08393f24bb9623f7f6c4b0618d991b) [45f497e](../../commit/45f497ea176dfe3717bac7b1c197475e67631ede)
- SSRs are not calculated for scores under 0.1% - [6f83788](../../commit/6f837887594bead76392018dc864922b4056f3c3)
- Player Options slightly reorganized to be chronologically ordered - [f0fe666](../../commit/f0fe666e249758f332e93eea47647e9551acc283)
- Basic cleanup of MusicWheel messages - [04afbb7](../../commit/04afbb766886e3c7342d6c5f1c91acce0ae10141)
- Improve cache space usage by the calc - [a7e0be4](../../commit/a7e0be41b1667e49d8e99817ade92519323d5cbd)
- Noteskin Player Option row is always 1 option at a time to prevent confusion - [a2e5646](../../commit/a2e5646fd7cf5d8b814f2e98e0c66777151aff05)
### Removed
- IRC Notifications from Travis - [d9bd3f3](../../commit/d9bd3f3b01f76da959381011fbc7df83c6453d7e)
- IRC Notifications from Appveyor - [7c11e2b](../../commit/7c11e2b4588a486e076b26c14bbd2f62c85448a3)
### Fixed
- Window icon is giant - [dead101](../../commit/dead101d49beba0158d6b3740191585a4b55bd3f)
- Crashing when moving to a negative location on the Profile page - [4722f33](../../commit/4722f33e95eb4fe2d6a2ead4996909964d3aa87b)
- Bad parentheses placement for 1x and 2x rates on Scores - [15c4faa](../../commit/15c4faa2bdab741279ec52cece45e4cec56bf3d1)
- GCC compile error due to non-const rvalue - [ba1e734](../../commit/ba1e734f628e97f080f6b8b2bfc23496225d1473)
- Non-dance-single scores sometimes give weird SSR values - [587b55e](../../commit/587b55e9a55cec51efa09698ea17fa1431612c6a)
- Lifts are not considered at all by the calc - [0785474](../../commit/0785474a34a784492dc9bff39eed14c0f3068684)
- Selecting a song from Lua doesn't select a song - [ac955d7](../../commit/ac955d7acc1b49bfd6bfa68d6964fbd8df476df4)


## [0.53.1] - 2016-12-25 - Hotfix

Windows installer release.

### Fixed
- Crashing due to null pointer exceptions on Steps objects in the Evaluation screen - [081c546](../../commit/081c5460a3c6ff83bb0b8dd661f71150be7da1fe)


## [0.53.0] - 2016-12-24 - Skillset Ratings

Windows installer release.

### Added
- Skillset Specific Difficulties & Ratings - [39559e0](../../commit/39559e0e3a5ddae60efbf7e7af09325fcefca66f) [d4f2b04](../../commit/d4f2b040890f244f00d0f9a5f61d3880586df27c) [3551116](../../commit/3551116ac4ab3ba5c11d0041f8215cfb1322cf75) [104edea](../../commit/104edea8e011512d5800c78374d254ca160d7097) 
- "Worst Case" DP to Wife conversion - [07be19f](../../commit/07be19fdba343777227d78800a97a1fd3c4cb7b7) [d8520f0](../../commit/d8520f02a4ce0b497a01d0eb48c31ed5dcba75ce) [5343373](../../commit/53433736176ac10df6cd9305feae4809addf025b)
- Number of Favorites placed in bottom right corner - [630e753](../../commit/630e753b8724a223ea384f7fd5afd657488b007a)
- Session Timer - [81b47c3](../../commit/81b47c3bdde437f181c7b4b1a5132629f20231dd) [2b6f8ee](../../commit/2b6f8eed8cf4b604c2a5b74ea2b93080e3ff03d5)
- Colors based on MSD - [f712c4f](../../commit/f712c4f74be55251d960eb676b33fc235d626488)
- Colors based on file length - [e87a356](../../commit/e87a356304bf8003335f8d6663a5e291eaea69dd)
- Toasty - [8305648](../../commit/8305648f26b6c27183e8d3d8852327b4ba969fe2)
### Changed
- New Loading Splash & Window Icon - [2352dac](../../commit/2352dac6cd31bac18ef84b562f9e5aa4292ec0ec) [9701017](../../commit/9701017fde218493a918e0a9ca8cdc28326dadd2)
- Hold Enter to Give Up displays 0.2 seconds slower - [9f86da0](../../commit/9f86da0e2a9d71f758f19854a4d05c7e2c2caa60)
- Sort HighScores by rate forcibly when grabbing them for Lua - [568ba80](../../commit/568ba8058296164e1d99df023a8dd27fd8b3ebc7)
- Invalid scores should not be best scores - [8f66d61](../../commit/8f66d61c7932b9b11e19ff1c93444a474487f4a2)
- Centered Player 1 preference will move Gameplay elements - [cf1ea4c](../../commit/cf1ea4c524d2bc5ecc08193abb6f22bd5505968e)
- Don't delay sample music playing - [bf8419a](../../commit/bf8419ab38903064af50a722ed2c1588e263fa68)
- Target Tracker properly interacts with set percents or personal bests - [9d65ca9](../../commit/9d65ca98373fcabac76b329f70829cbd708e7d17) [5a8ad7a](../../commit/5a8ad7a31582edea0cfbd4c4fdf03e83445f6781)
- BeatToNoteRow rounds again instead of typecasting - [f719a5a](../../commit/f719a5a4c0e43beee15bf768e843def338699e4f)
- SSR recalculation is only done to loaded songs - [bb2d5a5](../../commit/bb2d5a55879ad085c1e357dc940e1d970a0cac0f)
- SSR recalculation is handled in a better way to allow less hardcoding for recalculating - [d8a337d](../../commit/d8a337d17df707d9803f942fc1523b0667733dc9)
- Autoplay scores intentionally bad according to Wife - [a4ef6b6](../../commit/a4ef6b6eecf8a11c6a7031389040e3fbc03da883)
- Color Config allows MenuDirection input - [03f8947](../../commit/03f8947ebf02ab4bfafb35eafd1ec972ac60bbeb)
### Removed
- Text Optimization when not on D3D - [22b940b](../../commit/22b940b3e42c2531985a9c75ee93e41f4572a7ac)
- Various unused Lua Scripts files - [705612a](../../commit/705612ab9790961203b5d45bf72858111dca9f10) [afca92e](../../commit/afca92e9fa05884aff6db962edce941ad7cc1ea9)
### Fixed
- D3D Render States not working - [0394d35](../../commit/0394d35997365267f0a6055c38ac144706f2a1a6)
- Wife & DP rescorer not working on Score Tab - [2d0174e](../../commit/2d0174e54626145ea01ae8ee463f7571727fa7ba)
- OpenGL/D3D Text Optimization & Rendering Issue - [59c2b7b](../../commit/59c2b7b2100ce68f0ee1524a9f6495257660d827)
- Edit Mode crashing from Hold judgments - [294b1cb](../../commit/294b1cbffcc3353cefad3f2314f70b7e01e7c406)
- Display BPMs not showing proper BPM when on rates - [1dddd9c](../../commit/1dddd9c1ba8ae851b253dc50d863fb51f861f9fa)
- Multiplayer is visually broken. Redo almost everything - [0e3fa51](../../commit/0e3fa51e142bbc273c56fcebb7b484b8980cf742) [0e3fa51](../../commit/0e3fa51e142bbc273c56fcebb7b484b8980cf742) [2be527c](../../commit/2be527cd570f6338fa0724d095964c67d0b54b38) [6d4a71b](../../commit/6d4a71b12aed7457b78b534c70866f4052f1cfe2) [d21a97d](../../commit/d21a97daf290b9fc593dd726d3b1dd25a01d81d5) [ee807aa](../../commit/ee807aa4084e5afdc12bbb718fc92c143ef5bb8f)
- Some Evaluation elements are badly placed in widescreen - [845afd7](../../commit/845afd7216c3283e835b9c49c2312f50109c06c8)
- Bad Progress Bar placement by theme options - [b21018c](../../commit/b21018ca372123466741a69b10c0f2dbae8fc9e1)
- Background reloads when switching theme tabs - [321092d](../../commit/321092d8b024cf3dd800edcc5951b1af3d6ff3d7)
- Delayed texture deletion delays infinitely - [6ac403d](../../commit/6ac403d642262e49e7a46c8e3877f11116cae05a)


## [0.52.0] - 2016-12-14 - Song Favoriting

Windows installer release.

### Added
- Judge Conversion & Chartkey History - [80c7ceb](../../commit/80c7cebd529cfd502d78f6a182b7991f5ebe5bda)
- Judgment Cutoff Lines on Offset Plots - [39e5fa8](../../commit/39e5fa8a3e5ea6a00af686fa2233143e46398efb)
- Song Favoriting - [7f6f053](../../commit/7f6f05337ca42f8874e5397631eb64b79714bf8a) [717e175](../../commit/717e175a40bc365f415bd12dce2703c0e679ef9f)
- Sorting by Favorites - [428bf72](../../commit/428bf7222341e56418b2dab3534292c073006d43)
- D3D Render Target support - [a367c05](../../commit/a367c05eb67b3975c7bcd8813fd79740b72a6ae8) [577845e](../../commit/577845efca1bf9b0d4d4409e0852cc0b3f3b71cd) [ed45d5b](../../commit/ed45d5b439f81822863df53f66c7e19604db84b4) [f1cd28b](../../commit/f1cd28b5c09e0b42864bbdcbca2ac710f33982c6) [0b7e06d](../../commit/0b7e06dbbecb6e1bc1744bfadc8c0c17959bf2c1) [b3efd1d](../../commit/b3efd1d0778706f3c0946720c7c74083a51c2a9d)
- Target Tracker can be set to Personal Bests & Target Percents - [cd890f5](../../commit/cd890f57a090fbe8321a34860cf076f60886e541)
- SongUtil function for getting Simfile Authors - [43d3392](../../commit/43d3392d86f56be2e10957872645cd0c47d3e7d1)
- Song Search can now search by Artist & Author - [4599f9f](../../commit/4599f9f6032e9937a91d11403750558af1f6a66c)
### Changed
- More typecasts change to static casts - [75354e8](../../commit/75354e8581acc57f6583e5922c37708930063d1b)
- Don't reset search strings when entering Song Search. Press Delete instead. - [8b1acaf](../../commit/8b1acafee7aeb62a13895e63871e581bdc1c510e)
- Moved Wife2 function to RageUtil from Player - [868f329](../../commit/868f3291638d394ae457679f5e41e900ff45a067)
- Clean up DivideByZero Noteskin Rolls - [d9bbd95](../../commit/d9bbd9523dde2910efec5ebf121cd307030e72c2)
- Improve text character rendering - [e2041b0](../../commit/E2041B0478d766c2d7fed4ce08c5213bee56b990) [a04b891](../../commit/a04b891677012fa412296641f5155cc89e9f5977) [35d7c53](../../commit/35d7c5336722c853825d3bb055106bb577d8f736)
- Music display shows file length, not audio length - [b696378](../../commit/b69637866178d928d7ef9b7cd8c8892708a90c40)
- FailOff/FailAtEnd will not produce SSRs - [8737a14](../../commit/8737a1442ded55ff76d220c17b943fdb1f3aa852)
- MSD Calc updates. - [01d58ad](../../commit/01d58ad0c7616186e249ef9a1e9e37251e280be3)
- ArrowVortex is mentioned ingame as a better editor in the Edit menus - [149945c](../../commit/149945c77b5ca89e0c49c54ee79b95c8508b39a2)
### Removed
- Writing old Groove Radar data for HighScores - [e04f7e0](../../commit/e04f7e0d5cb0f257b5f7099c2b22d840799c51af)
### Fixed
- Bad decimal placement for Wife Percent in various places - [f29df05](../../commit/f29df050bf4b3cd02442f5a21ad30b284679542b) [d45e482](../../commit/D45E482924f6d81a558e764eabaad358d0d61ab0)
- Failure to compile on Linux due to some type issues - [eb76405](../../commit/eb7640534de58d4226d1e4cc1b8eb4cb2059a21c)
- DWI files crash on first load - [1b2294c](../../commit/1b2294cb4cfdaca3e6be169ed58f9405e9fad32b)
- Edit Mode playback crash - [f1e4b28](../../commit/f1e4b280739a27cb0af624a3f088af64a481d2ee)

## [0.51.0] - 2016-12-07 - Song Search

Windows installer release.

### Added 
- Song Search - [4795319](../../commit/4795319b309d8af2370ce6fbf7afe81a3bd57bae) [4f64c6b](../../commit/4f64c6bbb84278c331c2d0bb1f400288ed56fbc5) [5a65b28](../../commit/5a65b28338a3782b96726963eac41f06d8b6f5a5)
- Player SSR (Score Specific Rating) recalculator functions - [a1f4df8](../../commit/a1f4df86aa00dc9d5f57f96b0873de741e212aaf) [8edfc38](../../commit/8edfc3824cf40031f130a9a1fe649ad75bc9c805) [1a0339b](../../commit/1a0339b8a69c4dba45b164deff3fa980f0e0a7ea) [1741687](../../commit/1741687ee9834bf98fdcde9c9b0a562eb7cfada6)
- DivideByZero Noteskin Rolls - [6f19852](../../commit/6f198526182c78ac6f6c8cce55d276e55b91a86f)
- Grades based on Wife Percent - [e97836d](../../commit/e97836da3a71ae3636669a66058e330f9ac3c535)
- Song rates up to 2.5x - [9543b49](../../commiT/9543B499774d672f038ff1666e2a28e30cb29a54)
### Changed
- More typecasts changed to static casts - [772c359](../../commit/772c35957ecefd57c3faa72b55e7ff33cdda9ef6)
- Installer won't overwrite preexisting files & won't delete the song cache - [ad5b0ff](../../commit/ad5b0ff0f725577f89a8ed31e3b347789eef4b82)
- Max player name can be 64 characters instead of 32 - [a1f4df8](../../commit/a1f4df86aa00dc9d5f57f96B0873DE741e212aaf)
- Use the floor of Wifescore values when displaying them - [0028bd2](../../commit/0028bd23d92cea8518c6ea29f37422d1b1ea53c5)
### Removed
- Default SM5 theme & Default SM5 Noteskins - [182ea03](../../commit/182ea03cb472940c8b61b4f7303ac0da88b81323)
- SM5 Song Folder - [56609bc](../../commit/56609bc33cae0c53e98ae25ec65725625e2d9ffd)
- 90% Wife Score requirement for SSR calculations - [65a620d](../../commit/65a620d6e981aeb77963562cbd51137fbea6126e)
### Fixed
- MaxHighScoreList preference not properly ignored - [271a4be](../../commit/271a4bea60f3ee4116a4287e922020c0de6e2bbd)
- Bad default scroll speed increment adjustments - [7e399fb](../../commit/7e399fbe3d793b4bf7a2796e3d989d9029fa4c2e)
- Lag when modifying Lane Cover - [db03a65](../../commit/db03A65108D1649697887815f48cf04b76469e10)
- Bad layering for Judgment Counter & Display Percent - [9191d54](../../commit/9191d54d8fc89ec0689cf4064213fe0dd2a35d7e)


## [0.50.0] - 2016-12-04 - Public Beta Release - MSD & Wife

Windows installer release meant to be installed in its own clean directory.

### Added
- Installer will install VS2015 C++ x86 redistros & portable.ini - [cc8add3](../../commit/cc8add3af0041be3bca33124b2975757fa8fe103) [5a5eec8](../../commit/5a5eec8681705e4d3ecc6b201190b31526fe0808)
- Uninstaller will uninstall all proper files - [5abe56f](../../commit/5abe56f64b7d07328f23aae5f1e61bbdce535fe3)
- Predictive Frame Limiter - [8fbd324](../../commit/8fbd32486587ae5bfaf7e9386a8ba5c81fb5d45f) [b27d673](../../commit/b27d673159969efbe94ea4a4540c0f5e42886b0c) [3d0f895](../../commit/3d0f8955f419d9dce968aff57d6d588a27e57201) [4a1dca9](../../commit/4a1dca90bd321c25367b2f2ed0957ccb0fe4774b) [edbb2df](../../commit/edbb2df59893a942cd9d76f6935c4e9edd03fda9) [a77e8b7](../../commit/a77e8b7af40aaf12f3f573d77e122ac0c96375dc)
- Millisecond Scoring system (MSS & Wife) - [0e02141](../../commit/0e0214155a23570f23bd9a68332dd6f7d30aaf11) [0c1eaf3](../../commit/0c1eaf311c05fcf7e1498f691386df1a505c7aa8) [bc26e1d](../../commit/bc26e1dd2ccda6a317ff9696228676238fe2af7e) [92f3398](../../commit/92f33981e6449141bf07b88bdf5990e11a12ae6f) [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9)
- Mina Standardized Difficulty (MSD) - [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9) [065aef4](../../commit/065aef43d98e8dcf3738028b3d533e756af159ed)
- Preference for Allowed Lag in milliseconds - [87a8085](../../commit/87a80858e21969ea52bcda603503cd29b3876a28)
- SongFinished message for Fail Grades - [70a432f](../../commit/70a432f58be30ff2a02d786d6141dc8d038f5766)
- Polling on X11 Input Thread - [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9)
- The Til Death theme & some optimized Noteskins are now shipped with Etterna - [bd9ad63](../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
- Some new songs come shipped with Etterna - [bd9ad63](../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
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
- Unlocked Gameplay FrameLimiter froze the game sometimes - [2b4d67e](../../commit/2b4d67e0c075f6f4a54247602d020a06891f03dc)
- Edit Mode crashing on Judgments - [b5be683](../../commit/b5be683a86fa17b192d261357892c6d28a3648ad)



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
