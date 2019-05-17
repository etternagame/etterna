# Changelog
All releases of Etterna are listed in this file as well as all of the major changes for each. All changes for each version apply in supplement to the ones below it. Changes are not in chronological order, only versions are.

## [0.65.1] - 2019-03-10 - Quality of Life Patch

Windows x64, Windows i386, and Mac installer release. Linux tar available.

### Added
- Combo Text Coloring in the Color Config - [e33e1c5](../../commit/e33e1c57e6975b5312149e49cfc39e2254f74669)
- Lua is notified of Options Screen Exit - [2abb7e1](../../commit/2abb7e191da7cc0794f74933efaebc20bf0be1c2) [5a45f29](../../commit/5a45f292a956e4c895f4d84af1065d72b43e3a8b)
### Changed
- Calculate the name positioning on the userlist with more correctness - [7699738](../../commit/7699738b1d5b3f89059d034782ce01d09ba75686)
- Common Pack Filter Option should persist for the whole session - [531cd21](../../commit/531cd2140a8ab33786df997c5453336405673613)
- Don't need to parse for comments being inside of value fields in simfiles - [f372151](../../commit/f3721519b11a0fe66ca043015ffce7ea7630c645)
- Forcibly log out of Multiplayer when entering the Title Screen if logged in due to tough branching logic - [ec40e66](../../commit/ec40e66e3291f3a9f0f49d9ecebbdfad21a3dd59)
- Lower bound for Roll Life should be set to Judge 7's window since anything harder is basically impossible - [49457ca](../../commit/49457ca78f72556b43b895326a9994d42a893e2e)
- Metrics-generated Multiplayer User List should not be visible in Til Death - [0383411](../../commit/0383411907d7087297bfab27c8ceaae4b3cf39ae)
- Multiplayer should not start extremely quick when on rates. Make it all consistent - [11ee54b](../../commit/11ee54b7246894955cb2f5b7cc13d0bee1f80879)
- Only delete the installed Noteskins on uninstall/reinstall - [5dbb444](../../commit/5dbb44479629cce10ce46c34bd199fbfbf79cf35)
- Optimize & Improve Practice Mode
  - Cursor should appear in Practice Mode - [6932fce](../../commit/6932fcea895f514a68b5689cb5e18a3117a48155)
  - Don't reload the NoteField when moving - [8cf63f5](../../commit/8cf63f5ccfed4ca8b8ab0fd317ae2d6762e70abc)
  - Don't send Judgment Messages for Misses when seeking to prevent tween overflows - [dc08985](../../commit/dc0898537a4a7e7b96fed6631c49a6630de629d3)
  - Resize the Chord Density Graph properly - [cb849e5](../../commit/cb849e5d921f1930802703a723d5516643c470b1)
  - Update relevant numbers when changing rates (for Replays too) - [679e671](../../commit/679e6710017a029ce0fcd067b1111dcc8327372a)
- Reduce the number of builds on the master branch on Appveyor - [d6736a6](../../commit/d6736a64607fbaaffb736124a75f5371389afbfb) [945acc0](../../commit/945acc0279524de1734a54e129fa7f165208eff2)
### Removed
- Arcade Options and the last Preference on that Screen - [92819b3](../../commit/92819b3d91d408507c32a627e686a25b2af768ad)
- Last Quirks Mode Reference - [2849f88](../../commit/2849f88502af6bb13478bcd3d5e3a9c0e36ed68c)
- LibPNG to be replaced by stb_image_write - [3047db4](../../commit/3047db410bed28c2731848905025a72574ae7fb1)
- Useless Lua input callbacks - [b4f5c61](../../commit/b4f5c6164de17d228cb6370e2026ea0671e45b7f)
### Fixed
- Changing Judge in the Options didn't update on the frame - [5a45f29](../../commit/5a45f292a956e4c895f4d84af1065d72b43e3a8b)
- Combo Breaks per hand counter on Evaluation Screen didn't work beyond 4 keys - [cf0c5e7](../../commit/cf0c5e7123df17545624de3a039e2a147bd6ce01)
- ErrorBar movement didn't save - [ca99351](../../commit/ca99351345a467ad4f90c7cd3284d2d437434522)
- Exiting Network Options had a weird branching issue that led to the wrong Screen in some situations - [a64486d](../../commit/a64486dc8201bf263b7e513211115b4284768cec)
- Holds/Rolls in Practice Mode sometimes appeared covering the whole screen - [84c8eca](../../commit/84c8ecacd23a39e642c863ee8eaf5ff89253bc80)
- Invalid UTF-8 in Charts in Multiplayer crashed - [303bb5b](../../commit/303bb5b180f755a078dbc9dcecae05ddd683653d) [8101004](../../commit/8101004e5117fd3693bec84325abe1aa355dd962)
- Loading from Cache was extremely slow. Fix by doing the following things
  - Save full paths to all Song assets on load from disk - [993dc77](../../commit/993dc77936cb703d926bc24ff1c0a15806fd7c41) [1c7fa2d](../../commit/1c7fa2d0fac5f80373569526fdff11d75fdc4aee)
  - Don't generate all full paths every startup, use the values placed into Cache by the above change - [6a5ae27](../../commit/6a5ae27490db27983ef786d3bc996ca371860047)
  - Lyrics don't save right, fixed that - [a672e95](../../commit/a672e9587e9683b9f0975ad7195d8466fceeee0e)
  - Correctly load all assets on first load - [9bf1f46](../../commit/9bf1f467efd293a9d99a1fce2b7c01ae24b55518) [677a439](../../commit/677a439bcc2a8ebf367eeb7cd190054986806253) [43ae1a0](../../commit/43ae1a03f251070d624dc86a180d12c1215eb336) [262f314](../../commit/262f314d0e5564cdc463996e4166c3488aee2716) [2264db4](../../commit/2264db4b7ef209735ea6542573876896e6b47d6a)
- Lua errored when not in Multiplayer due to unchecked attempts to reload the userlist - [c121787](../../commit/c121787367f5201ec1f7a171cd9374a601d5635f)
- MusicWheel jiggled when picking songs - [5b1e8ec](../../commit/5b1e8ec9593112a7430302a9ea43192174ad1589)
- Playlists reset the NoteField Y position for no reason - [54c390b](../../commit/54c390b8ead4e9221c2e928031550051c25dd42b) [9fbb044](../../commit/9fbb044aeccbd826f59036066a7d4e16dc5f5ee3)
- Profile Options Screen crashed when you pressed anything other than exit - [b7e8469](../../commit/b7e84690d6625524d9d46359b1126b9767d035a8)
- Startup rarely crashed due to non threadsafe function - [c521f1c](../../commit/c521f1c3c5758043f1a02e654fc11c821c18e1e8)
### Note
- Temporary (reverted) change: MP3 Chart Preview Sync Fix Attempt - [4fc06ba](../../commit/4fc06ba4cd8de84e5b9e39ce8d7df80e4c096f7e) [62a9b78](../../commit/62a9b787caa5048508f54f4dcc84528fe8c31dd1)


## [0.65.0] - 2019-03-01 - Practice Mode

Windows x64 and Windows i386 installer release.

### Added
- Appearance & Hide modifiers will Save/Load - [7f0054f](../../commit/7f0054f6d5d0a6d045584f100d2eb2b8e1011cdc)
- Audio Visualizer Lua Script - [555d8d0](../../commit/555d8d073bf8b23fdd72918d1ab55bf05f062aae) [4893966](../../commit/4893966155d1f306c6a115bbbe3539c253ba02dd) [0be9670](../../commit/0be9670a7a302bf0358464c9df8dcfa1c81d8af4)
- Client version data & Pack list in ETTP Multiplayer connect packet - [5fb876e](../../commit/5fb876e557f4d41e0fc18b3582ee5736fec7cea3) [dddb438](../../commit/dddb438032e872579ce3dc05c2ede846ee629ffe)
- Combo Breaker Lane Highlighting in Gameplay - [1fafdad](../../commit/1fafdad3594c537c2595e28811b9c058d5193911) [be6e311](../../commit/be6e3119119ffc1ffc6ed0fa4f8a482087a9db9f) [c92ff3d](../../commit/c92ff3dc37dcf2237c7d2979b0738f0a6395c9cf) [5b9b2a1](../../commit/5b9b2a1eb8260690ca19236a96840602b4bfdf0a)
- Common Pack Filtering in Multiplayer - [e71d82a](../../commit/e71d82a70192fb242876673a31c8ac2b46fbd40f)
- Cursor Hiding in Windowed mode - [f95188d](../../commit/f95188d43a5e4dd1a21f8165186dbbe39144911b)
- FFTW (Fastest Fourier Transform in the West) Library for Audio Visualizer - [555d8d0](../../commit/555d8d073bf8b23fdd72918d1ab55bf05f062aae)
- Ingame access to the Pitch Rates Preference - [d99ff8f](../../commit/d99ff8f5a92e1a6721d6404d75a035d3de17de85)
- Lua access to see if a Song Group Banner exists - [9700b7e](../../commit/9700b7e84e3f84f5c5eabc14883f623f2bbaa2a3)
- Lua access to a table built of Nonempty Rows of NoteData - [c66b6dd](../../commit/c66b6ddb4b0167585c16992161963991d9a9f0bb)
- Minanyms - [9af6301](../../commit/9af6301cf9bba9b32841287b32c6e4b007462c09) [607c2d5](../../commit/607c2d5c63dad50bb349f27c5815c55dd4f21b89) [b612c27](../../commit/b612c27be108e2492c04ed3ad971b326afaf54b8)
- Mouse click to skip the Intro Splash - [903de6c](../../commit/903de6cdee3e1a8f052949aecd222eb2a08c2f27)
- Mouse hover on Wife Percent in Evaluation to increase precision - [ab21261](../../commit/ab2126154f09e26c5004eaec9c3e432983132ecc)
- Multiplayer chat timestamps - [e71d82a](../../commit/e71d82a70192fb242876673a31c8ac2b46fbd40f)
- Pack Banners are added to the secret headless generator - [f0a5b5e](../../commit/f0a5b5e07a5b5f4c010502326bb74c52b201cc42)
- Practice Mode with Bookmark, Seeking, Pausing, Rate Change - [dceede9](../../commit/dceede9e806e73e7772b35696fce89ab3992a3b9) [9748144](../../commit/9748144fd62878bb9699e95ad0ee3973789f48c7) [b99c4de](../../commit/b99c4dec99ac078962a73d31ee67706133b12f5e)
- Single Digit Combo Breaker ClearType - [befd19d](../../commit/befd19d94d85c1705429952289e4d4b94b7d9c03) [fe3b10d](../../commit/fe3b10d2a110461fc10d178a6cdf24bf223b628a) [3524b90](../../commit/3524b90fa6a9e23ff9a04d670488a202ee5827ed) [863a14f](../../commit/863a14f851d9aa1417b06ee9ce4dedf25056bd99)
- Song Pack Name is sent in the ETTP Multiplayer Song picked packet - [2b8306b](../../commit/2b8306b8967f1f09474fdca92b0e66314c17b2d3)
- stb_image will now always be used for images, and the opt in Preference is gone - [fb319da](../../commit/fb319dae8e092869521a6e25a18e49a63e9b32d5) [0e3b801](../../commit/0e3b8018b38c3b577832d61a3fc5cf56977d1632) [5a97473](../../commit/5a974733e7325db85dc8334e601ac658eacae75a) [bb1e1df](../../commit/bb1e1df266a4dc65f666cbb339458e3af24e1fa6)
- Til Death Audio Visualizer - [bae682a](../../commit/bae682aedfb3ec53281b8f048f1ccc1a8238d36e) [8408cb2](../../commit/8408cb298a5cd64b591e37a6949329a67e49f5b9) [0be9670](../../commit/0be9670a7a302bf0358464c9df8dcfa1c81d8af4) [9be3fa5](../../commit/9be3fa574e6d53f3aab5393511be3185ee6e4a67) [980cb3e](../../commit/980cb3ee4b15dcdc3692b5bebed9d71819269c6b) [ed514fc](../../commit/ed514fcd28920ea308d699dccaf7e6166fd51fd3) [e7bce8d](../../commit/e7bce8dc34ed2aabb1f25f828539cd0718ed95a6)
### Changed
- Actor attribute FirstUpdate is moved to ActorFrames - [ea60f7b](../../commit/ea60f7bd2b7520ca5969d081caead3f7ece1b03c)
- All Lua EnumChecks for PlayerNumber are hardcoded to Player 1 Enum - [5d68f5a](../../commit/5d68f5ad57fa9b7e7d663b399288ebb3a40f057a)
- Alt+Enter is disabled in Gameplay - [810b76d](../../commit/810b76d77d10bac8aabdbc6ca543607f25b965db)
- Autoplay should be automatically turned on when CustomizeGameplay is on - [9c7131d](../../commit/9c7131d01c2a48d11121ca376737e71377be635a)
- Bundle Pack Download Queues should use the mirror links - [49bcaa3](../../commit/49bcaa357d3e96f3c7a853efc3ceb05c5850d454)
- Default Multiplayer server is the main server - [be9a63c](../../commit/be9a63c310a9e95f45bb53a1895c694176085712) [160a076](../../commit/160a07686932ab4679b1d3fd9380b4e38e24e4e6)
- Dependency on static arrays of NUM_PLAYERS size everywhere in the game code is replaced by singular pointers - [1abc266](../../commit/1abc266591438db48646315aabe955dab244b9a3) [f5b6c0d](../../commit/f5b6c0d8d3950a03f0a7d46f4512c20f2154cfdd) [64963b7](../../commit/64963b7bed6c5d59994287bf06566f52b8a98f30)
- Gameplay NoteField updates are specific to Player 1 - [592ade2](../../commit/592ade28399b9c881601b9fb3b9dcb95399bf36e)
- Login/Logout button has a highlight - [df8b7bb](../../commit/df8b7bb55bb5b37cc7413fe1e2f7a433f4977223)
- Lua can't turn Replay mode off - [644d3b2](../../commit/644d3b22bc8874b9a08556f65d0ee5cb292bb6ff)
- Multiplayer Evaluation Scoreboard shows rate played - [0dfc498](../../commit/0dfc498950b2591735433e90a57e78f9696ea90a)
- Optimize recalculation of SSRs by collecting a list of only the Scores that need to be recalculated - [038815a](../../commit/038815ac38f5c449304df59ddb764c022a1d1a35)
- Optimization to coloring when drawing BitmapTexts - [3d2e4ab](../../commit/3d2e4abfdd0dcc73536c411d2a53f4974dc30e33)
- Player Join code is guaranteed to always be Player 1 to removed Player 2 issues - [884b457](../../commit/884b457bed49495d7b047ba4f04bf648f598762a)
- Refactor of experimental Lua MusicWheel - [555d8d0](../../commit/555d8d073bf8b23fdd72918d1ab55bf05f062aae) [8a22e03](../../commit/8a22e0393dc8ef397d70ed02e61d7f30b33a0c2b)
- Refactor of Gameplay Lua - [68f62af](../../commit/68f62afad5e4306881ec2861d11d4d2285eb3cbe) [b6db9fa](../../commit/b6db9fa8bcdc895b8011cdbb50540576732c95bc) [9dd4848](../../commit/9dd484848425bede6a2c06f64619ce084c231f2a) [a2c102d](../../commit/a2c102dce68b3919f3ab3989d8ba27bce92fb1c4) [b84ced4](../../commit/b84ced4e1a47fb9a753552b37c7d1478466e5a50) [ff79587](../../commit/ff7958782b314138296d83832e9930c493994483) [c4e0130](../../commit/c4e0130c6717279b957416142ea977ff979d71b7) [00584d9](../../commit/00584d9484e6971ea95744126cc54e6e81caf422) [75c28cb](../../commit/75c28cbc4ab6e381af7b99ebc88f8e208f8196b9)
- Revert a range loop in ScoreGoals to a regular loop - [2083d82](../../commit/2083d82098e8d71823fb8e7675646856d3db63eb) [40fdd62](../../commit/40fdd62c38417feb8c1a7d880e1dc0c226a31e7a)
- Song information is stored as std strings instead of RStrings - [e6106fe](../../commit/e6106fe384d0a7458fa2e6730978ae6b62abafaa) [77ecb13](../../commit/77ecb1304ef0d0e80e88c3ce431e9d53219381ca)
- Song Load has had many changes
  - All paths are defined as soon as possible - [33cd465](../../commit/33cd4650c768b0724ce4e0cba8f6866c8a9c9eb7)
  - Load from disk will store full paths correctly - [e6106fe](../../commit/e6106fe384d0a7458fa2e6730978ae6b62abafaa) [f88a497](../../commit/f88a4977c234c0172f375ecc8a8ae4279625a7b8) [6fe555e](../../commit/6fe555e782bb65dfb1478d86344e63467ad2f903)
   - Mac resource prefix stripping is moved - [40d7f62](../../commit/40d7f625b65667e48c825c59de8aa1303ef8c777)
   - Path generation for Song load is done always regardless of load from disk or not - [4dfcb35](../../commit/4dfcb35f5cd0de3fbd16077e5381cf6cc0947788) [a8deb38](../../commit/a8deb388ee9b55ce13f3b86e5a53dde4eaccf893)
- SongManager functions use std string Chartkeys instead of RStrings - [55a1a46](../../commit/55a1a464e078abaecaa4e1bd4e4b0fd6e82d20df)
- Song Title Sorting is changed to allow '#' - [715e02e](../../commit/715e02ec709718ea783215109399bad6352312af)
- TextBanner SetFromString uses std strings instead of RStrings - [ae71ed5](../../commit/ae71ed5c8e1b361485f1982f14815c27bc3fecad)
- Very slightly improve Chart Leaderboard force updating when entering SelectMusic - [f55e765](../../commit/f55e7654e1d5c13ba481a16a43e458bc17771419)
### Removed
- Actors stored internally in ScreenEvaluation except Combo/Life - [3b3b472](../../commit/3b3b4726fdcf287b0087f2897e420f56b349fa40)
- ActorMultiTexture, ActorProxy, Banner, BeginnerHelper, FadingBanner & PercentageDisplay - [d4af6df](../../commit/d4af6df212c1738a167b629677c60d866c3f1108)
- BackgroundLoader & RageTexturePreloader - [c431ee5](../../commit/c431ee50c0bc0555d84c63f3a9c4a014b420644d) [2033098](../../commit/2033098a8bf160ce474d362805028ef734170641)
- BeginnerHelper, ScoreDisplay Types, GradeDisplays, old GrooveRadar, ScreenGameplayLesson, ScreenGameplayShared, Tutorial & Attract Screens (basically everything a cabinet needs) - [a5a28e8](../../commit/a5a28e80afe4bff97b812848ddc72e1a4aae0d6c)
- Broken Chartkey fixer - [b7e6489](../../commit/b7e6489768324ec5e5066bef46d1ab58be1ebaeb) [03184d5](../../commit/03184d5a3e47466ca4b4a83362a6713afba91fc8)
- Combo Awards & Awards in Rave/Versus - [d3d323d](../../commit/d3d323d36a5a8dd07f6086d70ec60427444969e5)
- Couples & Routine - [8bee766](../../commit/8bee766acb473f34135f59e8029ef86d82852c59)
- Dead or useless Song related functions - [55c65ba](../../commit/55c65bae6bdec629a60f4ee13d7522ea036f3095)
- Deleting Songs - [fc88c33](../../commit/fc88c3369fbeddb25bde50d2d0048c464da5d68b) [22b9fd9](../../commit/22b9fd97267acb9d04d0222663825c7c9d435440)
- Duplicate inline Screen function to update timed Lua functions - [4ac9bce](../../commit/4ac9bce906fdb1b47f0114825fc07951414a47fd)
- DynamicActorScroller, FileDownload, GradeDisplay, HelpDisplay, PaneDisplay, ScoreDisplay (and derivatives), ScreenOptionsExportPackage, ScreenOptionsReviewWorkout, ScreenOptionsToggleSongs, ScreenPackages - [2033098](../../commit/2033098a8bf160ce474d362805028ef734170641)
- Editor Mode Noteskin - [87a1e94](../../commit/87a1e943d7429cbf3f4281eb9eecf40a3bffc190) [4ac03e7](../../commit/4ac03e71db503982d07fb4b45272da36c5232c62)
- ETTP NewScore ScreenMessages - [77bd018](../../commit/77bd0189cfa01c5f4290e3630a498748464323a9)
- Every single remaining reference to Rave/Endless/Nonstop/Oni - [b556b43](../../commit/b556b4314e13f473fabf3e927448d990d65d83ad)
- Heart Entry Theme references - [6e6d302](../../commit/6e6d30254a628b563cd0697e334c26c5b4ce90ec)
- LifeMeterTime & LifeMeterBattery - [4543702](../../commit/4543702f009e2dcdb704a1e415ba5a4ea0f2b537)
- Lua Scripts ThemeLibrary - [49e07ab](../../commit/49e07ab7af23abcae325fedc02e65fc321989d5c)
- MarginFunction for placing the NoteField in Gameplay - [49fafbb](../../commit/49fafbb2c3a14ded7acb8be1376c0e71f0aadab4) [746ab59](../../commit/746ab59b1baf2fe15c2afbbd1b3f4206a07385cc) [804f543](../../commit/804f54380a57b098f83ba7f6c9800bcfc85b4a4b)
- Multiplayer Evaluation dependence on checking HasStats for crash reasons - [e15c276](../../commit/e15c27644a0f6554da1b83a53a1ef077e58d7df1) [7890d5b](../../commit/7890d5b8472d548d9743b978fce7f55bde76a231)
- Old Stepmania Profile/Highscore Saving & Loading - [bdeb972](../../commit/bdeb972b56101fe8c4de99354bc04afa08fcd9b4)
- RageSurface_Load BMP, GIF, JPEG, PNG - [fb319da](../../commit/fb319dae8e092869521a6e25a18e49a63e9b32d5)
- Ranking categories for Versus Mode/Grades & StageAwards - [11335f9](../../commit/11335f9c4aa2abe76453bb67b86df7ac7be10feb)
- References to Player 2 everywhere & second controllers in GameManager - [cdb1ebe](../../commit/cdb1ebe7e62250a7c0a6eb51fa144e9dd028320e) [6525281](../../commit/65252818b8b663ed2f8c056a89b895a815f52196) [9bd7d64](../../commit/9bd7d6490c93a717ce98106471a8f045cbbd8fa7) [e440db2](../../commit/e440db27cd72eb758a79df902027c80488c098a7) [ebf8ca6](../../commit/ebf8ca63ec1602a2a8668b8d2d023b61c0c58124)
- References to Player 2 in the Theme - [ec0af44](../../commit/ec0af44d9a89c4f5be227c18c8a7ce8e541dc1f0)
- ScoreType Theme Preference for choosing between Wife & DP - [58517aa](../../commit/58517aa559628caa0e281fa3d9d2fe20341eeaed)
- Stats.xml to Etterna.xml converter - [bdeb972](../../commit/bdeb972b56101fe8c4de99354bc04afa08fcd9b4)
- Unused and broken Song Cache Version - [49a281d](../../commit/49a281d3016820dd50885242b022167d2c49d5f2)
- Useless CodeDetector codes - [102ddad](../../commit/102ddad6b57d84c9bfe347efc15861bf42511831) [5f657f2](../../commit/5f657f28ca4965890f690e23bf27001b536ea3e2)
- Uses of Foreach_PlayerNumber - [50e168f](../../commit/50e168f9fe737a3dc7e5781e4595ccb5ab21a191)
- Various dead metrics from older removed features or things in the list below - [a5a28e8](../../commit/a5a28e80afe4bff97b812848ddc72e1a4aae0d6c) [8bee766](../../commit/8bee766acb473f34135f59e8029ef86d82852c59) [933e8f0](../../commit/933e8f06024d76504973fd83b75bcd221409101d) [3b3b472](../../commit/3b3b4726fdcf287b0087f2897e420f56b349fa40) [2ca53bc](../../commit/2ca53bc8699216cf64ccf0068ecf46047e5f2fa4) [2033098](../../commit/2033098a8bf160ce474d362805028ef734170641) [7f5bf9b](../../commit/7f5bf9bb8786b9dd56a28a236501712c093df115) [0d98460](../../commit/0d98460e0d4d1725750b829415e2d707a112a7fd) [6375e20](../../commit/6375e20f6a51ee54e045ef15501690fec417a1b4) [0bdaf0a](../../commit/0bdaf0a405ef09848c9ba758b0bfac3496027c9a)
- WheelNotifyIcon & DifficultyIcon - [08f7b9e](../../commit/08f7b9edd608e8098f47da1224e81cdb8fba242c)
### Fixed
- A '#' as the first character in a field of a Notedata file messed with parsing - [9579773](../../commit/95797733dea91579aa0694a75184fe49f18fcce8)
- Chart Leaderboards didn't update when changing Difficulty - [b7c8651](../../commit/b7c86511e1c5789a90a10f5e2670466a77ee05a9)
- Colors were broken on newlines in Multiplayer Chat (and thus all ColorBitmapText newlines) - [1498cdb](../../commit/1498cdb6bcb9a0ece8cc12191c4a3d075728747a)
- Coverity Scan detected memory leaks & null pointers - [9a66db6](../../commit/9a66db603f3753e98ee5b6adf042607197d9cd03) [474c979](../../commit/474c979e17e2db507b57e031f207d383f1a69c7f)
- Crashed when loading empty Osu files - [8874070](../../commit/887407008e24adc85038766a21da5f2dc28841a3)
- DWI files that defined an unrecognized Style crashed - [8509dc8](../../commit/8509dc8f2446149c3c5b001eb013d537f4d6d400) [9855690](../../commit/98556904f3ca5a0a456a2c1db2b47edaa3630cd0)
- Gameplay exited early if the last Note is a Hold/Roll - [c41fb9a](../../commit/c41fb9aadf0ff34e07a91ea78d6f23929727a018)
- Multiplayer crashed due to invalid characters or bad data to log - [78759bb](../../commit/78759bb9367bd6d34259aacaf149580e02096243)
- Multiplayer Difficulties could get desynced - [96e0429](../../commit/96e0429e3dff32a3b079b99ce0a707297b90498d)
- Multiplayer Evaluation broke when switching Offset plots - [5aadfa3](../../commit/5aadfa344a78bedc157ed75c6c0dda74c89fbe6c)
- Offline Gameplay Leaderboard was always colored wrong for the Player's current score - [946e970](../../commit/946e97083bf75d1d9cd85df7ce3c1af483f7e7bc)
- Parsing NoteData & TimingData when loading Osu files crashed sometimes - [ea91ce9](../../commit/ea91ce9911f9f74e96bda0b5536e48aa8f9931ce) [457fbac](../../commit/457fbac3ced53a087590930d563d6b18c09fa106)
- Pasting into Song Search didn't work - [9ab20a0](../../commit/9ab20a0ada1b55b9fa281821b733571c2d5e60d6)
- Preview Seeking was broken if Preview is opened and then closed, then the Song is changed - [413a61f](../../commit/413a61fea9b1b4c5eb849e3302227f8a3e5eb558)
- Some Language strings for Skillset Sorts weer missing - [fed0026](../../commit/fed0026418f283e3ede802a0cabf262ca582005c)
- Some sightread Scores didn't show up as Top Scores when uploaded - [880b76f](../../commit/880b76fd8003c64b165ce65a3bb4741ab6a27194)
- Sometimes, 2 user lists showed up in Multiplayer - [765b4c2](../../commit/765b4c2e067fe966c6b4695288727e7b56715a48)
- Taps could be hit before a Hold/Roll ends, frustratingly as a Bad Judgment. Change the window to Good - [4893966](../../commit/4893966155d1f306c6a115bbbe3539c253ba02dd) [c0e1a48](../../commit/c0e1a48b9b989e41d2539675d9f301f875d8976a) [610f600](../../commit/610f6002351f39050fb1e7de6e348314011ec6fb) [23a22dd](../../commit/23a22ddb15e0a6b7539c6c65e3a5b6016d429ea2) [efd7c11](../../commit/efd7c11bb2f53c54345db5bec20939c0c334569b) [1c19cdf](../../commit/1c19cdf225c04e79e4a18b3598b3b7d30e29b203)
- TipType (quotes.lua) is missing so the option was broken. Hide it for now - [312a677](../../commit/312a677b7e52ccc5efc8f50604cee318276c9fcb)
- Userlists didn't show useful information for the state of each Player - [e71d82a](../../commit/e71d82a70192fb242876673a31c8ac2b46fbd40f)


## [0.64.0] - 2018-12-04 - Public Multiplayer

Windows x64, Windows i386, and Mac installer release.

### Added
- Additional Lua Documentation - [0bba0c6](../../commit/0bba0c6e5f0a27ddc7cebba25f1c0963c3b98c0f) [f099fe7](../../commit/f099fe7fdc41e5ad379e55c69eeec7687698d1bb) [578d009](../../commit/578d009fff77db6590c1bfc2fa9b2b15d2001fbb) [bb1ccef](../../commit/bb1ccef29b63bd5cdb82f9d4262052df5ed0de81)
- Automatic bulk ReplayData uploading - [aeb0ae6](../../commit/aeb0ae6b8b632d4d394af13add1b15a36b091320) [3829803](../../commit/3829803b2857990a6fdf04bac68c4a92018011c9) [9fd9726](../../commit/9fd97265ee725026b0ccf9287c9056eecc6b0636) [7692d4e](../../commit/7692d4e87586f18d6659044d5c7418dd8981d1e1)
- Chart Leaderboard ReplayData Retrieval - [a3488c0](../../commit/a3488c0bef7b3e6c049d3ee0b78f7494c0139800) [272d516](../../commit/272d516f7e3fbb82f2ecbc4925d27c420cfb8167) [bab7a02](../../commit/bab7a020313822b78a89eda4dc3e470b404ad160) [537ed28](../../commit/537ed28b9fa066f20125f52877fdb723eda3fc33) [0251c75](../../commit/0251c75534b9158d093c1a56a8bf11c81d0fad2e)
- Clicking the scrollbar by the MusicWheel actually moves the wheel - [a231159](../../commit/a231159410710d33667ec2ad8933170260e9e29e) [6c5818d](../../commit/6c5818d17b9b1f1d925a2a930e45e3a601f60cdb) [b9c41ce](../../commit/b9c41ce9110401670ff6466b56a62603ea8c3fcf)
- Experimental Chart Request API - [58ad984](../../commit/58ad984c75a364c43c8c5b16ca3a1c926a6ec064) [c516ef4](../../commit/c516ef42afe83a1296f82dd33b66aa0058e17bd1) [a0dbb00](../../commit/a0dbb004f0bca147516f4668d9ef0828d6ad5c22) [ef32fc2](../../commit/ef32fc20d23a992144d9a274960113a3e7d3f67d)
- Experimental Text Hyperlink Methods - [005753b](../../commit/005753bae8dca433a6b05566971658c4926adc8d)
- Fallback MusicWheel Lua Script - [0bba0c6](../../commit/0bba0c6e5f0a27ddc7cebba25f1c0963c3b98c0f)
- Lua access to a new function to play a Song's Sample Music with Chart Preview behavior - [0c976a2](../../commit/0c976a221c41693353a29218bc0c8fbac4b87777) [8921bff](../../commit/8921bff33621688023b5e6b2bf861364d9c0ca70) [e5dbf4a](../../commit/e5dbf4a8ff38e0c82c3c5d36382eee3f6338155a)
- Lua access to a new function to play a Song's Sample Music with default behavior - [e2f0713](../../commit/e2f071379c26be323b1d8852cff4f9a9d5c84d1f)
- Lua access to see the number of columns in a given Steps - [86e7401](../../commit/86e7401f79c9d5085329f6f5e851eab1b6e5479f)
- Lua library Functional for some data structures - [d6639aa](../../commit/d6639aad989ce19e4a78f0ff203cf5338dec3149) [3ea88b2](../../commit/3ea88b29e3bca35fc8810c1069ef6d35f4315bbd)
- Mouse clicking on the Profile Select Screen - [7aaf002](../../commit/7aaf002fcb08960d4927884a708a44f341ccb1b9)
- Minanyms - [a9694d3](../../commit/a9694d39f970c78affbf8c2a51a7ce96cf294832)
- Multiplayer
  - Censored Password Entry - [0e6ef97](../../commit/0e6ef97ddb6f3908d37d046ec59ab764e03111ab)
  - Chat Overlay largely reworked - [31dc372](../../commit/31dc3723e3d00b7f9dbf15cadd4625618b3535a1) [dd8d84d](../../commit/dd8d84dca361bf76179e9239235e126fd5be2d74) [15cab85](../../commit/15cab851b20fe4b8f6ecd570cdc0d9c453a3b191) [5dc5da3](../../commit/5dc5da301333b6fe3c82c39369af99f5564d7f98) [c81e4c4](../../commit/c81e4c486048521e703f4609f14eb846434af299) [16232b4](../../commit/16232b4bdec5d94a3d1891acd419b414030b2fc8) [293cb1e](../../commit/293cb1e007a7753673163913ecc34aa99cd3ff20) [757d431](../../commit/757d4319235ab5f4ca33a85c72c25a49bdbb877c) [315e832](../../commit/315e8329d0721f388b380cfe952c2d728998433d) [07269e0](../../commit/07269e04427967cd6eefba6d87b91762153d55f1) [ab1c596](../../commit/ab1c596e59ccf2f66a9e30c0d15cd338f9a1b1d1) [16754ad](../../commit/16754ad174645c1294fbde01af8c988a62889ea3) [c211afb](../../commit/c211afb6db22a7b278e2f37fc8673ed195c58da7) [0e4d75c](../../commit/0e4d75c1088aa44d84aaab22048441d423677abe) [3f0c97c](../../commit/3f0c97c3bf6b04774afc3bdfb0fb1eb60a9cfbad) [1fc925b](../../commit/1fc925bb96e20e45a116e0722e3406d9ebc2cae5) [1c3b3d9](../../commit/1c3b3d967f16de9e4270f1076dbf4217ee6dd0a6) [e277fb9](../../commit/e277fb90739b6e91459f0c51059ce07da2cd23cf) [794c2ba](../../commit/794c2badb06efb5dc867723cc1cbfb46e4a8e5e6)
  - Disable Enter in Evaluation for chatting - [3471e61](../../commit/3471e61c51b31aa0c4c10ed312e4aa04e2d77131)
  - Disable the Up/Double Enter option menus because it's too hard to track, put it in a clickable button - [63d09a7](../../commit/63d09a7288eb2f036274e64d9546d3b4e0b8359f) [e4156ea](../../commit/e4156eaf6e7936f1fe6dcc3a6e460e3bc821258a) [563c0a5](../../commit/563c0a540a33057d815fda6f78f5cfb5c2be4a9a) [8ef1367](../../commit/8ef13679cecaad92854db75aabfd1382e4755f86)
  - Disable Replay stuff - [12e420a](../../commit/12e420ab2ef6d010c005274951c3aaae30075cf9)
  - ETTP is functional - [eed136a](../../commit/eed136a383089359db30fc73439b6bd258d59029) [84408cf](../../commit/84408cf9c570a87cfd887e3e900fbd2676da76c4)
  - Force Start & Ready (These are buttons which run a chat command) - [f265c01](../../commit/f265c0151ff7ef2d867d0bfcaea8f2f463e68414) [906e014](../../commit/906e0140b9289516cec9a46f6e036d03e1dc4b1f) [dddf317](../../commit/dddf317d2de9e3f106010d25031fe012c2009f34)
  - Functionality to be as similar as possible to offline (Mouse support and everything else) - [cdd6f5c](../../commit/cdd6f5c32d22822eb82ef560bb80d11a2b868d03) [a47dc0a](../../commit/a47dc0a35b4aacf1b12ab03f44fb49a65806d990) [5dc5da3](../../commit/5dc5da301333b6fe3c82c39369af99f5564d7f98) [9f1bf36](../../commit/9f1bf36ef642a803d7366b93536e9aee2efd57d4) [76c8cb0](../../commit/76c8cb09dff410f97eb24b7870a085bca0a83e37) [cd5e1eb](../../commit/cd5e1eb63839cf8a455edd505c9600a2343db371) [3d654ca](../../commit/3d654caf97e4e531de879a82c423e3f6b7dffc31) [e97af08](../../commit/e97af08af50146c5e66b7c1d178241e9d1638454) [700d551](../../commit/700d5512ecca1ad156ab8e32a9679db78dfde23a) [ea54daf](../../commit/ea54daf2554c00afbc5312d3c8327e8cac54d870) [6268b34](../../commit/6268b3433921929d6d54ad2c583ab26278187aea) [381ea18](../../commit/381ea183c0f0aa67f41cf2efdf2cd1d203811947) [161c7dc](../../commit/161c7dc73f0013ea4d0154a751f10abb19eea459) [fef34cf](../../commit/fef34cf79576e4049cb42d7ec775cd9e685e4743) [bc4af4c](../../commit/bc4af4c4c90dd2c9dfd224c7c04c6ede67ad889b) [c59523b](../../commit/c59523b78da742fb6c65b9240e40c739f90823a2) [4689f71](../../commit/4689f71e692d3e82a7b789d5e1c3872eee67d085) [2a6d3ec](../../commit/2a6d3ec88995155dd2f6ca0109773e046bd7b2a9)
  - Gameplay Scoreboard - [4d57974](../../commit/4d579747d370c7e6026b4fe1290753eea67f1690) [afb7ca6](../../commit/afb7ca66eec47fb7c1414cac25f3c055a669650c) [9636b74](../../commit/9636b74d3c5fd0adb0d5be82692c8c0edb992049) [80fc6f0](../../commit/80fc6f0eddbfd56c82d8736db404eb27cb6e8392) [ad12946](../../commit/ad1294691a3f7b37c552c60e173b9cd5edbffad7) [fef34cf](../../commit/fef34cf79576e4049cb42d7ec775cd9e685e4743) [0a441a3](../../commit/0a441a373555a815e810bdd8970ddcaa1c82213a) [498fe44](../../commit/498fe4474d95df89fe73e3197edbc62f93c33fd4) [6691415](../../commit/66914159baf9ee2b676a9b4f578c1f5efdb06f25) [232db26](../../commit/232db26c64ec689a31ebac0689a3b69bb92ecaca) [e67ecf1](../../commit/e67ecf1ed0a8e529fafdedad2efe74c6dd36c2df) [81dc77b](../../commit/81dc77b385f3a2c32f043f8904bb96a120ce4945)
  - Lua access to Get & Set Evaluation Player - [3471e61](../../commit/3471e61c51b31aa0c4c10ed312e4aa04e2d77131) [7fb3f21](../../commit/7fb3f21075625b794e3e1e678f227dc9fc4d20e6) [27991d5](../../commit/27991d5cb647ea8a78e8586085a39e337d68f1cc)
  - Lua access to Lobby Evaluation Scores - [1fd2973](../../commit/1fd297384743bac25ba9b1f5c2ee1aa9f9999989)
  - Other basic Multiplayer functionality to be as similar as possible to old Multiplayer - [db12e12](../../commit/db12e1237da9d96eb883ef25603a3bf2500ae33c) [fef34cf](../../commit/fef34cf79576e4049cb42d7ec775cd9e685e4743) [d338256](../../commit/d338256dfb9e22aa90f012663be097102878a891) [d9e1e56](../../commit/d9e1e569e9a32f5df85d5747b9f8f08d76d9e46d) [f7a2d2e](../../commit/f7a2d2e6bb2602371074747942bb7d8b63310783)
  - Proper Userlist in ETTP - [b23caaf](../../commit/b23caafec69994a14fa36b576206698b0e326d2b) [06c8625](../../commit/06c862530e18a53406b6f6675c892d1af5229db0) [54372aa](../../commit/54372aa612cc97829b72bc2d1cb75c96e89da723) [7d0af34](../../commit/7d0af343607c1a184514a941ef3d661bbd3c8446)
  - Refactored Network Evaluation - [4ffcd4c](../../commit/4ffcd4cc760401e9bae4ba25fbfbeffcb5b778d3) [42212e3](../../commit/42212e3d4a8eb30752e0438bd708138a0263e5a7) [9272e30](../../commit/9272e30343620c5a088469c49b7e66b945f141d5) [a36c0ff](../../commit/a36c0ff1b9e422ed4867c1b85515bc027ef48d3f) [ac0e00c](../../commit/ac0e00c2b55ccf2c81da8b741dff8fd6be32a5e1) [ebee9f5](../../commit/ebee9f56dbee4b44c007b6879c9c232d978c6265) [151e9ea](../../commit/151e9eadd58a75aa86d4684fa48aa7251e416e22) [e95c5d6](../../commit/e95c5d659e6d0e81f20b08af32d48fa121c92fe8) [cd1cb97](../../commit/cd1cb97add49357f4bb8c5bad3587f085224abc2)
  - Some charts will not work due to UTF-8 - [e8f3cf4](../../commit/e8f3cf42be4e12b6cc420e57817f6ae8dcef67b7)
  - Uncensored Username Entry - [cbda3e4](../../commit/cbda3e4620b4b7a4c77422cba48a1e95a4423724)
- New Evaluation ScreenType - [e658b67](../../commit/e658b6763e1a07fc82ec848f47f22edf5c6bde7f)
- Online Replay Watching - [d5d51a0](../../commit/d5d51a087267295f6e7d4756d608fcc446ec41b4) [272d516](../../commit/272d516f7e3fbb82f2ecbc4925d27c420cfb8167) [bab7a02](../../commit/bab7a020313822b78a89eda4dc3e470b404ad160) [570bcae](../../commit/570bcaecb1ea20608f71243f957cb55391c60fee) [ade036b](../../commit/ade036b1f46f47bfc4bd686c6aaf597cd996e439) [0251c75](../../commit/0251c75534b9158d093c1a56a8bf11c81d0fad2e)
- Preference to use the stb_image library - [2768f10](../../commit/2768f10766206f0f39c0077af115139e0380ab78)
- Rate scaling for the Chord Density Graph - [a721e49](../../commit/a721e495076043a06180c04ec20c55d680f214b2) [8b10aca](../../commit/8b10acab090ddbc074c5d30094e95caa04e468f2) [3539b08](../../commit/3539b08349b9ab939004868689f9ac8badad6429) [f9a0e1f](../../commit/f9a0e1f95ea1638eeeb1a875ca32b193a8c8d335)
- Recursive functions added to Actors for Mouseover & Visibility Checks - [56f5e99](../../commit/56f5e99ee57371581678e1a272df5469a87f70d7) [01e7bd1](../../commit/01e7bd15598dd3838795f62665b437103cc3c7f4)
- Retrieval methods for getting the Fake Parent of an Actor - [5249e8f](../../commit/5249e8f8e3f0017586aad9719da7dfdd6a81b016) [6b157fe](../../commit/6b157fe7d35675e5b2be386c463e1ae2da786375)
- Screen methods to allow running Commands at set intervals or timeout - [6dece16](../../commit/6dece164ce6104ed6512dfb9c4cd3bb97c54c640) [c512d3a](../../commit/c512d3a8c5a16394e2009b964a86f7d1a9149edc) [21f1984](../../commit/21f198435b633f0bd691a39cbf4a36881504579c) [ffee38b](../../commit/ffee38b3b06a891766094163cdf32bf7db1840af) [3690d47](../../commit/3690d47c80ef1bf5c16defcc678e6e95261cf7c3) [41a5c24](../../commit/41a5c242d15e705116e9caced8f6f8dccf832032) [dc0337a](../../commit/dc0337a2a18305f4d287d931593669004a0ba8de) [ee81d6d](../../commit/ee81d6d04e5eda69c146b6a12e91921e68f851c9)
- Simple function to unload all ReplayData - [09aa87b](../../commit/09aa87b653dd5bc37e680ec4e9f239acce96eb02)
- stb_image library to replace instances of RageSurface - [23faeac](../../commit/23faeac5f59d7268ac18ffd55f798d6c90027b6e) [4930af5](../../commit/4930af50bd0f1924d9e0ba6b4738bea85b85e81a) [4930af5](../../commit/4930af50bd0f1924d9e0ba6b4738bea85b85e81a) [7d58c1b](../../commit/7d58c1bb721f617f4b926bd7718411ce1d4e6de2)
### Changed
- Chart Preview details the Song Position at the cursor - [c01495e](../../commit/c01495eb6209eb7117776fa53d0229f74e859538)
- Chart Preview refactoring & fixes
  - Delete the NoteField if there is no Music - [584f2a8](../../commit/584f2a8be5d9c83eea0d8f9844c980f717b5e3e7) [54811f5](../../commit/54811f50841facbf6cace27900cb696f7865e893)
  - Keep Density Graph bars in bounds - [55ad823](../../commit/55ad8234a16ed47220339fbd82749fde96b1050f)
  - Misc refactoring & fixes - [869469a](../../commit/869469ad2437e08a5722a89ffab0278cceac98c3) [264fb63](../../commit/264fb63a27b7667592adc6075f1affd0deb3c79b) [700d551](../../commit/700d5512ecca1ad156ab8e32a9679db78dfde23a)
  - Premature Fadeout & Joke Sample Music Handling - [3f17b7c](../../commit/3f17b7c9a0348381d43ccaefcda80f53868c3d54)
  - Transfer music control to Lua - [decdc66](../../commit/decdc6698cdf781e58cf765eb6e4e9f3c32d56a1) [23022b8](../../commit/23022b8613791885beac56eba46ba4ed03feca4b) [9d52f99](../../commit/9d52f9992086f4aa3a62e39d05450d0e67c637a5) [9036e6d](../../commit/9036e6d52386d60adad079e17d8ab6f372a5851f) [343c13d](../../commit/343c13db72c3f7b9f93ad28ede5a82c8893d0a56) [f88caa2](../../commit/f88caa2a67a9fac5181d94e1d5aa2c7fea67a060) [19bbc4b](../../commit/19bbc4b29e944cf2a1ebce44853f5772c733ab2c)
  - Unload NoteData in use - [54811f5](../../commit/54811f50841facbf6cace27900cb696f7865e893)
- clang-format was run on the source - [13581f7](../../commit/13581f79df2220776f2033c14b36da2fd896fc7f)
- Clarify and correct Chart Preview X position centering code - [47ed239](../../commit/47ed239e3ca443012343971721491b76fe8b0432)
- Collect Garbage from Lua in SelectMusic - [c708453](../../commit/c708453ea3e021c98a0f725eab4e1b1c6732f8b7)
- Color Config takes Menu Direction input instead of Gameplay input - [5d880bc](../../commit/5d880bc4a1f8f7e6cd0c5f8ea80ccea8030c5ff4)
- Default Texture Resolution lowered to 1024 - [bcd2e81](../../commit/bcd2e810d8db08cc83fdd0b25972a8f1ca83b066)
- Don't save the Profile every time the Playlists are touched because it lags - [6d4a331](../../commit/6d4a33191ac2904676cd6b9b3d48768cd7864331)
- Don't unload ReplayData everywhere - [ce2a40b](../../commit/ce2a40ba9ba5c7bc8f56b2d3e00d66070c4e7b4e)
- Don't update the Chord Density Graph if it isn't visible - [29eafa3](../../commit/29eafa351e37e1a3188183807ddc53a7b596a876)
- Double the check for a Song being loaded by checking by Chartkey that it exists via a Steps object & a Song object - [5ffb8aa](../../commit/5ffb8aa925da9b26c9cf3feff6500c3d3b3ecd3b)
- DownloadManager's Downloadable Packlist got moved to Lua - [8da0bbd](../../commit/8da0bbdd609cb74aa3509d50437ec9121e647a0a) [8d67f03](../../commit/8d67f0393343fb0d09745184fa7d334e433eef56) [9f51065](../../commit/9f51065d9ba066bdda0edbcd69ebf8d7d7910683) [b3fd361](../../commit/b3fd361bc8c88df6278db1076f298c539a70cfef) [460f3a1](../../commit/460f3a19f4e3667cee660b7f4afbda412b7f2adb)
- Garbage Collecting RageTextures shouldn't make copies of them - [a739d7c](../../commit/a739d7c9d814e2715692a8a178a79eaf3c0beb4a)
- Guarantee that Chart Preview visibility is what it should be - [5f83ccd](../../commit/5f83ccd4c11a9e304c060c9086288c88dadf7f7c)
- Improving Watching & Evaluation Viewing from ReplayData
  - Judge Conversion Fixes - [f59b2ca](../../commit/f59b2caa1bb08eaa203c47646f8d66a5f6b05f32) [8c431f3](../../commit/8c431f31b49fbd49ffadfa944bba1b693798a910)
  - Other bugs - [38481fc](../../commit/38481fc81c2cb1c9ebdfb30dfd09a1fded118074) [a4a316b](../../commit/a4a316bbf09e5a186de4b74e8a3120e314c9e8e7) [17d6195](../../commit/17d61955128242761161dab04d839ad35b0222f4) [55ed90b](../../commit/55ed90bee8fdb113b1ecf53063afbcfc3ab8a81d) [f54d91f](../../commit/f54d91fd09cb9d49b92394c893e50e2fcd0b4b56) [5da2462](../../commit/5da2462f52e2bc5f85eeb7d8d458984558c67004)
  - Replays are allowed to Fail - [d15a661](../../commit/d15a661207a48989fe145a3d2825c56ef556beb0)
  - Row calculation is set up to work with Local & Online Scores - [2931fc8](../../commit/2931fc82e4209ab41f5bb66d3f8fcf1cecc3b8a2) [e6092c4](../../commit/e6092c4a1b6f32dc822c335c3ce74a2785996d0a)
  - Utility to calculate Radar Values - [7417ba5](../../commit/7417ba5123adde140506124f4a477e8b349c8b3e) [8c431f3](../../commit/8c431f31b49fbd49ffadfa944bba1b693798a910) [f866af2](../../commit/f866af2ebb76a799b339bd6942fe9db6adb4192d) [a93ed09](../../commit/a93ed097a87f926bca342b5ccad99d596d2170a3) [bd692cc](../../commit/bd692cc1788ff35d048878d7369884b1b112dc39)
- Installer will delete more old Theme folders - [381ea18](../../commit/381ea183c0f0aa67f41cf2efdf2cd1d203811947)
- Lower resolution of some fonts - [319f4b0](../../commit/319f4b0fc1b79da939157c37d34f64749fd3591d)
- Make sure that Uploading Scores is done by Scorekey and not Chartkey - [08fb18d](../../commit/08fb18d35d38470af3beb95a1feacac1784569d3)
- Misc Lua Cleanup
  - General tab - [a876629](../../commit/a8766298dd9f40f9fb681b52e73de58ad52ba0ad)
  - Generic Highlighting - [f12eb31](../../commit/f12eb31aead4c0227efa919b9ced4d3bcd905fe0) [d9b59e3](../../commit/d9b59e34d91fd6fe17cdb5ae0239439b30c5bbc6) [faed128](../../commit/faed128e301bbe6a2a012b389386043cfbe5c65c)
  - Mouse scrolling - [ea54daf](../../commit/ea54daf2554c00afbc5312d3c8327e8cac54d870) [64a9110](../../commit/64a911040f4c42828ee94efbc6d4f6ead3b87fd3) [12807ae](../../commit/12807aec9abb08a81ee15b1f506963e0524e17b2)
  - Optimization of Input Callbacks - [1e11183](../../commit/1e1118379c6293503e8d5dac7532f0a83671b024) [07269e0](../../commit/07269e04427967cd6eefba6d87b91762153d55f1) [9a39605](../../commit/9a39605cdf4b37bf9a866be80ee7e19f2bc8b73b) [eb51304](../../commit/eb513040f7472c1ccbb964f893287d6a2824ccee) [a9a4650](../../commit/a9a46500100726b34153824fe3b9fa98b0038d46)
  - Overflowing Tweens - [bbe157f](../../commit/bbe157f10632a870bebe885c6b4fb3ed82d53701) [8363efc](../../commit/8363efc8a57232fbe49fac63df3aa023631e42ba)
  - Playlist file - [d9b59e3](../../commit/d9b59e34d91fd6fe17cdb5ae0239439b30c5bbc6)
  - SongBG file - [bf195c0](../../commit/bf195c03191c73ed17f84795031194c1991c208e)
  - Variable optimization - [1b10cb3](../../commit/1b10cb36d76dced1f8e2ea29cf888e402ffa9988)
- Optimize checks for Song Groups on the MusicWheel for Song Load - [da66a81](../../commit/da66a8186f6c87e48183abf77136e528abfd5864) [75f62ee](../../commit/75f62ee0a1a11a7428d0761458a9b4696f4a6e9b)
- Put the background back on the Display Percent in Gameplay - [37ef602](../../commit/37ef602d5a6dfe2d4a071a53a1c8fb64f11ecc5e)
- Put the ReplayData unloader at the start of Gameplay - [54a0f9d](../../commit/54a0f9db0862cd317bfa95b1004cafb23f90e6eb) [391327b](../../commit/391327b5d92d6caf935902b03ab1cfa5428e9a1e)
- Replace instances of NULL with nullptr - [00c04d4](../../commit/00c04d466d970b8eb59fede941f3349a36c61811) [49baed2](../../commit/49baed25be1eaf35459a5178f5ecbccce99f2c83) [58de944](../../commit/58de9442a9a8801c4012706ec9fbc4e82aea32ad)
- Set FailOff for all Replays due to unexpected bugs with Failing & Misses - [a16f36d](../../commit/a16f36d7e2d389280bf7a41d89943164d130ea81)
- Set many regular string parameters to be const refs - [dd03122](../../commit/dd031224e895838766fee22fc81aa4cdb4f01101) [58de944](../../commit/58de9442a9a8801c4012706ec9fbc4e82aea32ad) 
- Status updating for Chart Leaderboards is not correct enough - [4dedc2e](../../commit/4dedc2e5bf4e4cc8aeea9ca85d77e9030afd5300)
- Uploading Scores with ReplayData also uploads Noterows - [da6e59a](../../commit/da6e59a9d5ca28774fa07a5d4d10a01128136f5f) [10d4855](../../commit/10d4855cde5442c9ab8c241dc14ceef97e4f3137) [21d4393](../../commit/21d4393abbea29d5ebd7b55103ef893dd05790d3)
- Vanilla Stepmania Documentation is moved to a Legacy folder - [aff1e92](../../commit/aff1e92844be94f60bb2ea15bd7015873ec9a307) [f311816](../../commit/f311816c82750205dec255ce854ecc9e6993b695) [b71dae5](../../commit/b71dae544b51c17a5f77bc9b4253df2f3ef6e72b)
### Removed
- Dead metrics options - [180feaa](../../commit/180feaaea25e3efbfcd77aec7dafd767ede77345)
- Experimental Overratedness Calculator - [dd03122](../../commit/dd031224e895838766fee22fc81aa4cdb4f01101)
- Lua Actor method to AddChild - [333eefb](../../commit/333eefb837fa0d1585204a844eb29099967bc35c)
- Reference to Courses - [2337ca0](../../commit/2337ca063bcc10eee7ebd49b5bc36054c623ed42)
- SMO Related Lua - [76c8cb0](../../commit/76c8cb09dff410f97eb24b7870a085bca0a83e37) [5ba071e](../../commit/5ba071e766aee1f0e84bcc2ac5a6cbcedeab1c0a)
- SMO Related Protocol - [6803d0a](../../commit/6803d0a0c4e2391f012c50a01d784c01225fa19c) [d0af8d6](../../commit/d0af8d68c8e2cc817f15f01f52641842469a3bac)
- Unused source folders - [736439b](../../commit/736439b73649172ed64e17eaf6bb0b71b95fd4e8) [b44b6d5](../../commit/b44b6d5c101f0b963fa9988d326a2676d7cde827)
### Fixed
- Bulk Score Upload function failed due to lambda function - [25742cb](../../commit/25742cb07baa71cbc09760b7e6f7bfbe70c37eac) [8bd3e8f](../../commit/8bd3e8f0f2d2e55fb28e53f146d6ca6ebf3ce3ab)
- Cancelled downloads broke everything and didn't delete - [bcb5fb8](../../commit/bcb5fb898afb12e2f6875553b9aadbead2454624)
- Chart Leaderboard Retrieval was internally broken - [a3488c0](../../commit/a3488c0bef7b3e6c049d3ee0b78f7494c0139800)
- Crashed when changing to a Song with more than 12 Difficulties - [471e805](../../commit/471e805884e476b363ac59bb84686e23df608d20)
- Crashed when attempting to add a Pack to a Playlist - [30c345a](../../commit/30c345a362c4492d72d7f331b12bdc8bcdee9f45)
- Crashed when attempting to make a Goal on a Pack - [79709a5](../../commit/79709a52905551539ce3752c7fe56d0249f1922f)
- Edit Profile Screen softlocked - [45da444](../../commit/45da444686a93a43589bac211c8b0b62e8c2995b)
- Linux couldn't build due to type inference in a lambda function - [63846a2](../../commit/63846a2827467615da73d5516bd60df8dde6b026)
- Lua function parameters for RequestChartLeaderboard didn't actually do anything - [dbaa975](../../commit/dbaa975302fce60421e82b731edc21c365ad184e) [b938222](../../commit/b9382228c384d46c39b8de792dbdcfaf5c04b8c0) [1e21ae0](../../commit/1e21ae049948b0c75da06d9b63946dfdd14fce06) [4509801](../../commit/4509801a64c592eefa4446aebc282ad217ba61f2) [478c9cc](../../commit/478c9cc72b91d884e83148365e59d2e5edd4d716) [5ec7f71](../../commit/5ec7f71d355d43f02afb9afea119f78162a4e98e)
- Manual ReplayData upload function failed due to invalid JSON - [3e872ee](../../commit/3e872eee4b28d531ab518e9944a470a5f9f9ef84)
- Mines caused a Lua-sided Judge Rescoring error - [a19738c](../../commit/a19738c6575d949ba8c202e297b0d9da96b79c5a)
- Misses were not in the Gameplay Leaderboard - [2592c6b](../../commit/2592c6b4c4d925208ef1356fe9f57c74a1d8a39c)
- Non-floating Scores tab didn't respond to Rate changes - [aad0c4e](../../commit/aad0c4ed9685175171dfdb93bf6f4ee9f9f676a8)
- Osu files didn't work in Multiplayer & have no internal filename - [79dd2d1](../../commit/79dd2d13e483faf91bbb7e32232b9977b481b27f)
- Permamirror did nothing - [3c2c634](../../commit/3c2c6344eb8a78732558a48c86031dac5c66ed43)
- Popular Sort didn't work - [b16c1d3](../../commit/b16c1d3a9c6b94e6a6e3d110cde1198d766ac345)
- Scores tab didn't not update when switching to it - [8a5dbba](../../commit/8a5dbbaaaea86b3ae5cbeeece92973a93adc4807) [d0f9776](../../commit/d0f97769e87d181a493816ec7bd8625d622795e2)
- Score Uploading crashed due to null Steps - [4052c14](../../commit/4052c14ae055d47fd79af3c0c73b3c2d60bf9587)
- Song folder sitting directly in the Songs folder just crashed the game. Instead, throw it into an unsorted list - [fefadeb](../../commit/fefadeb5344f82098804cd71c3fdf8abaca6a1a4) [3c3a5db](../../commit/3c3a5dbef56f5979f2f900241e460dca24782d64)



## [0.63.0] - 2018-11-23 - Performance Boost

Windows x64 and Windows i386 installer release. Mac zip included.

### Added
- Main Skillset for a Score in the Scores tab - [1dab361](../../commit/1dab3619c56a5a925738562483d2d8873951bdeb)
- Replay Watching for ReplayData made from 0.60 forward hits Taps using offset values - [2003f69](../../commit/2003f6936e230d684511159046ca6dc9ae1b11d6) [f63ea82](../../commit/f63ea82f1aa6a6975ec8b57d1874396d0d8b750a) [a6563b0](../../commit/a6563b08049c208f3c79a0668964d8d67d37ae96) [5f18c63](../../commit/5f18c630b12b8dcad605bb5321316f9d74d753da) [9181723](../../commit/91817235e88287693c54147877a4824dd339cf49) [2efb3fa](../../commit/2efb3fac3d7369529d58722382c197b10278ec85) [ae000d7](../../commit/ae000d780f5979f7ea2abbf8b4e9d16cebe5f30e) [3736163](../../commit/373616359d38dcebb6d27efbffca3779aba6ab5a)
### Changed
- Banners don't load if the Banner isn't visible - [20cd6fb](../../commit/20cd6fb333ab358cffa93aeade8daba1982d7cc1)
- Banners don't load if moving the MusicWheel fast - [92e5359](../../commit/92e53596f189ae7988b90d97368917a6148a409e)
- Banners moved to the General tab Lua - [e0054b8](../../commit/e0054b8ed9a2f11d8e0e613dfbf1093b2dacd75e)
- Banner-specific updating logic in Lua is reworked to try to reduce memory leaks - [a189b74](../../commit/a189b74db87c1f6446b5904d5295856220189a90)
- Clear cached Chart Leaderboard data after Gameplay - [fc3c4e7](../../commit/fc3c4e79e5020d004d3a8ae29ee458b161d328eb)
- Don't update DownloadManager in Gameplay - [01710d7](../../commit/01710d71bb7467b1bd4f158207aa5307ed8476da)
- Garbage Collect from Lua (this should be called at least once somewhere) - [0bdd05f](../../commit/0bdd05fb7570b066bcc392e1b7acd61c9855e3e4)
- General tab Lua refactored for speed (this reaches all SelectMusic Lua) - [cce801a](../../commit/cce801a9981c0b40049d00064138623aca4737b6) [e0054b8](../../commit/e0054b8ed9a2f11d8e0e613dfbf1093b2dacd75e) [0bcf406](../../commit/0bcf406553347843b455e14d6db16f0c5b40e44d) [d793ad6](../../commit/d793ad6356f093c1404cc4727a690b22ebe4dca3) [92fb0e1](../../commit/92fb0e170da00820172034e4834e8f6556a946cd) [4ac9453](../../commit/4ac9453051d4953b707e6ff464ac4e8a9bd21ef4) [cfc8edc](../../commit/cfc8edc28b0e4cd9b8ec70410c5ecf02c791ca9e)
- Move Lua functions to get Display Scores (PBs by rate) to Scripts - [5c9b126](../../commit/5c9b126645e7108073f53eab10399cf4dbf3c9c8)
- Refactor highlight updates for Actors in ScreenSelectMusic - [c37d492](../../commit/c37d492557c9c30255804c131106df633c455814) [0bcf406](../../commit/0bcf406553347843b455e14d6db16f0c5b40e44d)
- Restrict max number of difficulties in a Chart to 12 - [c37d492](../../commit/c37d492557c9c30255804c131106df633c455814)
- Screenshots on the Evaluation Screen can be taken by pressing `Select` & don't save as low resolution in a weird folder - [508f50b](../../commit/508f50b56f9579e3543703ffa5d8fe9143ebaa41)
- Stop trying to store Online ReplayData directly from Chart Leaderboard Requests - [d4eda50](../../commit/d4eda50e337490de69b8687c03d2200dc874389e)
### Removed
- 2 Part Selection, a 2 Player feature - [731289c](../../commit/731289cc5e01f1c18f7f6d5760fa9822b9a0b23b)
- Unused or unimportant internal ScreenSelectMusic Actors - [e01ab8a](../../commit/e01ab8af140fd654fd15bcd9f5c276588ab4e912)
### Fixed
- Chart Leaderboards were being requested too much due to inherited Actor Commands - [2cdb5b5](../../commit/2cdb5b5de93286d06ae34b65917ffb5ac81b6a5d)
- Chart Preview was placed badly in 4:3 - [1f54adf](../../commit/1f54adf992441c8023d8ff0cea1c0012485caab9)
- Coverity Scan found some issues
  - Null pointers - [cc09585](../../commit/cc0958578883cb5284a2770125563261200f0753) [a6f9269](../../commit/a6f92693269f9fe2ae29daf058af7af776687c51) [a269a3b](../../commit/a269a3bcba84cc4a3fb59e163bcbb594c244bb80)
- Lua return values crashed everything but 32bit Windows - [b93f093](../../commit/b93f09380de951532f0ced05751f13c3a121c802) [fb5ca86](../../commit/fb5ca860f154b66a8e7f3292111d3e37939343b4)


## [0.62.1] - 2018-11-14 - Hotfix

Windows x64, Windows i386, and Mac installer release.

### Changed
- Optimize ActorFrames updating their children by changing an iterator loop to a range loop - [d116c02](../../commit/d116c02c92317431f36be7d12d9a5aeadea9b948)
- Optimize NoteDisplays updating their resources by changing an iterator loop to a range loop - [df7c12b](../../commit/df7c12b57f5120c4504a27987dec0f5f3e94bdd8)
- Optimize the Chord Density Graph by using ActorMultiVertex instead of a lot of little Quads - [dd02cfa](../../commit/dd02cfa48b57deecfe21c3b26eeb27686a961042) [5c0fed2](../../commit/5c0fed2e8c798c8f818632758daa1eec397831ae)
- Set one of the lines on the Title Screen to be color-configable - [8bfd6d3](../../commit/8bfd6d32d89c928e1e4cd9c9da77816b7aad3e98)
- StepsList shouldn't be so slow - [bf5411f](../../commit/bf5411ffc1144a100ad4e37905850af4a5ac4b04)
- Try not to load a Noteskin Resource for every color - [08798a2](../../commit/08798a28d87e24ed98d362c292c8919ea8dbeeac)
### Fixed
- CDTitle & StepsList was in the way of Chart Preview in 4:3 - [f550b00](../../commit/f550b00f93b3b6df70aae10c9b9fe44320c8865d)
- Chord Density Graph was too large for 4:3 - [86eafd0](../../commit/86eafd0ff425690e9dc5785e47336d4b05e78cc9)
- Division by 0 error in Offset Plots when the length of time was 0 - [56aaf5f](../../commit/56aaf5fd397b5a26000a2df4420b51e3b3a56eb3)
- Invalid Difficulty Enum Crashed in Score saving. Fixed by defaulting instead of crashing - [2c81ea0](../../commit/2c81ea0fa82819ab4a0aadaffffb32300c24d79f)
- Leaderboard Grades didn't color correctly - [28d63b5](../../commit/28d63b500fda09bda64f620ff6ba63aba4617928)
- Mouse was not visible in Fullscreen in CustomizeGameplay & Replays - [6c1790e](../../commit/6c1790e1b0995f8fabcac8ac590de99c3930da6e)
- NoteField Y axis movement was still busted when switching between Reverse and Upscroll - [b0a3735](../../commit/b0a3735aedb4966f21af751c7ae5bbf56776941a)
- StepsList appeared if changing tabs while not on a Song - [546a1dd](../../commit/546a1dd7fcc1612aeab343bd0d290147f106a3f8)



## [0.62.0] - 2018-11-11 - Chart Preview

Windows x64 & i386 installer release only.

### Added
- Chart Preview & Chord Density Graph - [677e13e](../../commit/677e13e398e725eca38538d66045681636882dd2) [802720c](../../commit/802720cba868b5c645261339d084a5c41016bd7b) [6a2affb](../../commit/6a2affb0d5e8c73f6b7a5bfd4413ed44a4c897fe) [f60c413](../../commit/f60c413e572fa48d6524ba733c44985c1da5fd77) [195efd5](../../commit/195efd576a9598f2fa045ba7927f2c871aacd7d9) [fcb659a](../../commit/fcb659a12d7d59df10f059624bc3d0d3c1da04c1) [611908b](../../commit/611908beb011ea3e517c0f1e845d95cef9368b69) [6f09311](../../commit/6f09311f0deb7dd5be17bb205fcd05df52f89672) [25e1da4](../../commit/25e1da4eceb2917c7593ad096a67b3e6658dc599) [b562ae8](../../commit/b562ae85e2c3ee504465c94fa0596df6038b890c) [9cece4f](../../commit/9cece4f04f8f4fac37d79c36353917ec36920644) [0ac88ee](../../commit/0ac88ee4dce8177e8dfb9f700549540229d99ba3) [0275096](../../commit/02750966213be55d9d6a4d13353794ec5d891961) [6915211](../../commit/6915211f843546a30dce8f9f5c4bddc2b4300557) [d3dd947](../../commit/d3dd9473407843346a800904ee7e067afba1af9c) [64c7f05](../../commit/64c7f052e7fc79688781b95c1e0bbacdd0ecad00) [14002c8](../../commit/14002c8ef11582c560ea617b7e6cfc005abede83) [6ca9123](../../commit/6ca91234ca80647a5251bffe01bd8a6855823f9f) [2c2b762](../../commit/2c2b762cbaa5fc5e1e9998c1f52c0789d4808c1d) [ed014c7](../../commit/ed014c7606e4e71a584db7334d33ea2c31791579) [08c6ba9](../../commit/08c6ba9a68adb539283378bd8051224da60e5884) [9ef02b1](../../commit/9ef02b1fbc0231c6233eaef99a6e509814f64874) [cd5827f](../../commit/cd5827fb929010c638d0ac3312f89ff09d49b184) [c8f675a](../../commit/c8f675a725ee77fec2db5b92ef8ff529eee8bdb3) [4d2d6da](../../commit/4d2d6da69ff1470682167124f391444187da2b85)
- Chinese Translation - [abfeeed](../../commit/abfeeeda22996a1e20882e5c58d3e83117a8a38a) [8f2804f](../../commit/8f2804f98cbba1ec0471efee3059cb86091e00ad) [4ec4b26](../../commit/4ec4b267898147003a28e8783e775b65d64f7bd1)
- ClearType display on the General tab - [1446cba](../../commit/1446cba91f6c15b3bb4b99b6cd59716a302349e7) [b94bcb7](../../commit/b94bcb70fffcede5ba8109b2ed3116a2cf9153f0) [eeef67e](../../commit/eeef67ebe2b8f509fab6938fc7739f26770c583b)
- Hotkey for reloading Pack from Disk - [aa94450](../../commit/aa9445054cc2afc2a5fb76d64c47ce6af33b65dd)
- Judge display on the Scores tab - [212995d](../../commit/212995defcdcd24f54fbcba70150fad4dbbd2084)
- Language support for some Til Death Options - [4ec4b26](../../commit/4ec4b267898147003a28e8783e775b65d64f7bd1)
- Lifebar added to CustomizeGameplay - [7ba8f36](../../commit/7ba8f365826aa0b36b87b7b0f122163c184b8323)
- Lua access to the currently playing music TimingData - [1ebb47f](../../commit/1ebb47f86486d56726d93f599a3c5eeb4969eb3a)
- Lua access to Volume - [8c89d91](../../commit/8c89d91d444b3b499058515242ef06685681eb45)
- Minanyms - [0e76024](../../commit/0e7602481eb63fad80b6f98cb7c6e7b6ec379aa7) [c0c6660](../../commit/c0c666033f116d41543ac606b94a6ad920ed6d6c) [8c0c084](../../commit/8c0c0840363c2d92cd152d9e197755578381af42)
- Mouse support, Leaderboard, Replay buttons, bounding boxes added to CustomizeGameplay in a big rewrite - [5321555](../../commit/53215552a957b60aa9dc0c83451b246b8a5e4f1f) [83744a5](../../commit/83744a5dff8183f7247d64a4d7a9584db43d6a93) [ba32793](../../commit/ba32793195829aa321f04bef70fbc52c5b8f7108) [6d5cddd](../../commit/6d5cdddbafb6992792edaea3749d0fba8c5851d8) [5ee5026](../../commit/5ee50260bf26d0a8161d257c5ce1958ba05afade) [81aaec4](../../commit/81aaec47bff47676289d968be30bbabcd66f54b7) [302cc29](../../commit/302cc29d867e5aa5eabbcbb85f9f0a181d8e7e01) [bc3d7c6](../../commit/bc3d7c69956b439b435bf93e8ac24f1813367ff2) [0564664](../../commit/0564664411c5b298583c7daafe8297f76c5d5a5b)
- Preference to reduce the verbosity of Log Trace outputs - [4ef582b](../../commit/4ef582bc40198c4cc279444d370752659569522b)
- RageSounds can have a defined start point ignoring the 0 time - [9761618](../../commit/9761618904ce37d31fae20b1d68413fef63237ee)
- Right click to pause Music (this is a bug but it turned into a feature) - [c8f675a](../../commit/c8f675a725ee77fec2db5b92ef8ff529eee8bdb3)
### Changed
- Changing Rate should update the Chart Leaderboards if set to CurrentRate appropriately - [16b7e75](../../commit/16b7e75e816f83d7bae4ea6a2e54d7d4f364ceb1)
- Color Config options for the Main Menu - [414705f](../../commit/414705f9ae677fd1653349bf10d5162f9dbf1784) [a9c4bf9](../../commit/a9c4bf9a58acce30ddf49d73aaf71115da54cee0)
- Default buttons for Dance mode should be the arrow keys, not random other buttons - [d367cf2](../../commit/d367cf2dc775d9b1af3476df0e84b8867ab096f2)
- Favorite Song count is replaced by a Refresh Songs button to trigger Differential Reload - [0a027fe](../../commit/0a027fe8c5d53b567495a4646f12907aec4178d0)
- Reduce the amount of times disk ReplayData is checked for just doing nothing on SelectMusic - [1004525](../../commit/1004525232c812b9d6aa55c11b3b6002984870ce) [abbacec](../../commit/abbacecbb62f73eeb2388201059bc78eb1a4af3a)
- Reduce memory issues in ReplayData checking - [b2989a9](../../commit/b2989a920aafd8463a14d06590eb41dc1b828eb9)
- Refactor a lot of Lua to remove Player 2 code & improve readability or speed - [0c1f72a](../../commit/0c1f72a5c34f04afd99fa45d70daebc6bf66e90f) [9daacf6](../../commit/9daacf6d89bbc16c70896480084abec8eafbed2d) [247c3a7](../../commit/247c3a7c641b53e49f68508497de843d55cc1bdd)
- Rewrote default Player Config Lua Scripts for clarity - [bc3d7c6](../../commit/bc3d7c69956b439b435bf93e8ac24f1813367ff2)
- Setting the position of a RageSound is done by restarting the RageSound with a new start point - [86086cb](../../commit/86086cbe5d4550b3f3de9c72300be1a586eb0eae)
- StageAwards don't exist so don't use them to calculate ClearTypes - [9b8dbeb](../../commit/9b8dbeb12ce5e8636b8036b7339a836cdb929f0c)
- StepsList moves up off screen instead of left - [7a53904](../../commit/7a53904a3ab8a28f68ab01893b05ed40a12f01dd)
- Text in the Abort/Retry/Ignore dialog should match the buttons - [edd3676](../../commit/edd367654b1fbdbb180711ef7831fb34f362a35d)
### Removed
- Default Player 2 back button - [76fd569](../../commit/76fd56974049f73f985ea7ce3f10bba2020919d2)
- "Press Start To Join" on Profile Select - [eac7298](../../commit/eac729835aedcc7b52f2c34301cc2941aa56311e)
### Fixed
- Avatars didn't appear in Profile Select - [217509a](../../commit/217509a666d3aa098d094354c0012f66071317e9)
- Backgrounds didn't change when loading from Cache - [abe6986](../../commit/abe69864e32f2c864d220be627683cd72ae84eac)
- Crashed when backing out of Profile Select - [4c234f3](../../commit/4c234f3792052684d0e48e3b649c4b39b30470db)
- Crashed when saving a Score after making changes to a file - [60e28ca](../../commit/60e28cad6cb6a1eb693f02881f7c993fc5622f34)
- Crashed when saving a Score sometimes. Try to catch it, but we don't know how to fix it - [941884e](../../commit/941884e8636011d67b198c861663c2f9f48b88cf) [4475e60](../../commit/4475e6028c1ce70e995d34c8028a593e3b534dcd)
- Game Update button was badly placed - [3e6d22e](../../commit/3e6d22eb47a4c921ec4c79ad741ce5859b316c62)
- Linux didn't build due to unfinished changes from the previous version - [5f72a70](../../commit/5f72a70b1fd27f4ee16e56cea4c8df507ff40d61)
- Mac didn't build due to asm vs C compilation - [709b405](../../commit/709b405b1773a1915fb249e80d32653daad5ca97)
- Permamirror broke Replay Watching - [92e9868](../../commit/92e9868337f7d12d38b7b6060ed897cee0118ccb)
- StepsList display didn't hide correctly when hovering packs - [7ffd402](../../commit/7ffd4023ac6595afec77eeee8156be9a90f29198)
- Style didn't get set when changing Difficulty in some rare situation - [5a003a1](../../commit/5a003a191795cf9e20ad7b4db873f64cdf224e23)
### Note
- Temporary (reverted) change: button remap lock/rewrite - [b6f0621](../../commit/b6f06215ed7837774e7b49c7234b6933ea9840ae) [0135f67](../../commit/0135f6793fe2a5a3e52cfdea1144f6983b71a818) [933f2c6](../../commit/933f2c64752f736dbde2aa8626bb591a266811b6) [8939993](../../commit/8939993d7a9b32f64b664f9685698cac88d2663a) [c3a6dcc](../../commit/c3a6dcc6f9dc2aa60ce6e843cac5fa0837e2a214) [c6b3bee](../../commit/c6b3bee52a24df430a60da1b6a88f573f61eba27) [f4d1d55](../../commit/f4d1d5533a9e3f991bcfe73216ce6a72c2bdf002) [3c0c05c](../../commit/3c0c05cdaf3413469a680212df758b0c95290b4e)


## [0.61.1] - 2018-10-17 - 64bit D3D Crash Hotfix

Windows x64 installer release only.

### Fixed
- CMake does not load the correct version of d3d9 - [396d856](../../commit/396d85601c6844b17dca5d0ccf91202397248a5c)
- Saving a pointer as an unsigned integer in fails due to increased pointer size on 64 bit - [b984fe4](../../commit/b984fe493d9f7ac84a35af3e6f80f16607aceb09)

## [0.61.0] - 2018-10-12 - Replay Watching, Multithread Song Load, and 64bit

Windows x64, Windows i386, and Mac installer release.

### Added
- 64bit Windows Building - [1782624](../../commit/1782624bfa76e9b8ba7dc0915836a55f9fa0a414) [9ecfd48](../../commit/9ecfd485a14e65c2049d33a81087474f4c54fbe0) [1900c7c](../../commit/1900c7cf94e936a1df937b05dc0baf4acbe16984) [3f63f72](../../commit/3f63f725e1400c777e6492d99e5217671f6248ac)
- Ability to upload old scores with old ReplayData for future use - [7acd837](../../commit/7acd8370b2e8ff6d46c4d1b46bd94344d4e25a30)
- Additional Lua documentation - [fa1ca5a](../../commit/fa1ca5aef274daa68d71ba403b4ea25202419bbc) [3b48cbc](../../commit/3b48cbc75ce83579f140d1bd5377e0ceab2fe078)
- Assets Folder for Avatars, Toasties & Judgments so far - [fa1ca5a](../../commit/fa1ca5aef274daa68d71ba403b4ea25202419bbc) [fd46634](../../commit/fd466344abad1e8efaa4ddde8179c3653a2343ce) [da3b610](../../commit/da3b610d3cef6e906e1a390c102f3df94c6e3ccc)
- Eval Screen Viewing using ReplayData - [48b6296](../../commit/48b6296121c2d8ab43cbf4568d5d7667bdba3d5a) [3548df3](../../commit/3548df3e1204dc4f2cb8a61f86d46476985ab180) [1d1b166](../../commit/1d1b1660df5979a319bd8a2e88955c5e3f3e186e) [dbe67c7](../../commit/dbe67c763c391fc61009d27ac33e1b69f3a5764f) [0de270a](../../commit/0de270af1229bac3a0eabcbbb5a84893b8150c74) [e39eb85](../../commit/e39eb8573fce4433cfd95d62df35b4f5597e5fa9)
- Experimental Country Filtering for Chart Leaderboards - [c60c602](../../commit/c60c60240f8a299e9a40cfd25f302b686bbdef71) [70cceb1](../../commit/70cceb1f5042a97128c6e2735d32845d5f3390e1) [c03f8e5](../../commit/c03f8e5d6f70522c5282752859f308df4ada3915) [de14c0b](../../commit/de14c0bcad11ba745920413f4a7afd7b50e516c5) [efd503d](../../commit/efd503df5d641dac81d31e9b44e9c69c4828be08) [b2addd5](../../commit/b2addd54d79ffabbb6cc40c2b34d4ec421bdfbf5)
- Gameplay Leaderboard - [aa26758](../../commit/aa26758619959b822f4244fc21b395db1e77112b) [28bc91d](../../commit/28bc91df8a12967754a673478ab85eaa337ab8ba) [2f1e20c](../../commit/2f1e20c991f1451aded059e5051e147e7504c902) [f2d86da](../../commit/f2d86dae30440ead27cc0d0f281be32d6391766d) [7874fe6](../../commit/7874fe6b159de00473b7f52f0b6a8c76c9ee0675) [2469cb6](../../commit/2469cb627ee8ecf43a255b8a3bd4c5129802f827) [50342c1](../../commit/50342c1f274f32a0d2566e009a49ee40a7d74685) [76f56b4](../../commit/76f56b436e96c4bf9d3890cf142f96625a3b0009) [16c3e0f](../../commit/16c3e0f68b261a9ad595a0c5e498e7f10ea198ec) [928a80d](../../commit/928a80dabd26c605b60e39f45387abf5cb48a62a) [10f5d13](../../commit/10f5d13ccf6a31bfdaad21becdb79b52a4120656)
- Google Drive Pack download compatibility - [a78c95f](../../commit/a78c95fde7b30a611157f634803fb9d0f50b3d59)
- Lua access to the "Real Theme Name" & "Real Theme Display Name" if defined - [b91c54f](../../commit/b91c54f2d76c4288355f79fce10d2f5268a4d3c6) [7ed1e16](../../commit/7ed1e160dc0a15e70c2186644b861755b032030a) [58c89e3](../../commit/58c89e31708abef6710b83d9aa5f98f8cd0ba930) [58c89e3](../../commit/58c89e31708abef6710b83d9aa5f98f8cd0ba930) [6bb3150](../../commit/6bb31507a72688df3970dedb275b9774e79211dd) 
- Lua access to the Scorekey for local Scores - [e12bb98](../../commit/e12bb9886b5413f72a2d9cb6b5d2f52f1c133607)
- Lua scripts for "Widgets" which includes Buttons, Sliders & some essential in-betweens - [57fd70e](../../commit/57fd70e95826c55bebfd9c4d24848477d05c37f7) [aa26758](../../commit/aa26758619959b822f4244fc21b395db1e77112b) [51631df](../../commit/51631df490e6b7b29f2b10a4de1753b4ad11f180) [f59e608](../../commit/f59e6085afe2aeb621725d5b9c049faeec0f0ae3) [2f50d22](../../commit/2f50d22c23d07fe02238f63a575848294d4a2002)
- Multithread Game Startup (Song Loading) - [e4cfc38](../../commit/e4cfc380ed773f367f762685f78f5af031ad77eb) [fe3f644](../../commit/fe3f644a0972f1adb5a592ccc07f04f8cb36e560) [2c74750](../../commit/2c74750079cb0c0f78137546ebf488a6520fadc4) [c0c72bc](../../commit/c0c72bc2962683ed6bd928c618bcc9bfa1423888) [b8851a6](../../commit/b8851a6eb8de38582d1c5837df11f81fc10a0766)
- Osu (mania) NoteLoader - [3a0e149](../../commit/3a0e149a6c34fc09c1f5253826d520e0cf9a3753)
- Pack Download Mirrors - [4a06d42](../../commit/4a06d42871026814cf6f2dd411339f89f5e7f3f8)
- Pack Downloader has a cancel button - [7f0bb43](../../commit/7f0bb43debe4671f7be9566c4aed5b94918e4420)
- Pack Downloader Title Menu Button - [1eb7af5](../../commit/1eb7af5e7b760aadf3496ec894db7ebba1f567bb)
- Preference for Multithread Game Startup - [0738389](../../commit/073838956de1ce37ea7ced83a892d7750e0f9b35)
- Preference to tie Noteskin Tap Explosions to Releases or not (actually a bugfix) - [85bacac](../../commit/85bacac48cff8bf8782414c17de07040ff4c4d49) [bfde9a7](../../commit/bfde9a7b52f6c35a5a51f6a501198609cc0b8ef2)
- Replay Watching
  - Initial work - [9f35751](../../commit/9f35751dcd5cbf4c6357490efb3c5326202c07d9) [61407da](../../commit/61407da73e6ce40250bf69f2eb1864b0da7b9172) [48b6296](../../commit/48b6296121c2d8ab43cbf4568d5d7667bdba3d5a)
  - Buttons - [57fd70e](../../commit/57fd70e95826c55bebfd9c4d24848477d05c37f7) [51631df](../../commit/51631df490e6b7b29f2b10a4de1753b4ad11f180) [649b0f3](../../commit/649b0f33e893ccb02f18b53390744f579318bcd2) [46b75dd](../../commit/46b75dd20025258bf7144f211bb386071593d2d3) [384f5cb](../../commit/384f5cbe6f447ca26f575d3b67d793ef8c5ee1ad)
  - Miscellaneous work - [2f5d2d4](../../commit/2f5d2d45662a9c62d62b844777e34ff87f062fa9) [4cb2157](../../commit/4cb21577a0056be6b58988f5b254687be6d97865) [b4debb0](../../commit/b4debb0b06b2b75cb8149d7209572f6dd9415d1b) [dbe67c7](../../commit/dbe67c763c391fc61009d27ac33e1b69f3a5764f) [0de270a](../../commit/0de270af1229bac3a0eabcbbb5a84893b8150c74) [24fbfff](../../commit/24fbfffd83b62a49543452c4c499c6fadedb565d) [9e21aea](../../commit/9e21aeab2725db4dd365223527c209a3cc87d620) [d6644c3](../../commit/d6644c3a87517de273b659b5a21280da8e6c4e92) [56a02aa](../../commit/56a02aac2ba7e314f155d3eaef7ebd5b33d43f1c) [a986503](../../commit/a986503df6f1c5554a6fc5d783677ef976c47df3) [8147077](../../commit/8147077affd86d4e85a756d6d7fe928a6e22a76a) [a37f3a2](../../commit/a37f3a2ad1f88e79557bf4f4bf0a8b4b0b6e81d6)
  - Non-Tap Support - [0af2f74](../../commit/0af2f74f951f4d9ab46df3c3fa3492d8ad2ab492) [51c00c8](../../commit/51c00c8132f41c1e2f463a2b99b2c3e470f3b64b)
  - Scrolling & Pausing - [0769a4e](../../commit/0769a4e1ccdfa95b895c27a3ccbbd3cfa9a1ef23) [ec6c99b](../../commit/ec6c99b916b63717ddc06de726ee9d5346f5be74)
  - Support for ReplayData missing Track data (Chord Cohesion On Pre-0.60) - [63f753e](../../commit/63f753e293e768b42c569a0144835e44a8cbf8dc)
- Secret startup process Banners, Backgrounds & CDTitles - [8067cb8](../../commit/8067cb81a56e429f2503ffe3ae276af6d476c023) 
- Statically linked libuv - [590310e](../../commit/590310e8a1a0fbb74c8dc836fd4898cc484f943e)
- Theme metrics.ini NoGlow option to disable the flash with Sudden & Hidden - [f1c7e64](../../commit/f1c7e6445e8479783b635c58bf3c41cd5f55cf0f)
- Top Score/All Score toggle in the Scores tab inspired by EtternaOnline - [da97cb6](../../commit/da97cb6ccb29c73f001c49ee18ce9ad195f959a6) [a301581](../../commit/a30158122d4dbb56698b8bfea23fac8c14d461ec) [936f32d](../../commit/936f32dc97f256aa0c2c9f94da4a88acaec92667) [7257aaa](../../commit/7257aaa4eb239c5babf4a51469d7657888ce9fc1) [909d284](../../commit/909d28467b83e645672a580aa4e1fed3ec69a74a)
- Unique saved states for each keymode (Style) in CustomizeGameplay - [dc3209e](../../commit/dc3209ee7c80314ee61a7e37d4c38e07683cae54) [f1f9291](../../commit/f1f92913b9f0e794a834947a6cf0727005ea6a0b) [570b5ec](../../commit/570b5eca590a3335f11cff8ce4418c9d4314e036) [560f8a4](../../commit/560f8a48b8e41eac79af2345b2d44c2a4b7724e1) [6d893f3](../../commit/6d893f30b40fc1e5797a9d60157516ea1486ae2b) [7061b0c](../../commit/7061b0c3ca3b937a6517dd2635c5c769da782e08) [d87cc54](../../commit/d87cc5427fa95a7bb390b40b726c553c3b5a1866)
### Changed
- Actors can now Save and Load coordinates - [01a195a](../../commit/01a195a2864300a037a55bd09090fcc237c1e8f0) [81a0836](../../commit/81a08362a786561a72b8ddf51d89969b8743c929)
- clang-format was run on the source - [3b8aff1](../../commit/3b8aff1c5506024ff27c4b27fe63cbfacda6318a) [454506a](../../commit/454506a6750439bbfc19c7d75a424a34eb9beb2a) [ac4244c](../../commit/ac4244cface672f0f033509b9a1427dcbedbbc26) [ef30777](../../commit/ef30777cd491d0c2ac256ac708241573ee2f1b23) [ce3a867](../../commit/ce3a867c5d83e802e4535d629410018561ec9eaa)
- Chart Leaderboards should force update when entering MusicSelect to update an existing list - [982b319](../../commit/982b319914217c7ddfc09d74ba797903fd76c43b)
- Chart Leaderboards should force update if the Gameplay Leaderboard is on - [448c2b8](../../commit/448c2b8bd37ec387d497affbd2d5e0f7d9c9bc52)
- Chart Leaderboards will only request from Online when they are missing internally - [b405f04](../../commit/b405f0468679a21984a3d7a78e5a94a541aa4e44) [239250d](../../commit/239250d5db6aa09ca8e598c937866626c925ec59) [23c5f15](../../commit/23c5f150dc69904e2a1df232fc12ed143e12bee2) [0fe45c5](../../commit/0fe45c5c97e05fa3a6132b9319df99e42d5c75e8)
- CMake will default to Release if no build is specified - [1705fbb](../../commit/1705fbb43583373f6bbfff05ddc5214744e7d6ea)
- Default osu related timing values in the Custom Judge Windows are changed to modern numbers - [e5cf4ed](../../commit/e5cf4ed836d6dd63b2b6a0d35e95d8b80c3b69bd)
- Default Visual Studio Project is the Etterna project - [0cd39e4](../../commit/0cd39e4f0eef991696acc234672056758049bdaf)
- Don't automatically migrate all ReplayData on Profile load - [b333460](../../commit/b333460e86714cd20f6684d63e57e934f92902b9)
- DownloadManager downloads are GET requests - [90b136f](../../commit/90b136f01dfea788ddfa3918642147502de0d865)
- DownloadManager's useragent changed to get around the StepmaniaOnline curl block - [cdb1055](../../commit/cdb1055534959514768cce062a042d5813e5719a)
- Fallback settings Lua Scripts should work better with renamed themes - [054f387](../../commit/054f387ab69bf32a8787418b13d20d101316e311)
- FPS is not logged by default - [1b6ea1a](../../commit/1b6ea1aa93534334b23c5f07b23327971706e3f1)
- Guarantee ActorFrameTexture sizes don't round the wrong way when giving size parameters - [b7686d2](../../commit/b7686d2794108c056df4e5f255316b628bafa02f)
- Improve a Lua method to render a TextureRenderTarget by setting the TextureFiltering to false - [b7686d2](../../commit/b7686d2794108c056df4e5f255316b628bafa02f)
- Improve documentation for Contributing - [7a6e2f8](../../commit/7a6e2f8119817960147ac1e14d09101bb8695108)
- Loading window doesn't need to update so often - [e644dee](../../commit/e644deeb8dcffd95dcbabfb56d3bfc76c3b820ec) [e6fdaee](../../commit/e6fdaee37bc162433966d02eb025c7365dd3de8d)
- Map for Scorekeys to Scores should be public (screw OOP convention) - [91e6f5f](../../commit/91e6f5f06fc20d68d189afa0d4b968e242163b45)
- Message is Broadcasted every ScreenInstallOverlay Update iteration if all Downloads are done. Move this so it only happens once - [8bba406](../../commit/8bba40611f22697949fd974876fd67dee0428692)
- New default Lift image - [180483b](../../commit/180483b000a01c2370a8c2bf7c5e95beb39365f7) [dc79d47](../../commit/dc79d473e098aed78b7f9b1e0247358ca470522d)
- Numpad number input should be ignored for the tab shortcuts because some people bind those to arrows - [4ec28b7](../../commit/4ec28b75d84b2664e2e22394fcfcb8990380e0f8)
- PlayerOptions has had some utility methods added to facilitate some Replay work - [a986503](../../commit/a986503df6f1c5554a6fc5d783677ef976c47df3) [8147077](../../commit/8147077affd86d4e85a756d6d7fe928a6e22a76a)
- Rename the FastLoad & ImageCache Preferences to the previous names because people complained - [544a321](../../commit/544a3216bf6ac34bbabde3b8b26202370474bd6a) [218488f](../../commit/218488f49c5d867c870bdffe410e0db876c9d053)
- Scores tab should display whether or not the Leaderboard is being requested - [ae8e11f](../../commit/ae8e11ff23124bf339b07c986bb007b2cdecb1a3)
- Score invalidation by checking mods is more thorough by checking mods not normally available - [8f193a4](../../commit/8f193a4c1c6f0a3eaff7feb0918bdfbcd0eb6e49)
- Uninstaller should uninstall old or dead theme files - [907ca82](../../commit/907ca82664343eb22461412f16c5f80e41a8f3a9)
- Update Lua documentation XML - [7b179ca](../../commit/7b179ca6f21fe8eed0e2e0cf6239589626991c33) [afc1cd0](../../commit/afc1cd0968d6dedbc70091288ca33209a0d8c518)
- uWebSockets updated internally - [9d5e926](../../commit/9d5e92633886461dfba8cdefa2fdce72f9386e15)
### Removed
- Carry-combo Preference - [667a1d1](../../commit/667a1d1e26c04b7adf3dbcfa97729a1270db18b4) [b29f756](../../commit/b29f756d0d6fd0b25d6f7824b977ca764fa876c7)
- The Manual Folder - [8411c5a](../../commit/8411c5a6b2c17464d3a67b023680922c83f56af8) [aab9d3d](../../commit/aab9d3dc116a184cd5797633e57ab9afe4fc82f2)
### Fixed
- Bad Lua return types crashed Mac/Linux/64bit - [0f3218e](../../commit/0f3218e48815d1fd910d777cf2eb979f0cd2d052) [25a9f28](../../commit/25a9f2818ed72d2feac5e86fb487da0778cb6445) [893cb47](../../commit/893cb479163d4d3195bf694c42433d95e919e17f)
- Changing avatar caused the playing music to stop - [cd2a460](../../commit/cd2a46015c5bfd7aeec6301adcb508c13a4d52c4)
- Coverity Scan detected memory leaks - [900aaca](../../commit/900aacaae35f054c9dc993268e389ba0a0b6387f) [993933b](../../commit/993933b39d93e766ca95364106c3457c22ca80fe) [bad57e7](../../commit/bad57e75b0786a3119426368f0d9475110d76e4e)
- Coverity Scan detected null pointers - [8f76c32](../../commit/8f76c329c49becda8eddc3d9e4ada6f48e08712e) [2bcca2e](../../commit/2bcca2e085351e6f5ace26dbeb32852578617610) [14d9d64](../../commit/14d9d643801c7359b83894dcf735c8f48b153a18)
- Crashed on entering the Advanced Options - [fc0cf22](../../commit/fc0cf2213f26061fbc73526fd865dfeccb1f9219)
- Crashed when changing audio output device on Windows - [c444cbf](../../commit/c444cbfb01771f723489437d3260caaf3752db32)
- Crashed when loading old Profile due to leftover Replay code - [02d48b7](../../commit/02d48b76ff7bb616df9d92d94dd11d6d4a37c1a8) [e88a666](../../commit/e88a66644517eaab05826658c1c2d115cf07a238)
- Crashed when pressing Player 2 Start on SelectMusic - [9da774a](../../commit/9da774ab1d5421b517eeb74655c2716636368081)
- Draw order for Favorites, Goals & Favorites was weird so the layering looked awkward when all 3 were active - [de9c98d](../../commit/de9c98d7ea908e14a3ec5ee470fa320c84702a02)
- DWI files suddenly loaded no Notedata if changing offset - [ab4b04b](../../commit/ab4b04bf0b169937d25da1a65f5ea9b400712448)
- Exiting a non dance-single Style file filtered the whole MusicWheel to that Style - [c4731a0](../../commit/c4731a08cbbe558f694c78c83dd1699d752ed0cc)
- Function used for making hashes on Linux (getCpuid) didn't work - [09d88d6](../../commit/09d88d68fbe91131122ec7c88760a22c776b79d2) [4378f4c](../../commit/4378f4cca6bd737540a2ec16a21ec2ecb3eaa85d)
- Gameplay stuttered when logged in due to DownloadManager updating - [4ee14f8](../../commit/4ee14f8faff142e1d870ed2a85bc62d728f4fa48)
- Loading ReplayData from disk just crashes the game if Hold data is present - [26b99ce](../../commit/26b99ce8c463c81d8f805b4de8fcb40d025201dc)
- Lua error in Evaluation Screen broke Custom Judge Windows - [b404c6e](../../commit/b404c6eb5214502a696c02998f60d8fea6cddf06)
- Lua error on Profile Top Score list broke coloring - [b1e7ddd](../../commit/b1e7ddd81c715469b878c7d0e575150955de25c6)
- Mac mouse appearance or movement might have been inaccurate - [0e603b3](../../commit/0e603b3e9c2e627f98c82252dd60f005f153f22b)
- NoteField movement on the Y axis was not symmetrical when switching between Upscroll & Reverse - [536845c](../../commit/536845c93997136dc762c381dec07a75e4eb6ca6) [6c31207](../../commit/6c312075894d0e60c2a686302d2d02af783dfb6d)
- Opening the Offset Plot in the Scores tab caused the playing music to stop - [bd0fff5](../../commit/bd0fff5ba610000f6312d7cda960c0a51f280cb6)
- Pack Downloads sometimes ended up in the root folder somehow - [34effcc](../../commit/34effcc61ec92ab5d2000c2d15d65027221b5559)
- Some Screens indicated math.mod was missing from Lua (nil) - [48b6296](../../commit/48b6296121c2d8ab43cbf4568d5d7667bdba3d5a)
- Space didn't work on the Pack Download Screen - [5140c30](../../commit/5140c308220515e68ddc6731037a779787b5e70f)
- Starting Gameplay on a file with no valid music path crashed. At least try to reload - [f43a7c2](../../commit/f43a7c2ce52cbdd387f9354e5841a92fc52bf39b)
- Sync Change Overlay disappears once and never comes back when a sync button is pressed - [53f07fa](../../commit/53f07fa247e13057e7eb9ea658487b423922d9c0)


## [0.60.0] - 2018-08-18 - Online Integration & LuaJIT

Windows and Mac installer release. Multiplayer is functional but private, so the option is hidden in this release.

### Added
- Ability to remap the RestartGameplay button - [2139a45](../../commit/2139a450d562a3ef23f9673b4eaf70be1f9aac5e) [44def90](../../commit/44def901bfcbbb4ecf9f43b0755f513a539ace3b) [b84d52a](../../commit/b84d52a4880d76d2a9949341e5555091e1aa6666)
- Appveyor will build Windows installers - [7889aed](../../commit/7889aedad75117d5d01c89c2f737854336c2ced4) [226f225](../../commit/226f22570826022b0335d0a93192d46fafb7afa0) [2e20c42](../../commit/2e20c4202f5284156fc704db73388dd5372a4a99)
- Bug Report Title Menu choice - [ba21ec5](../../commit/ba21ec5376a8d4a176b61f01a31fcdaa353acfc0) [6ae0e16](../../commit/6ae0e161af42c35884320bd9db3a118de73bbd74) [6f391b3](../../commit/6f391b38f8a5935801e991e002ad08d5e43fee4f) [791582d](../../commit/791582d64e08f3d804ce32e9b0a6387728f3c7b6)
- Clickable difficulties - [71ac7ff](../../commit/71ac7ffc33edf396bd0798d9bc2bc7de42f3d9d4)
- cmd() format removal script - [5bfbdd1](../../commit/5bfbdd1946bfd5aed1bd9a3be01c93f00ea61eb7)
- DownloadManager has had many major additions
  - Account for cases where downloading may not always work - [5eb0981](../../commit/5eb0981e72788f09f64eba9a744767a0f2d42d4d)
  - Download basic profile info (Name, Rank, Top Scores) - [25d6c75](../../commit/25d6c75c4b90a4be1b1f33955da33da05c032301) [91cd70f](../../commit/91cd70fcddef3a296aab1a886ba2792006e13f6f) [b292be9](../../commit/b292be9a43ce443ead525b24c728ded586fcfe76)
  - DownloadManager clears out the temp Cache Downloads folder on construction and destruction - [826363d](../../commit/826363dfa3749f363785a8470705845e013c2509) [e1089e5](../../commit/e1089e530894fb8118411d62f344869aa4636a1f)
  - Download speed Lua access - [71716fe](../../commit/71716fe9768f9c78f380e3905bf3849bcd68f5ca)
  - Favorite Syncing - [f91cb36](../../commit/f91cb36f06d36fd4c1cd8813589ec0ce54a49ecd) [dd3a4e3](../../commit/dd3a4e330c18e68bac958b255f85dc6173fbbe04) [708a543](../../commit/708a5433bbc46e816c2d9f1d52656e076b73b378)
  - Goal Syncing - [16026d8](../../commit/16026d880a050776cc95220fb997d1bac234b744) [ab3185d](../../commit/ab3185db2f0397d865d7ee9c14e6223789743b49)
  - JsonUtil is replaced by nlohmann_json - [1f6f088](../../commit/1f6f08891aa7b8be9f30498549a0151c41189ce2)
  - Links followed by DownloadManager are allowed to redirect - [ac60041](../../commit/ac6004166e16faf678772c0abacd46ef632e37fb)
  - Logging In/Out - [f95c698](../../commit/f95c698d82987a59e7fd7f42d69c3d3039cd3cc9) [b7b7550](../../commit/b7b75501e9f01faae92ad660a901af1e8ed2a4c7) [a169c22](../../commit/a169c228a804c0b4f5a16ce8d8fc9fb0c06d68dc) [26bb418](../../commit/26bb418115c8e2d178019aa544589bad2c8ed35d) [078139d](../../commit/078139d0276f822b63ffee9b3139348236e8473d) [fa9eb01](../../commit/fa9eb0138654aa3f48c83d00ca92bfeedfec4c3e) [90c0ef7](../../commit/90c0ef734a5bfb11f3547770447ea7e8875290ef) [86a31db](../../commit/86a31db52241c00dbd78304903680116f624f28d) [0a2cebd](../../commit/0a2cebd27c1a2a4eb571fab6e5b40b53bb0c5c96) [1e8290c](../../commit/1e8290c6fcee99caa58786956b02a5b38d96fdef)
  - Login Token Requests - [8b79096](../../commit/8b7909699fe8a8bc0cc92552707ea27d92b4bbef) [1e4cd76](../../commit/1e4cd763a88f008474c41882267dc8661a72f191) [90c0ef7](../../commit/90c0ef734a5bfb11f3547770447ea7e8875290ef)
  - Lua access to a link to make an account on EtternaOnline - [154825e](../../commit/154825e85890549e474c16c12181009be6a92c66)
  - Lua access to see what the latest version of Etterna is according to EtternaOnline - [8b542b0](../../commit/8b542b03a498c19747d39438ed4d9ac798c3a7c8) [21461f2](../../commit/21461f2edae2cb2966f7b991ebf740ef39c2b46e)
  - Mid-Gameplay Downloads - [03524a2](../../commit/03524a2d16e650da0706a2de49fb2216302254d7)
  - Score uploading with ReplayData - [f95c698](../../commit/f95c698d82987a59e7fd7f42d69c3d3039cd3cc9) [bfaaeb8](../../commit/bfaaeb864fc6bd23383b5b33860f85dd305c5c21) [71b8795](../../commit/71b8795250a925017d7a81fa9f671f1f82085283) [8d4b2ee](../../commit/8d4b2ee343cc12f0d76c2c28bf05ee0d5325af69) [530a57d](../../commit/530a57df7ff22fa5b0007216be8003b56ea80f3d) [c9fc19f](../../commit/c9fc19f96f6b47eda21b4d2949390c5f32c29378) [a169c22](../../commit/a169c228a804c0b4f5a16ce8d8fc9fb0c06d68dc) [c93e58a](../../commit/c93e58abc24e5db2a1011327b532555f9c105988) [12cf2f2](../../commit/12cf2f261af6cbad3d6053253d22b0a261802591) [4def248](../../commit/4def248bfc681d429a3c47486872c2af07fc37ab) [bf3ce3b](../../commit/bf3ce3b602a657bc9c28788a39f911455d76f681) [3715c9b](../../commit/3715c9b855d194f488ecb63927969a3fcdc9a097) [f528375](../../commit/f528375da2725e7672260f8ecf44b6a74193bed4) [e9046db](../../commit/e9046dbcdf2048fbbd713a4b896efe2756c4498a) [e079908](../../commit/e07990844f37d7c3dbd2d7499bdf4e221eb6378c) [2c8408b](../../commit/2c8408b6f26538dbd14f30bfa8a22c02db8adbfe) [6012920](../../commit/60129205c54d5ebd1ffeb9746682c882bfcf0b44) [c880bfb](../../commit/c880bfb1b249581ba7b0545876e8737d1ede3633) [acc38c3](../../commit/acc38c33f26a1ba7b5a6307c9656acbc597aaa61) [1e83deb](../../commit/1e83deb026f4625e716fd2ccf0554ba4a4fef184) [1e4cd76](../../commit/1e4cd763a88f008474c41882267dc8661a72f191) [2962754](../../commit/2962754abbb60223d17a1a815e05d76866f49c84) [d2ddcbf](../../commit/d2ddcbf58a2a4df6e3e659758f2fa1708215e9b5) [f3a568b](../../commit/f3a568b1220eafd6342de93c19fa79c5f2ebf26e)
  - Top Score per profile list retrieving - [39dd6b9](../../commit/39dd6b9822980158457b4d4c5b2846ac25ca9e37)
  - Top Secret Client Data Upload Key - [bd63aea](../../commit/bd63aea70d9c19a60f41c2811b4f489bf9a97915) [98070cc](../../commit/98070cc07e064ae9bd2d225bab6acf0c8295d620) [7b063de](../../commit/7b063de02d257428133dd211428252f2b44d7380) [84a0328](../../commit/84a0328b3342d9f0e28afba0866bca26b52989e2) [db3b63e](../../commit/db3b63e6fda49420ef832b3fc14e983ea80d0691) [b3172a7](../../commit/b3172a71d6c5a716f682f44e5f0db224299bcb19)
- Exponential Weighted Moving Average option for the Error Bar - [2344255](../../commit/2344255485dabcf1a887989a4cf730242dd807ee)
- Filtering by length is now possible - [26559ef](../../commit/26559ef5cd8982ebbbbc00a4beffcbb015804a36)
- Floating Score Display - [2e3bf2a](../../commit/2e3bf2a86518d97802f30677a700f388e41906ca) [e2d465a](../../commit/e2d465a914eaca53fca17068f6320bd71dd4fe84) [9e3cdc6](../../commit/9e3cdc682901167095e07031726c4ad56e1a1a30) [e7cb98b](../../commit/e7cb98b6c91202d019d6bac5cdd516ce3b62f640) [8ee53cf](../../commit/8ee53cfb9dbe7572e9959c9bdcdb1111896263cb) [a5cf24e](../../commit/a5cf24e0d4a73ac1ae8124bbea37296669bc96a5) [09adcd9](../../commit/09adcd97c7aff7592b3277e981d041280ce918b1) [b1cab8a](../../commit/b1cab8a92a02e921d4649579cb65a9a18ff1c9c8) [47822b1](../../commit/47822b17d71773f53fd29a10f49da64537929d91)
- Game Update Button on the Title Screen - [8f2d145](../../commit/8f2d145bdd883c1939a0a92d3cb93a76e3254a64)
- German Translation - [ca6ee26](../../commit/ca6ee26edef7fb3dea7449600f87045816e958be)
- Libcurl - [609e5df](../../commit/609e5df614e4a86ed5a133f169f2db90e8c31ad6) [a553327](../../commit/a553327551dafd1032203fad540c943248f2c1c2)
- Linux mouse support - [cf67599](../../commit/cf67599772ea0d72f68c7733964530ab045ce3e1)
- Lua access to all ReplayData info from a HighScore & PlayerStageStats - [d935125](../../commit/d9351258646d0ec2a1a0a451a011f433a38e9ebe) [8d21265](../../commit/8d2126522d6d89ed34a7967c2455844177647080) [2c12a83](../../commit/2c12a83f240f098268f09a1d757fd96713afc333)
- Lua access to the directory of the Etterna executable - [82dd344](../../commit/82dd34449cebeea3ca51b2ce5bb61bc081ad0a43)
- Lua access to Differential Reload & MusicWheel:ReloadSongList - [14a2059](../../commit/14a2059c82161ea6229d1bb26ddde532ae1dcc47)
- Lua access to InputFilter:IsBeingPressed - [82c4cc3](../../commit/82c4cc33531a83f519569c1eb9bdac65656ecc70)
- Lua access to NetworkSyncManager - [69503d2](../../commit/69503d22e0fb80e021ca6c86b18d154d081c0a83)
- Lua access to setting Judge Window - [11cbd6e](../../commit/11cbd6eba44d242ae6645035b0f46c35e96f8620)
- Lua access to see downloadable Packs and check if they are queued to download - [6e13714](../../commit/6e13714231bf4e9524e311980dca14ad162216f9)
- Lua script to get a color by Grade - [ff347bc](../../commit/ff347bc534a073e30002d95430fdf6eedbee5da4)
- Lua script to get a color with alpha by Judge - [28e7ae6](../../commit/28e7ae69e01c87a11df4e98845900e7cf27aa33a)
- Lua script table with default Judge Scales - [b5f1d5b](../../commit/b5f1d5bb21341785ef5423d85262ac4bae461667)
- LuaJIT replaces the Stepmania custom Lua 5.1, so cmd() is now gone - [23a92e1](../../commit/23a92e1d8c7a02fe44d15ed6eb5f5c4a904587e9)[60da8ed](../../commit/60da8ed13c87a19bfdc43da98b55fc81f86215f0) [5a761f9](../../commit/5a761f9a33063293179b7ef242dd82ccfbed6ca7)
- Mac build is 64bit - [715f689](../../commit/715f689ebaba0922bd92748dc0ff65e55ea9479c) [09f0461](../../commit/09f04619d4c508ccd575ac46882cf7253c57f3fb) [92d9cf2](../../commit/92d9cf2449f962170b29d41a4b1b2a1f3f5c5eea) [d3b29db](../../commit/d3b29db99f027bdd9e38d57641a023762b29f586) [890276f](../../commit/890276f56e37b75c0481e2ce8fbbe3c1c6fa08eb)
- Mac build is deployed to Dropbox from Travis - [2d03a81](../../commit/2d03a8163f5d10c6b569eb3c92543677c910e82e)
- Mac mouse support - [ed31534](../../commit/ed3153467750f1dedff222b14bd419669497af7a) [f5f5069](../../commit/f5f5069c44c2006fc1d8d55651596c5fc6ffb20b) [fc1971d](../../commit/fc1971d772210089d37780b355bdddf5f9cd33a4) [22fac3c](../../commit/22fac3c4dee3a9ed55d29ef56968ba959d6c3d8d)
- Mac mouse scrolling support - [a54abd3](../../commit/a54abd3d5a4f56e43f83cf3d05a30e5e04dc2305)
- Mac support for DiscordRPC - [07179b6](../../commit/07179b655924eb5489aa1f2cadba901f76765a44)
- MAD Library is back because sync issues could not be resolved - [3c7063a](../../commit/3c7063a7c2a525b3d07b76d361070ca895d3ceb6)
- Minanyms - [185d68e](../../commit/185d68e9c83fa266bdf9fc8b74540aee13b954e6) [1089438](../../commit/1089438cb9b181bb33c5e9b7e33ad7875ea96401)
- Multiplayer Chat Overlay - [69503d2](../../commit/69503d22e0fb80e021ca6c86b18d154d081c0a83) [8c99ae5](../../commit/8c99ae51f335ca1bc19afea49e17dcb7300ec52c) [5c3d7d8](../../commit/5c3d7d80cc1b3ba97497b92f6ec54fe7f8fc80d6) [1947373](../../commit/194737300d16534c39ec8c985f55a0cb1c88a034) [96bf9c5](../../commit/96bf9c5a56a7cb557b84348a51c75be9462c8cf6) [ad2c68a](../../commit/ad2c68abf346cc5ab5eab1aa8f0a1b424c929e03) [cff68dc](../../commit/cff68dcbf6ba46b52a89a3914afca28f43d4d3b1) [e64f811](../../commit/e64f8110fa55eb87c6a478373ed8bdcb1480e093) [1b8c1a7](../../commit/1b8c1a76a1498385432503ab52781c9adbf82102) [15052af](../../commit/15052afc3d0156c49e30b16e5d15e12e0cb0d086)
- MusicWheel can sort by MSD Skillsets & Length - [e53de21](../../commit/e53de2188664ec44caeee3b8a35e15b1043e7ef9) [8c08e92](../../commit/8c08e925476a0e51715f226e86bc9d4c913fd6cc)
- NetworkSyncManager holds a new Etterna-specific Protocol (ETTP) - [8c2dede](../../commit/8c2dede13ebda5d7533bf94a6fb8086d8af22327) [47b3fe0](../../commit/47b3fe0e87163c4e4066c31f660491102a41380a) [f6f049b](../../commit/f6f049b36ab3969f6c7f42bec3c0145fa43649b4) [7027dce](../../commit/7027dcebb792cbb1b3c10d6f7688996169995207) [b323c87](../../commit/b323c87e300ee268eadcc5fd81a48fe43b67fccd) [759e508](../../commit/759e5085d065870cabb3f8b1840962d12fc37fb0) [c7adb6b](../../commit/c7adb6b6215db43a88d8f399cb0e70c7ca03aafa) [e79f694](../../commit/e79f694afe288fdaa401a38ffe0d2a9008f55e3d) [3f0bd80](../../commit/3f0bd803bde00e0deec85b3aac01cc4df5b35e4f) [de40825](../../commit/de4082575d41d4372ebbcf0a37a0f83c7ff6f18f) [3a32cd6](../../commit/3a32cd67a4b49e70534e6fdd9f212c25b518ab24) [52b27cf](../../commit/52b27cf45d392312c72d85cbcbbfc6054e6d9660) [35d5843](../../commit/35d5843665c146c3460b5e42a6e191ee8dc2dbbe) [5b36262](../../commit/5b36262a63aae9a89368ade74db44f2136fbf6c7) [4d5eabd](../../commit/4d5eabdd45318efe9f23be83edcc429633630d05) [14c5e2d](../../commit/14c5e2d594896cee1a5e5383a53b750d7110d948) [21a5b19](../../commit/21a5b195d3cc576f4e8332fa5dccae818a70cd6a) [21a5b19](../../commit/21a5b195d3cc576f4e8332fa5dccae818a70cd6a) [a075a95](../../commit/a075a95f4b4e39746bbb90f3090330550b4b68d1) [add9315](../../commit/add93158ca49adb6f87552583bb2df8915c10e48) [a4b7cb6](../../commit/a4b7cb6e88e6660387acd43b763bd0a8305cdbc1) [ab642cc](../../commit/ab642cc8d85af217d50dc6ca9f2f87ac844a4422)
- nlohmann JSON for Multiplayer & Online Integrations - [90a9cc4](../../commit/90a9cc499abc4d46d43d07319122ee08b8ca9d67) [f6f049b](../../commit/f6f049b36ab3969f6c7f42bec3c0145fa43649b4) [7027dce](../../commit/7027dcebb792cbb1b3c10d6f7688996169995207) [65bfade](../../commit/65bfade628049c14c9176ab995c7bb0a2eb85dbc)
- Noteskins have access to a few more Lua Messages & visible features - [555a283](../../commit/555a28374fdb0f7f14344d5bb8a3ba60285371b2) [b3831ff](../../commit/b3831ff0848ad1c2aa5f7afe90b08dc5a27c8a6d) [fc01eef](../../commit/fc01eef8cc67cb623827cf1ab756976353e47823) [dcff549](../../commit/dcff549825d557b76b6c678cb8c9bfe5409322f7) [936b818](../../commit/936b818be46de4630faa36e534e64c4ec96cb327) [b05c00e](../../commit/b05c00ee4afa39b3c99947d5afe91aba4e3b3f8d) [ced35c7](../../commit/ced35c73ac00cac080861f64ffd8bdcec7ddef84)
- Noteskin Previews & the Til Death implementation of them - [a001db1](../../commit/a001db17c68594feba8b934cb61e1fd0e47ab711) [1505d85](../../commit/1505d8510cb8335f5ed0c9e057c7636b365c8a52) [ebb62b2](../../commit/ebb62b28ad0d787bf3e646f7bf37be5821860e37) [7555024](../../commit/7555024adca6e68edadcbffbf44771c34b88f0ec) [90dea77](../../commit/90dea7791f231d27c499cedaa742649fc8c82626)
- Other Online Integrations
  - Loading window indication of attempting to connect to online - [c3f6aa1](../../commit/c3f6aa170576c537cecf7a3d8dcee7eccf3ee4f3)
  - Profile tab shows Online Top Scores - [39dd6b9](../../commit/39dd6b9822980158457b4d4c5b2846ac25ca9e37) [c9fc19f](../../commit/c9fc19f96f6b47eda21b4d2949390c5f32c29378) [862c0ef](../../commit/862c0ef87955a7c3bf70156e154c77e4452a95d3) [5d072e2](../../commit/5d072e296a8d59d950d9880fbe5cf4cefef3b17a)
  - Display user rank - [25d6c75](../../commit/25d6c75c4b90a4be1b1f33955da33da05c032301) [c9e9d79](../../commit/c9e9d793c6c0e8212dfee75dd369524fa33d00f6) [24d5776](../../commit/24d5776e22051ae1dc666e7f25c6c25f8af0b65f) [515bd35](../../commit/515bd3506ec992b270aca88d68fa5e0e39fd61c2) [df7a186](../../commit/df7a186b5fc02b81262e25a7d2ea9484a1709fec)
  - Asynchronous downloadable pack list retrieving - [f571e03](../../commit/f571e03cfc5a97ef25a61d22b859765bf15c2225)
  - Core Pack Bundle Downloads - [db70d23](../../commit/db70d23fe230f884d6dc7538ecf37f4e95cb8cc3) [36b9c25](../../commit/36b9c25f71d6f7e90b2447a248533ecba602e719) [2394f91](../../commit/2394f914e6e3670cc01cbedce5e300e8395436a9) [9e9ff48](../../commit/9e9ff48547957ee22d2fc8ccf29d5a30df54961f) [8b79096](../../commit/8b7909699fe8a8bc0cc92552707ea27d92b4bbef) [7f8ccd9](../../commit/7f8ccd92f67e84873c91668a06fa47cdb4ea1448) [cc42c9d](../../commit/cc42c9da1e40271f5ae3ee8449ac5ce6566b91b3) [aba15f5](../../commit/aba15f5e61f26b90415925b00166fce894a468f9) [d8a7575](../../commit/d8a757575a658f758ee91c84503e17135f9852e5) [6c5cb16](../../commit/6c5cb1628727cc48db68c2f1b743eb5bd77f4149) [77d12c6](../../commit/77d12c6e1da59ebb8132ab0cf09de17e3c924bbe) [ca4a953](../../commit/ca4a953b9e4c48eb3a9dd5ac6e65a902806ac0dc) [59d83bd](../../commit/59d83bd4a5b538a74d283328af45383e9ddce0c6) [8bc8c66](../../commit/8bc8c66af51e6faeddf81ef0a711134de78d784b) [9344bbf](../../commit/9344bbf04d9940aa82be259ddffb9c647ed9ed93) [8a4cb0a](../../commit/8a4cb0a151ad82c711ff13ebc5b948db34bbb8f3) [a1758a3](../../commit/a1758a3b869b5aab7c8ea10ec19105d89e08ae90) [7284467](../../commit/7284467532a83ad3d85fa18617b552cf994ce43a) [d1b95ce](../../commit/d1b95ce2aeb6c141bfb7251a2d826ef8fb3b3705) [0565100](../../commit/0565100f2da0ed5407036002d116c1c99f3f1818)
  - Download Overlay rework - [59d83bd](../../commit/59d83bd4a5b538a74d283328af45383e9ddce0c6)
  - Download Screen rework - [9344bbf](../../commit/9344bbf04d9940aa82be259ddffb9c647ed9ed93) [cc7d661](../../commit/cc7d6619b2020faed334a20f96121c94c8e2e7ed) [1f152ed](../../commit/1f152ed93b5e21b94e7d650f8e44497f7825b281) [d1b95ce](../../commit/d1b95ce2aeb6c141bfb7251a2d826ef8fb3b3705)
  - EtternaOnline API v2 - [def24bc](../../commit/def24bcbd6b16cbffd484ae04b7768ce500cd452)
  - Preference for pack download location - [97a0b5a](../../commit/97a0b5a7d521e3f799c63db81a61493ef262442a) [bc1d923](../../commit/bc1d923fb09d44b2d9b0fb52b386bcad92a61e90)
  - Score tab will show Online Leaderboards - [41f971e](../../commit/41f971e960a217fc402a8c922cc864ab6a2408d5) [74c6257](../../commit/74c6257f66348211bf5396ecfa9f0e527dbdc5f5) [0d00061](../../commit/0d0006146eb738da25bc0463b1875122485af61f) [57d63c1](../../commit/57d63c1a8469f1396e9e138625db70f44e7fc3f7) [2264b07](../../commit/2264b07439aa0707d582ac4f13208e821d2628a9) [73caf32](../../commit/73caf320f981f6cb3499b447d637242d369b12af) [aa51130](../../commit/aa51130a1e889f93dc265f7783a95003634a2296) [a3bdcf3](../../commit/a3bdcf374fb163a441920fdf01d9da06073e4fa8) [aa5f169](../../commit/aa5f169641a0c0f5f5db70fa08fdbf5effc31711) [769aad2](../../commit/769aad2ab0bd86520d6f6a95164ceac485c4070d) [84951fd](../../commit/84951fd610bf1d71a86b019dce7099eb2f86ebb0) [41716f7](../../commit/41716f7892da494da2e880e96846d17a7f750973) [fe4a4c1](../../commit/fe4a4c17129c14bc79e4ced4fe557c4742287198) [e3ce7eb](../../commit/e3ce7eb271d568feec3f8de60787d7a3ebb12670) [13d704c](../../commit/13d704cf493d30495a3d31bcc4f4d45f6c8264a7) [57dc187](../../commit/57dc1873eb87daf587f7843c63c5e32324c05cd9) [812396f](../../commit/812396f2b96e56f1dc46419e1692b037d1149f02) [a5cf24e](../../commit/a5cf24e0d4a73ac1ae8124bbea37296669bc96a5) [09adcd9](../../commit/09adcd97c7aff7592b3277e981d041280ce918b1) [b1cab8a](../../commit/b1cab8a92a02e921d4649579cb65a9a18ff1c9c8) [baefd3e](../../commit/baefd3e05a7c7578b93dd88bb99f34f5e12299b1) [58df91a](../../commit/58df91af26d820d9c795d7b67d19b24a7152173d) [b9e7ec2](../../commit/b9e7ec232e7411c548ef9a9296ce6479ebb26f26)
- Preference for connection timeout (for ezsockets) - [3e4b2e5](../../commit/3e4b2e54245b81f66cba3b593e19af2b8ea5bb75) [7cbd41c](../../commit/7cbd41ca6e49bf064ff61d7c1e8cae47150be328) [cb95bd7](../../commit/cb95bd73a078c506ca430a7c045b5ca7ae35ab34) [75067d5](../../commit/75067d55b76a24ca81ad225c62794ba2bd9d7ef3) [ed6ff64](../../commit/ed6ff64944fa301fc9d2bfb6961d51386184b448) [a4b7cb6](../../commit/a4b7cb6e88e6660387acd43b763bd0a8305cdbc1) [aee72cd](../../commit/aee72cd7da4c833208a18e6f809ce60f6f971813) [118995f](../../commit/118995fe8745f426e0daf9859149019bd25ce5c8) [02b7875](../../commit/02b7875fe1695682562bce28242b9a2386b640a7)
- Preference for logging ETTP related Messages - [fe7bba2](../../commit/fe7bba2b1a5525feec9bcefe82b0ba6d58eec591)
- Preference to load Playlists as MusicWheel Groups (then proceed to break it) - [619b75d](../../commit/619b75db89a62eefa51ff94905e5a7a174f05381) [f9982ad](../../commit/f9982adbff6cf6ccd580273019af48c9d3228526)
- Renaming Profile can be done from SelectMusic - [606da10](../../commit/606da10b50a57754d6b7f2f4b528bf4cbd1c1a7f)
- ReplayData will store into the ReplaysV2 folder and contain additional information about columns & Tap types - [198c255](../../commit/198c2559afd821ec8ac6be3156f0d14af4ee67ec) [2e9ffed](../../commit/2e9ffedf55f71a458563ce80cec07017952f0cb4) [a94a749](../../commit/a94a749918dc6131fbe4a6f5ec6908c0c1cd27fd) [7fd067c](../../commit/7fd067c3a9b03499a950005212b0b38a11bd3904) [035c6c9](../../commit/035c6c90b9f95c8bf8bf553caa19f5254816463b) [7159c0d](../../commit/7159c0d5ec7cb88294d3f4dffed7f1dc8940fa29)
- Reworked Goal Display - [108a036](../../commit/108a0368eac907f1e61fedfe60ac7fcb64d0bd89) [7c50eb9](../../commit/7c50eb92993b95824b3fda6239233db9480f3327) [331cdbd](../../commit/331cdbd0525863a02aded0044ad4ea1a354e715c) [647c9d6](../../commit/647c9d674b0d2599903f2150aaf21db91e9c2ede) [9e3cdc6](../../commit/9e3cdc682901167095e07031726c4ad56e1a1a30)
- SQLiteCPP - [dfd5017](../../commit/dfd5017bafde64fc3af81f0cc39f9c2bb0bae69b) [8488f7b](../../commit/8488f7b342b361c966e5efa6431cdd44740dc9c6) [e8c73bb](../../commit/e8c73bbbe50a785c1ee8c66684254144e20a620f) 
- SQLite DB Song Cache - [fa80c28](../../commit/fa80c285a613cefe85171f3e3ad0e243044f934d) [b2c1a65](../../commit/b2c1a65f860c555e18b03a1bcde5c8237f2f9601) [3b6940f](../../commit/3b6940f80956dadcbc5868ffa27a822dbe2af047) [477c3e3](../../commit/477c3e3cc7039c71168ccc6e2bf324eebe6993bb) [06ed2d4](../../commit/06ed2d4e276531e342de368d202debba56f2f6b1) [54198b3](../../commit/54198b3d7cb311018831c6715151837fbee9ba71) [78939ff](../../commit/78939ff8317003c319c50951af2dd96a0e82393e) [76ff23f](../../commit/76ff23f8a187b70621849c5a8bbe6428f8c25c75) [b465096](../../commit/b465096e8c285886096f4dad81af0a26d99e1b1f) [7c70b82](../../commit/7c70b82fd41491a3f3a24bc1733cdb16229b8a60) [9b0eeca](../../commit/9b0eecaeb0685c8b61fac4741758ddc66cd54a14) [fd6b34d](../../commit/fd6b34dc1c17a7bb6d3491f3136c421672284f5d) [b8d53ee](../../commit/b8d53ee056f9d1b01e2e5c06bb84ad7f79ce13b9) [a9302bc](../../commit/a9302bc4fed5be58eb241fadc87f829919d92247) [2f9d366](../../commit/2f9d366778fd5334b5eb161eb5a94d8d01215a5d) [500120e](../../commit/500120e17c475122eb7f635de63ce196c2365b4b) [75cd75c](../../commit/75cd75c326561862df87e40551214b7626049356) [02df71d](../../commit/02df71dc47a13da58051b29e4f06f3fca8f4f0f5)
- Tags 
  - Filter MusicWheel by Chartkey - [9ee6ac9](../../commit/9ee6ac9e2922e770124b18d0744b058f27d8154a)
  - Song check to see if its contained Steps match a Chartkey - [0f80a8b](../../commit/0f80a8bd2212ed67e51695724f8ca3aaa3e5bc0d)
  - Visual implementation - [e075d3a](../../commit/e075d3af68f28436ee4b457d0fd5a98ba860b8b3)
- Temporary Experimental Overratedness Ratings on Scores - [f0f525d](../../commit/f0f525d4c86e38ef42819da05541b7010df2e5f7) [660e45f](../../commit/660e45f2489da5ab0ba9b8b856945037ae4d0f2f) [37377c8](../../commit/37377c8b7974bf7546ff2d84c40e4b7c38a6f9d9) [7990e8c](../../commit/7990e8cfbbeebca7b66f3688b8ad3370898a552e)
- Travis will build Mac installers - [e47c377](../../commit/e47c377f112d441a5810adbce3b5d52680346da8) [d88f00a](../../commit/d88f00a092242220accd221b3847f0807f9ca05e) [3835fb3](../../commit/3835fb301855020d7a753f40424d8cf9c938539a) [d681b89](../../commit/d681b89d7df0de1730ef7beccdb1408440abb61d) [8b84e77](../../commit/8b84e7707f3a672cdfad9647d0eaba92a533a783) [198a676](../../commit/198a6766e841b9d0949311e872662fe9d7d8549a)
- uWebSockets for Etterna Multiplayer - [9175fde](../../commit/9175fdeb54d302eb4a82641236db8d658aaa9338) [19b822d](../../commit/19b822dd2926057df8473e928187f4467a1c879d) [010df50](../../commit/010df508572d5065f9e1b661c1b4a9f4ce2dd248) [59eb5ba](../../commit/59eb5ba6273de4e0a5e80ddd736242d949c87e39) [37d43cd](../../commit/37d43cd0564797cc776ca8ce4b9c0eadb60df0d1) [0e10af2](../../commit/0e10af290fb40bb9b55634238b7cee55d8c911be) [aa30abe](../../commit/aa30abeb1b64e6d6b245e87cd8a8b86541565a07) [097dddf](../../commit/097dddfb34c2c1783f32e5354977110af8ef43a2)
- XMLProfile & Experimental Profile DB - [0f1a9bd](../../commit/0f1a9bdef5b2fc3fe1f1f61dc42552b487ad1eb5) [bad58e3](../../commit/bad58e3298941637552402d1ddb2f5e351bcf5d8) [cde7526](../../commit/cde7526c12b9f8c0bb214577239c65ab88974b98) [7b253b1](../../commit/7b253b1bbd991706af03317c0e06bc4faa721ebc) [b7f85b6](../../commit/b7f85b62a839393d5e61057aa72bc5505aed5d46) [8c145b0](../../commit/8c145b020d19127c3031a20e07aff725bedbb46c) [11c3284](../../commit/11c32843962b5e3db64e930bf2558a1bd64efb37)
### Changed
- Aesthetic of Etterna has changed
  - Clickable Title Menu Items - [2a8a970](../../commit/2a8a970ba21445f0ccb083a93c8d042c1bcf4c33)
  - New default avatars - [9f8b454](../../commit/9f8b454d3688bb4687e50b180991bb35463011a2)
  - New default toasty - [a3821a4](../../commit/a3821a41bd6b24b16064ba7cafa641769ae7b0b3)
  - New Evaluation Screen Positioning & Track Information - [065f061](../../commit/065f0617fb4e96cbabec818744e1181bde774844) [e66004f](../../commit/e66004f8076349425b507507ab82ee8d009528b9) [26bbb18](../../commit/26bbb189ac2f4a4bdf809330c9840509f33757e0) [bd2a9a1](../../commit/bd2a9a1e0ea4a0d1aa175f2ea353dea230b5750c)
  - New Favorite & Permamirror Icons - [b76b019](../../commit/b76b0197e31d24e3058817dba22e635177959ace)
  - New installer icons - [2c02f35](../../commit/2c02f358724aff332a155d1a37035f25597f32aa) [6342a24](../../commit/6342a24cde9664ddb096d3478a893d89730e655b)
  - New Player Options Screen background - [1dc0361](../../commit/1dc0361bb4b1c044f54d76c951a1ae007f091a4f) [7d8816f](../../commit/7d8816fad9bd83200b36fd7ef41064984ad6e671)
  - New Splash/Loading Screen - [5de14f1](../../commit/5de14f1f1f159b5b53ccf9c9a65de4f6dcf95989) [ce28703](../../commit/ce2870356b6dda5eb74f255bdd37713c6499e676) [0a2cebd](../../commit/0a2cebd27c1a2a4eb571fab6e5b40b53bb0c5c96) [9863e31](../../commit/9863e31364036c05811562b0bbebdec59aa82b20) [6591804](../../commit/65918044bafeadd024bdb9e2fea7bfac926d31e6) [83f37ac](../../commit/83f37ac8cd95aff8038884fcc4bcaf50f3b347f1) [5e9c7b4](../../commit/5e9c7b49afce36224e3b6a451f76cf48655fa737) [fff0d9c](../../commit/fff0d9ce2d1c90f83ebe0753972cc096769a2937)
- After changes during this version, Song Search & Pack Download Search is broken. Replace the input checking method with some regex - [cdc5f7b](../../commit/cdc5f7b5fdc3fce24575679a7cd469c830008e27) [ad7b246](../../commit/ad7b246fd604e83c62e2305a44cbb251c8105704)
- BannerCache is now an ImageCache - [1fa09ea](../../commit/1fa09eae6e8942614d6d949fcad4287f75b94711) [f3b6bdc](../../commit/f3b6bdc0c8262dd401f30f174d4411230d53a4c3)
- Beginner & Easy difficulties no longer turn Fail off - [0cfee43](../../commit/0cfee43cff539bd13821bcf14a190fe56a70cbc3)
- ByMSD renamed to byMSD - [5075df2](../../commit/5075df241a3a0ca94fef0620bc04db32e708599a) [0914eee](../../commit/0914eeefe20fe51438898db85de8c45257ea5083)
- ByMusicLength renamed to byMusicLength - [1f9fe6e](../../commit/1f9fe6e2640b4c1b17632d9a6a39e353995ec611)
- Charts that are not 4k show the Difficulty number as the MSD - [7197a03](../../commit/7197a0341ccf04b0bc2fbe92883fa0c9dc27265c)
- clang-tidy was run on the source - [640ceea](../../commit/640ceea16c0878c200a1db96f242c1dd74d0596f)
- ColorBitmapText is now its own Lua-accessible Actor instead of being an element of ScreenNetSelectBase - [9d012b3](../../commit/9d012b33e476ad47efb59565d8357af90a143089)
- DiscordRPC is compiled with -fPIC for static library support - [3ba0600](../../commit/3ba060072022891881331ed98b72b7574dee9976)
- Don't crash the game when in specific states on the MusicWheel while using SelectCurrent - [10da38e](../../commit/10da38e3fb673055a90e04beb34109c27ca33b61)
- DownloadManager should not sleep for a full second when running curl download updates - [d7ac18b](../../commit/d7ac18b4a67694f4ac4f656d7871de29da298992)
- DownloadManager should update every gameloop iteration, not just at startup - [3690feb](../../commit/3690febf9ecb4326012cf331f26668b9ac7d0058)
- Editable.ini will change Profile DisplayName & Lua has access to it - [c06e17a](../../commit/c06e17ac986bec7a061a60c2c5101c28be6eef1e) [12a44f3](../../commit/12a44f3e9d98e5c8904ba70d17ba32a03d7769da)
- Etterna has a website - [a8ec099](../../commit/a8ec099f0fe97380661e9f3d3cc1e480753ebe7b)
- Etterna no longer comes with songs pre-installed - [c34436c](../../commit/c34436c5cb9ed8f77314f0a73a15a369652b5e2a) [c6c0646](../../commit/c6c064650d7b71f52ba5c7614c17c9c3999d0af4)
- F12 help menu is modern and correct - [d5372e6](../../commit/d5372e6eba4494e93d5944fb9e7c1048babf40e6)
- Files with exactly 4 notes are considered valid - [7aab3b0](../../commit/7aab3b044836e93c4e3a396abc33404a22f07645)
- Forget about mines when loading Offset Plots - [92cc75a](../../commit/92cc75a0ce46d4ffeb0eb2b2c405c1ace6e0a74d)
- Getting the best Grade for a Song is possible through Profiles instead of just the MusicWheel - [228da82](../../commit/228da82ac6c2ac91013825cc5abc1738ad1cfac7)
- Getting window sizes should be specific to OS/Env - [ca30a62](../../commit/ca30a620662b993816440a2ee966e715a20ed219) [8a62af8](../../commit/8a62af815d6a3b7345d2f52a58b5ff2d3b38c506)
- HighScore GetOffset/GetRow Vector functions should return references instead of copies - [46f73a3](../../commit/46f73a3318ad0bfb1b78e440e0d882d69cb543af) [a2ea43e](../../commit/a2ea43e859f487f416a8a5faf29241544236097b)
- Ignore string format optimization needs for single BPM files when displaying the BPM - [5b7e49d](../../commit/5b7e49d1300510975b335532010972e1dfbada06)
- InputFilter doesn't need 2 arguments in a function to check for a button to be pressed - [7330e13](../../commit/7330e1320893a51cf9fda90b0b9adfbd9299cb2a)
- InputFilter has 2 specific methods made for checking Shift/Control pressed states - [3962e55](../../commit/3962e55b44905d602ebc59ff2b6f399965fe38b7)
- Instead of crashing when there is no Style to set at SelectMusic, go to the Title - [8b7493d](../../commit/8b7493d04b14415feb2eac0265240c9adfca96bf)
- Lua functions for getting Chartkeys and lists of Charts are named confusingly. Rename them (several iterations of change here) - [02775b6](../../commit/02775b6c92594e074e70dc13933b64615191f88f) [a6b380c](../../commit/a6b380c1f71d19883d080f0088d3b18c819e5636) [51bc2c0](../../commit/51bc2c0313b73efdc4a18974c8ef0eab3ca312d5) [d1d0296](../../commit/d1d029605c7744697679b9e6693f73c0aacb8f67) [db5929b](../../commit/db5929bea69d5ccb85814aef2ed5aa9414be3792)
- Modifying Playlists should save Profile to ensure changes are made - [27ac631](../../commit/27ac631f9c2b0cdfa26cdcd19367019042b9f5e5)
- NetworkSyncManager has to distinguish between SMO & Etterna Protocol now - [8c2dede](../../commit/8c2dede13ebda5d7533bf94a6fb8086d8af22327) [b52c34d](../../commit/b52c34de87a7f6dec2c00bba7a7e07c628ed5fb6) [ccb60c7](../../commit/ccb60c719ce4f74aee9335b58eb8708918ea46ce)
- NoHolds force a score to become invalid - [ab16fb1](../../commit/ab16fb128dc0ad4002670d3707c6f77eb1572b99)
- OffsetToJudgeColor, a function to find a color based on Judge Windows, requires a given millisecond input instead of second - [81586e0](../../commit/81586e00e85dc66e3192c3cdc494668fad7e807c)
- OffsetToJudgeColor functions consider the max Boo window - [b05179f](../../commit/b05179f1910d9836727a6a934d1d964ed5cf4a6a)
- Optimize Lua for Offset Plots for various features, cleanliness & simplicity - [938f296](../../commit/938f296689ecc7bbd3e86af9499349618fb98c48) [9260cc3](../../commit/9260cc3afa79ebe0d0df523780d749b29fc24ce6)
- Optimize OpenGL more by using modern functions for creating textures/mipmaps - [ab42745](../../commit/ab42745fdb7a69cc68f6eebbc028f61680f0f238) [50656e8](../../commit/50656e8d068a35bf3e1e5bd6a8a33cb90cb81bde)
- Optimize OpenGL's RageDisplay methods for creating textures & ending frames - [6852775](../../commit/6852775b9296568a47ce013247757cea374c6247)
- Optimize Song Search by skipping sorting in some situations - [1836e26](../../commit/1836e265c01ad6c8ef61d20fcc4d725131a9a250)
- Pagination on the Goal interface is separated by filter/sort - [78ff727](../../commit/78ff727178bbd4f2d6d28bb0817bbeaad3aa544e)
- PlayerStageStats's GetGradeFromPercent will return a WifeGrade - [58ec71d](../../commit/58ec71dcb10cef1f81512324489687cc52ac4dfb)
- Replace some instances of Stepmania with Etterna - [41d873a](../../commit/41d873a3cf0df7e65d12d300fc253dfa3d9a14b7) [fae2db5](../../commit/fae2db5c63df5416d7652f0d887973197870dbf2) [8326bb0](../../commit/8326bb0bf4524d0538491448a5068fda6e4c08e7) [4e8b9d9](../../commit/4e8b9d969b27196eafeec7c5c524b60d35878a9d) [e95db8c](../../commit/e95db8cb13e6e572f03291dc42ab47ab3482e0b7) [b68031a](../../commit/b68031a3c7df399702df7359eeca046ce03dd8ce) [7791663](../../commit/77916638d16282676214d417bc30cbf3720ccc01) [3f97bc3](../../commit/3f97bc3abe1c51b97d056608c09aaffdfc42c817) [629f00e](../../commit/629f00e288a724ea61413d791cb999b4cf079f36) [c015665](../../commit/c0156650049722eafce7f23fbfdee5f78e7eb344) [bcc33c5](../../commit/bcc33c5167938231328f43202dbdf5955fba2eec)
- ReplayData is unloaded after being written - [8de1477](../../commit/8de14773821eb1704bbf0916c879a17ca9e097a8)
- ReplayData containing lane information is loaded with extra features in Offset Plots - [3a2a910](../../commit/3a2a91090bfec261193de49de261e466de91e6b9) [0c4d37d](../../commit/0c4d37d9c588b9c9f630c27be2c937e688ba3078)
- ReplayData not containing lane information can be loaded in Offset Plots with some changes - [a91d8e0](../../commit/a91d8e0e819cd8cb2637190e324fbaa72510669a)
- Repurpose SMZip code to open Downloaded Packs - [d2367b1](../../commit/d2367b1710a7887156d10ebf1637aeca37d3651f)
- Rewrite/Merge the Download Overlay into the System Layer Overlay - [20ef02c](../../commit/20ef02ceccbb61a3c4e157fe02aaaa454980dced) [04ebcf6](../../commit/04ebcf6b15efa0076564c93834fb275404ae8263)
- Rewrite/Merge the Reload Overlay into the System Layer Overlay, replacing Credits - [3618d38](../../commit/3618d383e8699b4bcebc7e8d2e5ba94b9e471304) 
- Rewrote many input callbacks in Lua & transition Commands - [4c27584](../../commit/4c27584aab410c3f43b3c4c8db275c7ef1103fc9) [a1b7a50](../../commit/a1b7a50a7b7482c4dac0f19f6c39c21abe4644c3) [0d37b00](../../commit/0d37b003243669da6efd0a06ba90f84c2cef2b4b)
- Score tab renamed to Scores - [5c5d919](../../commit/5c5d919df6a9ee435b9830664cd70227d6028319)
- Service Switch is now bound to `Ctrl` + `Operator` due to fat fingers - [57ce8e5](../../commit/57ce8e5be3deb4124019c7d7d445a2dbaed8bde1)
- Support for clicking to change avatar - [b4cad27](../../commit/b4cad271f4226f7637bbbc289d70786f07ac53d1) [dd0fb66](../../commit/dd0fb666ce9f6393043b5b059efb72d70df5bd26)
- TextEntryScreen should have a black background - [1b0eca7](../../commit/1b0eca76e541c557b27100a5a41ed50e0e8d968e)
- Text Shadows are back (previously removed); FPS is fine - [77a529f](../../commit/77a529fa794bcc63d9161303844af7a18a4301b6)
- Use getcwd instead of \_getcwd for commandline arguments for obscure Linux-related (?) reasons - [d67d984](../../commit/d67d9846189465b430b6511accb2e6418c723a12)
- Validation of Scores is more exhaustive by considering loaded Lua, Autoplay, and Mods used - [7f038c4](../../commit/7f038c4971671c57deb561196c59a461c796320b)
- Validation keys for SSR calculations ignore certain Enums in generation - [7835f51](../../commit/7835f51ceae0ad2087e87ba94c3c541da089d670)
- Validation keys for SSR calculations account for Grades - [2b471ee](../../commit/2b471eedd50c9a00ec263b480071d9155b883268)
- Validation keys use millisecond values - [cd35727](../../commit/cd3572754aa5faf64538fd9721842b2a560dc1a6)
- Validation keys use sha256 - [98070cc](../../commit/98070cc07e064ae9bd2d225bab6acf0c8295d620)
- Wife rescoring functions should not care about mine offsets - [6295fe5](../../commit/6295fe553d6aaf4df0220a8c8265a0e4bee9ff0e)
- When displaying best Grades on MusicWheel, don't care about Style - [aba80ed](../../commit/aba80edfe10e44194ad14aca2c7488906d437e17)
- XML related Profile management (saving/loading) is handled by XMLProfile instead of Profile - [f1fcdac](../../commit/f1fcdac212d87d692685e5f2631b7aebd5c7527a)
- XMod float values are rounded specifically to 2 decimals - [a27c7e9](../../commit/a27c7e90250011cb971a577e8d6b173ff3532daf)
### Removed
- Access to Multiplayer - [5d16124](../../commit/5d161249602fe15c7d34e71a65c323af8695d5aa)
- DancingCharacters in Backgrounds - [904abee](../../commit/904abee9f92185c27f9d84defaf792292b1517cf)
- Etterna Songs Pack - [9938baa](../../commit/9938baaf586b0f9ee7bb366a46d9082f2859b33e)
- Mangle the RatingOverTime function - [919ab7f](../../commit/919ab7f8322e87be0b8a92251f96a7f5b3549d7b)
- Many things related to PlayerJoined due to Player 2 support removal - [8c80bba](../../commit/8c80bba292656fe1aacd676eb277251d668152e5)
- Old hack for missing Grades on the MusicWheel when entering SelectMusic - [2953a03](../../commit/2953a03340f25689299ba342f7e85e90e0b3e227) [b670d80](../../commit/b670d8060df4294ef267a06b8636f75d6bb36003)
- Pausing - [2d2c88e](../../commit/2d2c88ef04c610862b47b69684fff18e2251632d) [2565009](../../commit/2565009258461f4214766c00b9a11ebf155523bb)
- References to Composite Charts - [24067c4](../../commit/24067c43728327dfabeca38da1991b9aebd93bcc)
- Reloading from the Options menu - [a54d374](../../commit/a54d374918258ccebdffff8dae5249d1f2e861ea)
- Stepmania-custom Lua 5.1 cmd() style - [0791d8d](../../commit/0791d8d032aa2f0048d6ee44c674e06c47ce657a)
- Upload Profile button is replaced by Logging In - [b7b7550](../../commit/b7b75501e9f01faae92ad660a901af1e8ed2a4c7)
- Uploading a Profile - [7e6487c](../../commit/7e6487c35b7101687b893179a7afeb3b8cdedb90) [439a919](../../commit/439a91968b9e1c3bbc7f0edb9e2add2c8fdd30fe) [e2bf44f](../../commit/e2bf44ffcb70917d36130271197e65025c9e991d)
### Fixed
- 2.0x appeared slightly incorrect in the General tab - [a86250f](../../commit/a86250f3a62a9cffa80a51946f88e71fcb6087d5)
- Boo window in Offset Plots breaks when converting Judges - [e5ba40f](../../commit/e5ba40fc63ee3a96b88d864ab3e31c79dcc2695e)
- Button to show ReplayData didn't work - [4d47698](../../commit/4d47698c2588b015a1caa2c63c59ec4913e92155)
- Changing the sync of a Song from Gameplay didn't reflect any changes after saving - [a623eb3](../../commit/a623eb399669ba78b4edcefc40ea921eec295c8c)
- Clicking after not refocusing the window gave wrong coordinates - [ae46c4b](../../commit/ae46c4b4408b1d9432fa9d434fba7a33222133fe)
- Clicking outside the Game Window counted as a click on screen - [a96d1dd](../../commit/a96d1dde1a2c135e915a4e5af5af8c8946cd0267)
- Clicking Start Game crashed. Fix this by ignoring the issue - [0e213f9](../../commit/0e213f9bf180b9e6b63182261341e09891782a11)
- Coverity Scan detected many defects
  - Null/Uninitialized Pointers - [b12757b](../../commit/b12757b4e3d554ac6948902e9815d77f8237a3d2) [cec97d0](../../commit/cec97d01d955e495d9643370fbf691f2aec14e2f) [8361385](../../commit/8361385720e8808e0c5de43a6b19ab6eb4ce431d) [1eadc40](../../commit/1eadc4093bad2ec0fc09025e8c8635c8ba59ebf3) [4be5af1](../../commit/4be5af11d939ef18d42935bab3978a16065e1485) [80b35c6](../../commit/80b35c60309c274e288034ee35f48663ffe3700a) [09dc617](../../commit/09dc61755be3d96cc7db4a78c71560c5aeebe3d2) [33bc784](../../commit/33bc78468f8993e18ae76effa18319c7f2dc78f1) [ea648b9](../../commit/ea648b9df92e4f6d722259d6c2af311e074de273) [2c1d3d6](../../commit/2c1d3d6fe74663197df188607083accbf4ae8d01) [1766eab](../../commit/1766eabb976621d8040abdd5c0cb7d8c460f8e01) [f9d7a31](../../commit/f9d7a3100b804898e7645202b9832cde9df6da75)
  - Memory Leaks - [dcc9295](../../commit/dcc9295a0e014486f6d242568da26f3ad2392d82) [bd8c6fd](../../commit/bd8c6fd77d3d8b0426f560e8533384f01d2d9649) [da3b757](../../commit/da3b757fa9c6543809a035a935ccfa39b4773608) [3c565ee](../../commit/3c565ee219651e2193cd8da611991c48c5ddf7f1) [24b5ab5](../../commit/24b5ab56ff7c11d7efb9662bb48e51eaaa1e6d6d) [eab76f4](../../commit/eab76f4800ab0b23771dcabd0ec35049e889cf69) [5821a3a](../../commit/5821a3a6ef11d6556db14451cce695029af70cee) [ff8d36d](../../commit/ff8d36dab7f45d7e47ef79fdf3af8f67a17b867d) [08ee76e](../../commit/08ee76e5ea83d86b66b45c341773d38483d7c357)
  - Misc - [228fc16](../../commit/228fc1642f2f71a84259c103919f49a722a56412) [9842959](../../commit/984295921aab3a9dee026af5331e767b8688dbda) [8dc236c](../../commit/8dc236c01053659bc75e3388a7d5c0f362105dd0) [c7eafec](../../commit/c7eafeca5628d97548028db6551f26a2c2ad0d4b) [bda2ce2](../../commit/bda2ce2b343db51b076ab14bae4e1d1f83ce36b8) [d5d89d2](../../commit/d5d89d2f16470a37fdaf859858e25ccc56661817) [bfafe52](../../commit/bfafe52ac63891d89f468ca156abc37eab256ea1) [822a485](../../commit/822a48567a8db3a36f47067d076ac7040f65f759) [bef2a68](../../commit/bef2a6892074005995240b590d815150888d70dc) [822a485](../../commit/822a48567a8db3a36f47067d076ac7040f65f759) [74516bb](../../commit/74516bbe6211d5d8f860c4820e8c8ab1e9679b53) [0e63b4d](../../commit/0e63b4dabf82dfdb2525239619c633e178d08e50) [ed51c01](../../commit/ed51c01ea80e962bf972676f3614aff3375b8290) [b950fc5](../../commit/b950fc551c94439914c16a6ee2cf5bcd7d97f852) [b7212d3](../../commit/b7212d316ae00914ba766ee1855bd8c07d08f182) [7b42be9](../../commit/7b42be9786edb6cd6cb84c2014f6607b7e24a9b2) [bdf95a4](../../commit/bdf95a453efa3dd9fbca1fba93c8d7c675138509) [fd3215c](../../commit/fd3215c3aa31542af8fc16dad24688266570b263)
- Crashed when judging Pump Holds/Rolls - [268b82d](../../commit/268b82d8a5742d708d5377447f2bd97b5dfff959) [e8900cc](../../commit/e8900cc37167a2852539f646a5117f4bf023ca94) [9bfcd95](../../commit/9bfcd950e8e2a4a00c335fee2335bed019deba2c)
- Crashed when making a Goal for an invalid Chart - [88024ef](../../commit/88024ef44105acada475c1a356147339d8a4cd51)
- Cursor didn't show up in fullscreen on some Screens - [ee97a5b](../../commit/ee97a5bf988ce4be62de368ce4cfbdb79915e891)
- Downloading packs crashed for packs already downloaded - [5d1f844](../../commit/5d1f8447ececb6220340e07982abc9c2b4fe5e39)
- Duplicate Goals could be created, which broke the system - [0d0402e](../../commit/0d0402ed1beedb4fc8e087747268228c3595ac6e)
- Edge cases still existed where Player 2 can join - [c624267](../../commit/c6242679f54d25c306f67134e913e5522b8067fe)
- Filter tab lagged very badly when numbers were left in the input and scrolling was attempted - [8507174](../../commit/8507174110022d0946c7d334939bb0ecfc80593e)
- FullProgressBar positioning was broken & its setting did nothing - [3805beb](../../commit/3805bebd30acd8a666939f2915ed048feda7cf45)
- Gameplay Error Bar failed to load in some obscure situations due to particular logic - [3128f6e](../../commit/3128f6e96a3d8e4f258e27cf57c2fc0ec8487fdc)
- Getting PB Grades for Chartkeys was broken - [4ba2cf7](../../commit/4ba2cf7f6c7959cce41359fbb9823536ecce1adf)
- Giving up in Gameplay sometimes gave score. Force it to invalidate and fail - [2e5dfd4](../../commit/2e5dfd47e8dd4016e5bf714073e37d9014104a4b)
- Goals broke when songs contained were not loaded - [e207279](../../commit/e207279e98c3f5f402d8a2ba248263302a6d3589)
- Grades didn't appear on the MusicWheel when entering SelectMusic for the first time after removing the old hack that fixed this - [b670d80](../../commit/b670d8060df4294ef267a06b8636f75d6bb36003)
- Half rates (0.05x) didn't get correctly factored into the sorting for the PB display in the General tab - [e09236e](../../commit/e09236ed5e8299dc383cd1a58e65a95f752f4b5e)
- Having multiple Profiles loaded caused weird/bad behavior with many aspects of the Game - [8fbcdc0](../../commit/8fbcdc04d746b7ca9632f86b7d27d6b77bb2511d) [5594f6c](../../commit/5594f6cdcc1f170c35304a85552849e8fa1e7525) [4cb2031](../../commit/4cb2031bce15c7745ae63d5d4b4f43dbcaa73cce)
- If disconnected from Multiplayer while in Gameplay, odd internal behavior could occur - [72f2c74](../../commit/72f2c74062f3d41f8bd8199ab801d06772098056)
- Image Cache was always deleted, causing loading slowness. Fix the Image Cache - [90939f2](../../commit/90939f2dd8e8f4fbc8cf3fd9cc45c746ce92948b) [f3b6bdc](../../commit/f3b6bdc0c8262dd401f30f174d4411230d53a4c3)
- Internal Lua interaction with Judgment Messages was broken with Chord Cohesion Off - [e103a9a](../../commit/e103a9a465a97d8ccad849a3622edf1d347ada9a)
- Key modifier functions didn't work for Mac - [9baf024](../../commit/9baf02468bee87fe30fd7e22bbc8c28789cdcd79) [3c5349a](../../commit/3c5349ac5920115c3bfa0f2110b6fa82bac3e858)
- Mac specific defines in code were incorrect - [f03d20d](../../commit/f03d20de19c337ddcb813bc173ed041539479712) [37cfdb2](../../commit/37cfdb291bc0a11397a642d94558bbda363ced2c)
- Mac/Linux crashed for reasons related to Lua return codes - [4461eb0](../../commit/4461eb09a15db3e3c8af4bcf2eb35866d09702c4) [7253309](../../commit/72533095fc9b24869cdc0fc066fb11b13e0f6393) [d6671da](../../commit/d6671daff8c6fd647fe4de231b3d928457b28d1c) [88b43bc](../../commit/88b43bc1550916825203f1e3f7e391aae488a643) [9daaa40](../../commit/9daaa4032d30553243bd2272e613c86461bde06c)
- Lag was reported when using LaneCover. Attempt to fix it - [13475e3](../../commit/13475e38f4a63bd3ce2aec38cde9aba5bd8e046b)
- Lane Cover didn't follow NoteField movement in CustomizeGameplay - [923d8af](../../commit/923d8af99ce8b86c4d60e8e55b443b66eef66983) [4739b0b](../../commit/4739b0b051082a70d7dc62ade1734f8c8de4528d) [43a66b8](../../commit/43a66b89560a8679a6f73406127d57b2fbd27026)
- Lift Notes didn't work after the Chord Cohesion changes - [23731a8](../../commit/23731a84aeae62325f9eb7b852863e214e22cc0d)
- Multiplayer SSR values were broken - [b73d93b](../../commit/b73d93bb03c365d6489ad4b0913f973534ce664e)
- NonEmptyRowVectors didn't consider rate when being packed into scores to upload - [71b8795](../../commit/71b8795250a925017d7a81fa9f671f1f82085283)
- NoteFieldColumns (lanes) were not equally spaced in KB7 - [78d5a71](../../commit/78d5a714c35aec4636da5340c3919beb8f91de09)
- Pasting from clipboard didn't work on TextEntryScreen - [3962e55](../../commit/3962e55b44905d602ebc59ff2b6f399965fe38b7)
- Playlists broke when songs contained within were not loaded - [e207279](../../commit/e207279e98c3f5f402d8a2ba248263302a6d3589)
- Playlist interface pagination didn't work - [b3ffdca](../../commit/b3ffdca5efaa8605a081179a737207587dbe98ff)
- Pressing 0 in Song Search changed tabs instead - [0e9a354](../../commit/0e9a354405dbff67bf0c4c1b3384d9228fe5f16c)
- RageFileManager_ReadAhead didn't work on Linux - [68625d4](../../commit/68625d4dddc2a665f97854376913ea7a67069f12)
- Rates on the Player Options Screen were out of order - [454c817](../../commit/454c817182068dd1cb47e4b6f338ad480c2d9b01)
- ReplayData had no misses - [0fe54cd](../../commit/0fe54cd6ddecb3adf68ff8d7453188875d67b3a7)
- Saving Replays failed when the destination folder didn't exist - [5ad7366](../../commit/5ad7366213d464fe7b430620f152fc2e6cda86da)
- Softlocked when clicking the Pack tab while transitioning to the Pack tab - [e240076](../../commit/e2400763df48b40717dc6d519970166614040194)
- Softlocked when redirecting input in Lua in some situations - [a366581](../../commit/a3665814f1533308a43a8f1486fc170bc738913f)
- SuperShuffle had a strong left hand bias - [b151f00](../../commit/b151f00c59ed323e188bb676ac1e8cb0162ee59a) [df2f0e0](../../commit/df2f0e01e91a71982519fe1e4d21a323cb6bc6d2)
- TestInput Screen did nothing - [3214b75](../../commit/3214b75298278f08a297ef7349b61ec0b4b6b370)
- Text Entry Screen sometimes held old entered text in the fields - [c7da123](../../commit/c7da123c355720eb2c1483594b7cf80d3084ac4a)
- Top Grades sort was broken. Attempt to fix it - [50d0775](../../commit/50d0775157a361903ebc9e77195b356b8d822ad3)
- TopScore values indicating which Scores are PBs in the right order were incorrectly set - [bdfe79d](../../commit/bdfe79d9fd1f6cbb9035228db35348df049589d3)
- Unexpected/wrong characters were returned when grabbing characters from input through Lua - [2cae11e](../../commit/2cae11e2a07c94e78a1a78e0c1190b6ca21c665e) [da7c4f0](../../commit/da7c4f09bd917f905a6e795eedee83c865fed9da)
- Various errors when there were no Songs loaded - [2f33a2b](../../commit/2f33a2bc8e43b71cce72c1d2c099ae284bb3b62e)
- Windows 10 had issues building in some situations - [a662005](../../commit/a662005ea6edf58b370196fb9552a01a195ac657)
- Writing ReplayData when there is nothing to write crashed - [58f8c1c](../../commit/58f8c1c8070776be2ba7f501783b95a87441d2e8)
- Wrong song info showed up when moving onto packs from songs - [a86de6e](../../commit/a86de6e81ba263b1093c19320f82cc3fdff93299)



## [0.57.1] - 2017-11-15 - Last Big MSD Update & Bug Fixes

Windows installer release. Due to merging, some features of this version and the previous one may be confused.

### Added
- Allow ScoreManager to purge scores - [8da6004](../../commit/8da60041ea588dcda204bf65877c9c2d47a0242a)
- Allow pasting from the clipboard in various places - [f0c1bc0](../../commit/f0c1bc0474bd035a25dfa753b8d8d444d9651725)
- Hilarious splash screen - [19ea4af](../../commit/19ea4af11c54b0498947b568e95132c1a435a35d)
### Changed
- Discord connection properly closes when closing the game - [31e0210](../../commit/31e0210af83f913d841304219032d2d9ec4c70cf)
- Don't care about input debounce for the mousewheel - [c5a97e3](../../commit/c5a97e3d2c209c0bbd12fd52e302992af669031d)
- Don't let Lua mess with internal rescoring functions - [2fc75f9](../../commit/2fc75f90f990e66e162ae6d6c772ca2c4d913866)
- Drop the lowest skillset when calculating Player Overall - [063f5e0](../../commit/063f5e0224fa4d97bb8d9027ccda6230fe3a6ce8)
- Initialize MessageManager earlier in the creation process due to undefined behavior - [dc484e6](../../commit/dc484e68bc792821b00c913a0a427e63522f21c6)
- JackStamina shouldn't be ignored in Player Overall Calculation - [2201b59](../../commit/2201b5928f6f2b81288c9f42200efbb72deef6f4)
- JackStamina has been replaced with Chordjacks - [3065bb5](../../commit/3065bb579eaa189f3b403d80734dfb94eb59711c) [d29f7a9](../../commit/d29f7a93563735cad9cbb363a5ceb7cfb3e8b840) [b80925b](../../commit/b80925b06070eb894abbcf4131fe59b8c6b39148)
- Multiplayer improvements to input redirection & room searching - [0b00b5a](../../commit/0b00b5a64bd8813a895182830708a1bba24bcdd7) [505ab41](../../commit/505ab4121293d0266886178b2ad36f286096c63e)
- Replays after Chord Cohesion changes go into a different directory - [02d839f](../../commit/02d839f1acbac4f15bdd3cc21786eacde47b88ad)
### Removed
- Levels - [0b00b5a](../../commit/0b00b5a64bd8813a895182830708a1bba24bcdd7)
- Unused theme file - [55935e7](../../commit/55935e7c057290c4f50f857758e07a771ca8c5b6)
### Fixed
- 180ms window cap didn't work on Replay-Loaded plots - [903375c](../../commit/903375c33e3bf714de01c730515551adebf381d9) [2349890](../../commit/234989037e2b81bd168f6009d4196612bc6228b3)
- Loading ReplayData failed and caused crashes - [88f2262](../../commit/88f2262b7bcd33069618bdf9d46091b67540d890)
- RoomWheel was broken - [6b33880](../../commit/6b33880c28cfdbc820cd646964e03ab6e4347cfc)
- Somewhere along the line Profile Name & Rating disappeared from the frame - [26db3d4](../../commit/26db3d41472b2328d76b0272aa7dc84d5013d6c0)
- Up didn't reset Judge conversions in Evaluation - [d791e9f](../../commit/d791e9fbc98738123a9ca9e21421f95804271dc2)


## [0.57.0] - 2017-11-11 - Discord Rich Presence

Windows installer release. This version was released as 2 separate installers, which are effectively the same.

### Added
- Discord Rich Presence is added to most screens - [b141923](../../commit/b141923975d882edeb900dd9e4071733621ac1f8)
- Experimental ingame Pack Downloading through curl - [dfde109](../../commit/dfde1092912ce8e11032f537b8e32c00d66b59b9) [259b574](../../commit/259b574f1199ccdff5f388c77da0926107a895ef)
- Lua has access to OS for os.time - [d2686d2](../../commit/d2686d28b486c7c4f3eaa1d98e2cc287651fea32)


## [0.56.2] - 2017-11-10 - Client Bugfixes & Previous Calculator Updates

Windows installer release. 

### Added
- Custom Judge Windows & Custom Evaluation Rescoring - [199cd64](../../commit/199cd649a403038d1f8f339c2ca579bb0b643391)
### Changed
- Boo window is now capped to a minimum 180 milliseconds for all Judges - [1864e48](../../commit/1864e4806d60b28ada654111743978b0b972d7a4) [8e08eb5](../../commit/8e08eb587030ff84d38a955743110bb6c343f23c)
- Score tab shows highest SSR for scores - [9c3611a](../../commit/9c3611a205a3b1be7629b59dff2ad8514f4cb421)
- Use WifeGrade instead of Grade in many displays - [0a4bfbd](../../commit/0a4bfbd3b22f0ceee7ccbd79537fa2bcbeabd1fe)
### Fixed
- Grades didn't show on the MusicWheel until it moved - [c503302](../../commit/c503302dc64554650dc8f837b19dbb68a170a77e)

## [0.56.2] - 2017-11-05 - Pre-release: Calculator Updates required for EtternaOnline

Windows, Mac, and Linux pre-release. The MSD Calculator is receiving heavy chiseling.

### Added
- TopScore flag introduced for HighScores for identifying Top Scores properly for EtternaOnline parsing - [d518aed](../../commit/d518aed9f83ae3a59710d1cb9605feb18a1a9171) [5df2fc0](../../commit/5df2fc054545ec13a16a78fd25c2349cfd543e78)
### Changed
- The Calculator has been updated - [dd1ed41](../../commit/dd1ed41ebda7617e2f354c2d45c8b1310853645b) [7e0bcc7](../../commit/7e0bcc772ce9290247ca47fd9d58bf48a408d2da) [0848477](../../commit/08484777b7277805959f9d58ce210ff638dfd3d3)
  - The SSR cap is now 97% instead of 96%.
  - The distribution of all Stamina files is a bit more even.
  - Stamina associated with Stream or JS/HS is better rated.
- Calculating the Player Rating also sets Top Scores - [ff9bee9](../../commit/ff9bee9518cfae946407fff97fe756e3f84d522f)
- Only TopScores are considered in Player Rating Calculation - [3cd733a](../../commit/3cd733aa59351d0da323a24e9312f2c99055c313)
- Player Rating Calculation ignores CC On scores, is a flat average of skillsets, and had the internal algorithm slightly changed - [c7cc786](../../commit/c7cc7865c0bdcad3ba82bff66840ca84f0c984d9) [bb6dfc5](../../commit/bb6dfc5cc1d8a48814c083862c649a4f6798f521)
- TopScores should not be read from the XML, and always calculated - [f2216ac](../../commit/f2216ac716d2911432c59df0253515df52a21759)


## [0.56.1] - 2017-10-30 - Pre-release: Calculator Updates

Windows only installer pre-release. The MSD Calculator is receiving heavy chiseling.

### Changed
- The Calculator has been updated - [363ee65](../../commit/363ee65244c7bc9ffd34f424c7795fba55c42648) [30281d2](../../commit/30281d21244a012ab4de1598a1ed7f34847793b3) [8ca7e6b](../../commit/8ca7e6b461bd97b53f9258880cf6249cfc5fc835)
  - Longjack files should be rated JackSpeed instead of Technical.
  - Files with hard Jack sections have more JackSpeed so it makes more sense. 
  - Stamina is less overrated.
  - Technical files are more properly rated, and include less longjack files.
  - Chordjacks & JackSpeed are basically the same thing.
  - Underrated files are even more underrated.

## [0.56.0] - 2017-10-17 - Pre-release: Calculator Updates

Windows, and Mac pre-release. The MSD Calculator is receiving heavy chiseling.

### Changed
- The Calculator has been updated - [98f806b](../../commit/98f806ba514fd24d4dc58a6868de3b7a5fb567e0)
  - Chordjack files should be rated more JackSpeed instead of Handstream
  - Stream files should be less populated with fast Jumpstream & more populated with actual Stream.
  - Actual Stream got a buff, while files that aren't Stream but got rated Stream should be nerfed.
  - Handstream files should actually show up as Handstream.
  - Stamina files should not be made up of sets of overrated files and underrated files. Perfectly distributed.
  - "Real" Technical files may be buffed.
  - Scores under a certain percentage, scaled by initial MSD, will not receive SSRs. 10s are around 50%. 30s are around 70%.


## [0.55.3] - 2017-10-12 - Hotfix 3: MP3 Sync Fix

Windows, Mac, and Linux release.

### Changed
- After syncing a file, it should be reloaded from disk - [8eec96f](../../commit/8eec96f5bc107c11aa57ff81c88989fb266bafc8)
### Fixed
- MP3 files were off sync due to incorrect math when calculating the next source frame - [52fbe37](../../commit/52fbe3752d493902e8d9ef0e35676a559a3c337c)


## [0.55.2] - 2017-10-03 - Hotfix 2: Weird Wifepercent Fix

Windows, Mac, and Linux release.

### Changed
- Use NormPercents from the SSRs to set PB keys instead of Wifescore - [c692da0](../../commit/c692da02d52f96aa531fd3be53e2b5a0ee964af2)
### Fixed
- Some CC Off PB Keys were picked or set wrongly - [4ff3fb3](../../commit/4ff3fb3fad1725c29512f3d6328e2fdf69dea743)
- Some files caused a crash due to an invalid Difficulty Enum - [82ca9a0](../../commit/82ca9a07461187be92d7320e52bbe84e69f999a0)
- Some scores showed up ingame as 11111.00% - [f7a4311](../../commit/f7a43118efc2537021a46246208a2de962637650) [e1ed93e](../../commit/e1ed93eac91dfee0638d985a74dcb81135b9b703) [4c7e2c8](../../commit/4c7e2c871e55a2f5bfc377212e6e69368fb93a28)


## [0.55.1] - 2017-09-18 - Hotfix: Things We Didn't Notice

Windows, Mac, and Linux release.

### Added
- Rates up to 3x are available in Player Options - [0ff5102](../../commit/0ff510201bd2f20a3fff2f17d91790ba1734f73d)
### Changed
- Don't disable Gameplay Sync controls when Autoplay is on - [8b9a92a](../../commit/8b9a92a2c8c8aaadfe3e5440954d24bb9935925a)
- Doubly verify a Score is obtained using Chord Cohesion off by counting the hit Taps - [2208f4e](../../commit/2208f4e5b1155e935c0cdce47853e856de1ac5d3)
- Fallback to basic old MusicWheel sorting for broken sorting methods - [1b78562](../../commit/1b7856217680e9257d24ed72ae2374f4caf95aae)
- Keep track of PBs, specifically PBs which are Chord Cohesion Off - [49c669f](../../commit/49c669f3853ff6daf487bbee82ec132b2783b56c) [c7addfc](../../commit/c7addfc4e62ba2b4897eccbf812db4e1cf39ec12)
- NoteData must exist when writing SSC files for sync changes - [286645e](../../commit/286645e7ebb3dbaf739a62e7f928eaa7d9d01bf3)
- Set the video troubleshooting URL to a real informational Stepmania video instead of Google.com - [acd1ceb](../../commit/acd1ceb443dbc4cbe202a0109e345179a11d89d1)
### Deprecated
- Deprecating GAME:CountNotesSeparately() in Lua in favor of GAMESTATE:CountNotesSeparately() - [7585ee8](../../commit/7585ee82ef48c8cd0b115f388bae442eea21a90a)
### Fixed
- Changing offset messed with MSD calculations; fix by normalizing the first row to time 0 - [8d0454f](../../commit/8d0454f6bb7254cff0acd27e48a367450874fe70)
- Crashed when loading a playlist with no charts - [4279a10](../../commit/4279a1077e82497af62cefb2307626831361f2bd)
- F9-F12 didn't work for syncing due to a logic error - [263b797](../../commit/263b797e9fd09548a35095b81227508fbab1aab7)
- Mines counted in the NotesPerSecond counter - [42207be](../../commit/42207bec8878168092a1caaf3e54d942a5eb9f4b)


## [0.55.0] - 2017-09-08 - EtternaOnline, Chord Cohesion Off, and More

Windows, Mac, and Linux release.

This version coincided with the launch of [EtternaOnline](https://etternaonline.com/).

### Added
- 5 new Etterna pack songs by Alexmandi, Elekton, IcyWorld & Cataclysm - [65a0174](../../commit/65a0174634799ba2b3e19c3e09c6eb4b5628e228) [5c85e12](../../commit/5c85e12a8efddace8fc85975e7e8f29ef9498042) [f29d0b9](../../commit/f29d0b97f689e2643bca3274b6f0e4033e4a3a39)
- Better Chart Goals - [ee3c6ec](../../commit/ee3c6ecb8de05202fc8e3aa7eae843e53406da1f) [aae1e3c](../../commit/aae1e3cea4efdc486aff7634c07e8d2eb6acd8df) [67b18f2](../../commit/67b18f278437f332fb3f58154b6d3300d1e769df) [cff3868](../../commit/cff386855d6e27374d4ecc936366ab4ac4e38edf) [c62e0cb](../../commit/c62e0cbf26a65b0f71099a61ea51377a4f8bf881) [206ee3e](../../commit/206ee3e8a0f1ef7359721636072237664c48aa26) [e918b68](../../commit/e918b68736c6f1ec11f291faa23339cbfd36fbf1) [360050b](../../commit/360050bc99442e0f4042fecbdd14f83fbad9700a) [ca7ec56](../../commit/ca7ec562a157788c920db7915d5696ebdf899d4b) [510d7be](../../commit/510d7bea827d8e3dac6962015edd49578257ab90) [4f10ee9](../../commit/4f10ee906c53da223f8477a320f51547c4423956) [ae60130](../../commit/ae601308684e1b0f466038d0d7c057b92294c63b) [dd04262](../../commit/dd0426272335ec63bdc43922e057c10964767e66) [9bcc516](../../commit/9bcc516610512381ec9b313fe966c198edf26026) [09c2773](../../commit/09c2773076637702286297103caa2a6aa9202a49) [0290698](../../commit/02906983c4f3d26565cff7673118dbd4d36028bf) [e0f53c5](../../commit/e0f53c5e4db89eb5f4372721ae40f2c19c1750f6) [176b6f7](../../commit/176b6f7ffb90e733699906af55b5987befd60652) [74c2051](../../commit/74c20513dd4c2fc4b4219f8f87b97a3f73a1221d) [44ec91e](../../commit/44ec91e942f1527aed3f64818599d4cc257f2336) [5c5d834](../../commit/5c5d834d67ed2712ef47e0ecc353f580c547d828) [0e48965](../../commit/0e489657699c27b7f0e785631250a6cf5a307146)
- Chart Goal Interface - [574ffa1](../../commit/574ffa1874e2b5272bb0d6a607c26f5dc5ea366b) [4036134](../../commit/4036134ba17315469f8ff673699392f3c963e55c) [2bd7476](../../commit/2bd74768ea5bbf3a3a16ea142ff8f837c63a9e8d) [5cbde03](../../commit/5cbde033b835d11af65ddb5bb7f93725a06cd98d) [708a829](../../commit/708a829309ecf0a90b0494a2259c3fe39ae159dd) [de4e192](../../commit/de4e192a3dbeb9066e385c462e116172138736c0) [f25a925](../../commit/f25a9258f63e420295a3c25a1551434e8cd45a5d) [c317251](../../commit/c31725122d08c5f435278665065e472cb22bf37b) [c86bd88](../../commit/c86bd888bc516daea9424402aebd242276cc1c9c) [05738db](../../commit/05738dbe445c1755755fb5b6158d07272f2e3e59) [cfc9719](../../commit/cfc9719e7bd47a8578f7df1870a5cf31a1e20b37)
- Chord Cohesion is disabled by default - [4bd00d1](../../commit/4bd00d123a26347a8d958d24e07c8ecc16c0b84c)
- Chord Cohesion enabled scores are invalid - [d50db3e](../../commit/d50db3e041a601e396a08c2beb539051928eb916)
- CustomizeGameplay Implementation completed - [8791e91](../../commit/8791e914546ba9e3c1c38abd138bbedd3c1eaef6) [81a0b5d](../../commit/81a0b5dc9d41d1b111eb4e0308c086660cc20c77)
- Differential Song Reload - [943935a](../../commit/943935ad51f55bc3102c0f3a2eaff1dd408c5baf)
- Experimental NoteData cache - [735348d](../../commit/735348d430a6229879088bacf8d694f5644905a8) [e76366b](../../commit/e76366bdba5dd273d1c706f1b67d8fe46a9c0dde) [8cb704c](../../commit/8cb704c82b94cac9e0976340ccbbd14611d61236)
- Experimental Rating Over Time implementation - [dae82c0](../../commit/dae82c0e8d79c5a81ea58a3992dffbfdae29b912)
- Lua methods to get Songs & Charts by Chartkey - [d0e0c2f](../../commit/d0e0c2fce2c1e5b7f8b7a0afc5b73e2411f4777d) [789181f](../../commit/789181f81aa3ff2b02235650728b228832b6ad71)
- Minanyms - [85a1e49](../../commit/85a1e49cc7f6aa7c65c8d6580a929eb4d3b76b2d) [f2a9fc5](../../commit/f2a9fc583775dfb7afa18d52fec4c33394a3a759) [0037787](../../commit/0037787bb10bd1b68e4a016f3a8ab84d995c1184) [19eeeb4](../../commit/19eeeb4bb158724a8fa2b89d7e14b8c126690b08) [1d9de24](../../commit/1d9de249d7a98e6e7ba5d737c4fe100f4cb835ed) [dd59276](../../commit/dd59276cc58861046fd7b594f28453bc7d00370b) [41e3550](../../commit/41e3550b50961bc83f7c1ac292244742693002f5) [2b61bc3](../../commit/2b61bc3d9673b2ea212eb245a4b792f0cfe25db1) [e8c4731](../../commit/e8c47313ed7e2b0b3bf2b191235616b5beb18e2d)
- Offset Plot viewer to view Replay Data - [920e236](../../commit/920e236e3b87648f3557aa6d943bc66f0335f283)
- Playlists - [6f0e2a1](../../commit/6f0e2a182980b1231a282dcfd800a1921c13a453) [2f94614](../../commit/2f946149335396e507d49e97e5349bca1bba626c) [aa727d7](../../commit/aa727d7f767882d2c5b16a8ede5a3c6400764884) [809698d](../../commit/809698d98d8e39c155d42b31538817cdeac40d3f) [1909f38](../../commit/1909f38d73bd5ba7d66411856a04658e548ae366) [6bffed9](../../commit/6bffed9284b50823fcd26b985865a92ce186c9ef) [aced494](../../commit/aced49498e02f7772821bb75fce95d43bab12482) [dc3c054](../../commit/dc3c054465a6272a5eaa755e93cf0c720abb428d) [610c38c](../../commit/610c38c0e919305dd19b27500e0ee15ee132cbc8) [f5021b3](../../commit/f5021b3ae25075d0332bd16f1ffe1eeb96a9be4c) [c6d8e34](../../commit/c6d8e34a98eccaf79053f5f41488e48a1882daed) [f051a5f](../../commit/f051a5f99acc6f4956610d2f9fc86ba1f191dca4) [8bf3b3d](../../commit/8bf3b3dbbe2cb641be4c32ada2a98f63ede24b47) [f9c3284](../../commit/f9c32845384d2e0b627a2cbeb781d5a52eacab48) [8f8f0af](../../commit/8f8f0af37a64db956a6581c56458706cdd43184a) [05898fc](../../commit/05898fc87d77f3d9df8ce17dc9f2fbd35836acb3) [5f4ab03](../../commit/5f4ab03941df32b66bb243134708cf93076341c6) [39580f8](../../commit/39580f80851b6520b6e0de8286619ec8bf3b3da5) [f79c1de](../../commit/f79c1dec45d81df5bcd48994a7e387e39ca7cb7f) [d0a4175](../../commit/d0a4175f2d7343e546d7cf7d4f0d4b91df19760d) [30ab098](../../commit/30ab098d4fff0b48443fa3bc82d5b1585fa4da17) [599e9f5](../../commit/599e9f549c37f0dbed76e04344601d0fb5c0ffee) [bb64dbd](../../commit/bb64dbd8fdbc90fabc6a39f9887f6c2920faeca1) [612d5ff](../../commit/612d5ff01bb9911949f991abd016add49ec44697) [16b4810](../../commit/16b48100e3433674861dff993898999a3f79322a) [8fb457b](../../commit/8fb457b794503806cbb2831d6fc5ac38ae6f485a) [b4138dc](../../commit/b4138dcad71f432616c7d098be7f76e567418319) [5cc615c](../../commit/5cc615c8b7cd66c8ad67398ba987df74a79e718e) [781a57d](../../commit/781a57dc4e32816b0866dd675c60efa66ea2f2f7) [e950df4](../../commit/e950df43b7165384efb516230b039e783dd84071) [3fd8f48](../../commit/3fd8f489d732cc05778155368603d085f2f6509d) [4770443](../../commit/47704431e1357f54e1982ad75a06a26b03d3206f) [bdfd39d](../../commit/bdfd39db5b840913040f296b7936cefc4e6fc7a3) [c96bcd5](../../commit/c96bcd5337113559d55f0195e8c9b22827134955) [32e07dd](../../commit/32e07dd07c338d5f3fda5ef9877222080d3131df) [9005ee5](../../commit/9005ee583c50a22098a048e5b5d76a43151333d9) [3718be2](../../commit/3718be29fc23bc3e88d917bd357dd7901b375908) [a1ac458](../../commit/a1ac4581f1d3822922397eaf5de3d6e90584c791) [c5463c9](../../commit/c5463c98994a2d00695b350a425ff92b3451e731) [b66b4fa](../../commit/b66b4fad4241b3b3fc0b8a0abd49de6560a3ac0a) [9f3c791](../../commit/9f3c7918ef2756b95ff9b3592fd8c7323c94d22d)
- Playlists show up as Groups on the MusicWheel - [eab1dfa](../../commit/eab1dfabc16cccafb6e27abddc2ba341141440bf)
- New Etterna.xml save structure, like Stats.xml but without ReplayData - [82a0b9c](../../commit/82a0b9c13571339bd4e586d23f58ff786e9f3329) [b5ae71c](../../commit/b5ae71c960eb266f85935a21d0b9cdce9ce7ea93) [4f3ed7d](../../commit/4f3ed7da600e463cbbb67f5f02aff70db770d4af) [2146655](../../commit/21466551533e53188769a3103781401a1f987875) [868f31a](../../commit/868f31ac8dc99a12a6f58b8e62c3380338a4fef0) [e7602e0](../../commit/e7602e023976d839675ff9c9664e01c5a8b2fbb4) [4502a04](../../commit/4502a043a3983e0ba7a82cd2ae4fe0fff8753d54) [4073f0a](../../commit/4073f0add736e0ed7121d2c653d50193af309b3e) [f455c7e](../../commit/f455c7e8fc60997bbee0fe0a18e73f0af79fc72d) [f893394](../../commit/f893394709aa7eab319845dac8b9bc4823a589af) [e94420c](../../commit/e94420c794b0f8964a0d0812b847df22cc21d94b) [6a47206](../../commit/6a47206b3d9f1db58ca91a2de492cae381ad9b17) [b318a42](../../commit/b318a4282ddd6deb32c938775206a123b719a281) [ac71458](../../commit/ac71458d53da1f0c697aea3b151454db415743fb) [074812d](../../commit/074812d151ab30508c1dbd9f47fb02f380b871d1) [277e3be](../../commit/277e3be48960be8a25d3c989a0e9078022da3bef) [f06bffb](../../commit/f06bffb6cff2b082710a72a0b46d904199d7819d) [3856357](../../commit/385635795f2acb314e46e1fa441cb6a08fa42294) [3d0a90e](../../commit/3d0a90e2b7a23d5021713add8318e46ee16c3181)
- Per-chart mod Permamirror - [fc8692e](../../commit/fc8692e6b7745e9a48e2b61cd0dcd622611c9bdc) [c0c1333](../../commit/c0c13339f6e9bf9658eca941d53f347a44aa41b4) [109fcf5](../../commit/109fcf5900bc4d315d01cf3ffdcd183d42a84d2b) [87706ab](../../commit/87706ab808fa90038b0354667ccf1b16cba3abdb)
- Player Rating appears on ScreenProfileLoad - [d0847ce](../../commit/d0847ced4360a6dbd46eb57d3ae8fef545b5361d)
- Preference to hide the lag window with the FPS counter - [befe017](../../commit/befe017fc879f8c816d34c0b4703bc089719989a)
- Recording Wifescore over time - [804aa0c](../../commit/804aa0cd6d5decc58e33ab9ce43413ef515a6d23) [859116f](../../commit/859116fcccc86a79528840b3767f0c5575303c7a) [bb065fc](../../commit/bb065fc343a66ecbbc150b7f0c07d5e6bad76b26)
- Song Search tries to return Packs if it has no results - [363c36b](../../commit/363c36be89dc9997b01fbedf9c2e58728f8351ee) [f4b6e4e](../../commit/f4b6e4e4c7568202b9b198a8f46254621889c6cc)
- Stats.xml to Etterna.xml converter (and SM3.95 support) - [7647cce](../../commit/7647cce1fdd208589a5b35e553ac5f3b9b033f9d) [86a50c6](../../commit/86a50c681da31278bac12c17cfd142648a9be58c) [d013f3f](../../commit/d013f3ffc966fc723bcebf0ab44221e9f8203dfd) [e8ec832](../../commit/e8ec832f551ab08483fd42931bb74ee61c82960a) [4b77582](../../commit/4b775828fec5f41499d084b8f3a508aa806b34a4) [65ca243](../../commit/65ca24391d8b9b6f1bff77a3f8ca93fb4230d020) [ee613c8](../../commit/ee613c85ceb25bb163345e3d2821e890c4fe3af9) [41fc83f](../../commit/41fc83f9a093177dc7edd9800a705933d1f83277) [3021745](../../commit/3021745dc0916472bfb38cbac07b60c011f9751e) [8a8e7d4](../../commit/8a8e7d47341dd0ab44f43413552193b4dc31abfa) [1e3fa56](../../commit/1e3fa56a4d82a83aebbb213927d4c75d31ba70ae) [ad58aa3](../../commit/ad58aa3f8f8232e27daa4138f72ba7c1d0f4ac2b) [769eee3](../../commit/769eee3763ed93226d7e5b652c8a90ea8872115c) [25540d0](../../commit/25540d05d097d48aeed824f7ba445082231f77b1) [5ecb98a](../../commit/5ecb98a516dec689beee97fbe23047a14d40db95) [8f4fbc6](../../commit/8f4fbc66f116a1c6dc7ed90eb4e055fb8f5fd8a8) [6cbd15e](../../commit/6cbd15ec0f6eadb9018e143272a26e0167f285da) [22b7fc9](../../commit/22b7fc991c16ffac47761d66f0cc9d629f1171a5) [cbddb1e](../../commit/cbddb1e0272463bd0629662e233c29ab1d55bfc8) [879cf79](../../commit/879cf79f212bc53cba4c3aad2b24fc0775990cc4)
- Save profile hotkey - [c82080d](../../commit/c82080db97b05ca8393fde2f704c4e02953db029)
- Reconciling broken Chartkeys is possible - [4020933](../../commit/40209337eaaefc47604eb2dbe689cda4a3d97d50) [18657d3](../../commit/18657d3b4bbb015dd857f5b3b14648ed184041e5)
- Replace MAD with FFmpeg - [3958470](../../commit/3958470636aad59d4746a15003aefdad554f8360) [21c3395](../../commit/21c33956f89781789eac6dff1c5058a735222328)
- Travis runs Coverity - [0a607f7](../../commit/0a607f73635e012bfcf9addeca475af1f6bdaaff)
- Validation keys 
  - MSD Tampering - [fb39253](../../commit/fb39253e5bf9cacfe5fb496a203d717a171e06e3) [0910fea](../../commit/0910fea8b26d05b8e1dafd392d0c796434cc156f)
  - Other - [22f2f72](../../commit/22f2f7225c247390338253e2b42cbe6148c87fa7)
- Window size getters are placed in ArchHooks - [455b70d](../../commit/455b70d44210568ce9204dd4b2383ea832ab2764)
### Changed
- All pointers to HighScores and SSRs are sorted by Skillset Rating - [d0b12ae](../../commit/d0b12ae36e732d333d8d8c2c7bc71fd9a9798e3d)
- Autoplay is not human - [15bcdba](../../commit/15bcdba1a7cfcca4fc0b31830655d64c636626f8)
- Autoplay is now even more intentionally bad - [98fe95a](../../commit/98fe95a540ce996e243922177efa81f969d79cb2)
- Banners have capped resolution sizes - [2525925](../../commit/2525925554f5d32329fdffebd15cc8f3be8fd3cb) [dfe35bc](../../commit/dfe35bc297d5f42c9c837763f7a3a943f82a3af8) [c768ecc](../../commit/c768ecc6fa7c74cf5ef4f76a98f24103b4afd60c)
- Building the Estimate Time vectors in the NonEmptyRowVectors, reset the previous ones if rebuilding - [c1c960e](../../commit/c1c960e5cb2f95c85dc190b69f1305a4071b097c)
- Chartkey Generation & MSD Calculation only happens outside of Gameplay - [a2b4b34](../../commit/a2b4b34948e5a7376a96fedaee3de02c904a5f42)
- Chartkeys are private fields of Steps - [39a3134](../../commit/39a31347ce66c57c4615c4741874d0f41f0336e2)
- Checking for a changed input device happens once per second, not once per frame - [01b930f](../../commit/01b930f84c1b3d3d6468b85e8644d46393a37125)
- Clicking a category on the MusicWheel opens it - [412f5b2](../../commit/412f5b2c4382de99e6d8be5627daadbea0f23252)
- Don't allow the Sync overlay help page to repeatedly spam Commands for showing - [c46dd91](../../commit/c46dd9156d9fcddb7f5fee66275c4f3a8ad3c6df)
- Don't cast a Cstring to an RString - [3d89914](../../commit/3d899146930b027efeda03451352196e36e466ba)
- Don't check to see if a set of Steps is invalid, because Chord Cohesion is the only reason that existed - [66f6fcc](../../commit/66f6fccdf8801ba7a4f0f56e80240504f1b2c10b)
- Don't consider invalid scores in player rating calculation - [69314fd](../../commit/69314fd40bb1dcf7fb51bb9919168d890a2b03b9)
- Don't consider scores from old MSD Calc versions in player rating calculation - [6f3555f](../../commit/6f3555f6c7ba5295f232f3501e859f8bab20b79f)
- Don't generate Chartkeys for files with no NoteData - [f0d645b](../../commit/f0d645bdd56c77bc26cd79dfb793916f2496478a) [dc3cbf9](../../commit/dc3cbf9995d2876d99583ed1dfd8e32f4bce20bf)
- Don't grab the loading window from NetworkSyncManager - [084da49](../../commit/084da499a16237af7f91f9a95546845d56c1057e)
- Don't reset an SSR calculation if the Chart is not loaded - [e40d3c1](../../commit/e40d3c1c542debf1c4c8a2121acc970777082140)
- Don't try to render BitmapText shadows because they don't work - [ce38224](../../commit/ce38224b6030687850d2ef32ed8a4837bc1a6386)
- Edit option on the Main Menu is now a link to ArrowVortex - [5034aa4](../../commit/5034aa4f1dbff52a4564b79a2988abd4caa39011)
- Empty Steps are ignored for Radar Calculation & Caching - [920d502](../../commit/920d5029a70f5dd9f268b2bcd76a992346730585) [41fe93c](../../commit/41fe93c34b482cce353728e88f92c5b685c5d035)
- ETT NoteData compression method is changed - [426fb61](../../commit/426fb61f263e0406a09b5a57e5c0d19cfbfb1a4c) [3724b16](../../commit/3724b1653f5d93e708f4775de1e0c757f8e001a1)
- Favoriting and Unfavoriting is a single hotkey instead of 2 - [249c16a](../../commit/249c16a5327b0fd40b3ba7c9fcc00e5c4940fc11)
- Favorites are not a vector, they are a set - [27b97b2](../../commit/27b97b259b5b3393baa42c1e4d5afc8b23ab4ef3) [6140fd2](../../commit/6140fd2210cad1067191c9c73462adb20069f72a)
- FFmpeg updated to 3.3.3 - [ec92941](../../commit/ec92941d5cde2f8c8c474763cd3c57d54502b27d)
- FFmpeg GPU acceleration disabled on Linux - [20e0301](../../commit/20e03016af76dbc8f8ca4c1ec5d631da0f6e4713)
- Files with under 200 notes are invalid - [fd16570](../../commit/fd165708e11735863e3960cc4df4559ddeb3acf5)
- Filter tab supports 3x rate - [bae2ca8](../../commit/bae2ca8b4c2ef93d966cb2958a970cebf28772d3)
- Getting WifePersonalBests uses Chartkeys and Rates - [869bc14](../../commit/869bc14a1c4baf8add32de223cc6f6005150e8ac) [515a8f7](../../commit/515a8f7c9a856b269596fb8091ac0362af39fd04)
- Grabbing Songs & Steps is done by Chartkey instead of ID - [3477591](../../commit/3477591b26ab29e8107bf580ac8cdd58e1c8c383)
- HighScoreLists in Profiles are now maps of Rates to HighScore vectors and Chartkeys to HighScoreRateMaps - [884b450](../../commit/884b450b776733c6a927f227272e65c07a7d44cb) [cf8283b](../../commit/cf8283b8183716efd416f36c5ebaf7bf8abdc66e)
- Historic Chartkeys are just Chartkeys now - [4eaa892](../../commit/4eaa8929d7655ec79b5ccf4910c1ae81a9136c6e)
- Hold Shift to save a compressed Screenshot rather than the other way around - [1ecde1c](../../commit/1ecde1cf13854b5e71bba4bc29e237d773c2e523)
- Improve Profile tab code & sorting - [eae3632](../../commit/eae3632f0e6c2cd39856eb58a0b15e1d237ff7e3)
- Installer refers to Etterna as Etterna and not Stepmania - [3ec7644](../../commit/3ec7644883a096837f950c06300e773835e279db)
- Instant restart button will not work online - [03a2a83](../../commit/03a2a838d5f55975bee6babfec24ef86ea52b4ca)
- Judgment Messages send unscaled Wifepoints by default - [087c022](../../commit/087c022ab9121a71bfa51c5ec9a4cfe13ea4e899)
- LessThan operator for HighScores now considers only WifeScore instead of Grade - [3f0ed1a](../../commit/3f0ed1a197be38888f7bb566c9b0c54a1c627dd7)
- Load a fake profile if there are no profiles, in lieu of there being no Machine profile - [23b513c](../../commit/23b513cf32c736fa4a3ece38ee2b025965ed64bb)
- Lua must force Gameplay input redirection repeatedly in Playlists - [b712fba](../../commit/b712fba00fa2cddebfda44b0bd3eed4d29d169df)
- Many instances of RString are replaced by std string or Cstring - [de63892](../../commit/de6389291e670335a81a0a0ca36e4fb85a1424aa) [e45d4eb](../../commit/e45d4eb8a45ecc9d1f8d798bf8b38edfb45dd511) [a7326c8](../../commit/a7326c872022a0668830350b973fdd419e7c9c8d) [6a47206](../../commit/6a47206b3d9f1db58ca91a2de492cae381ad9b17) [d76c96b](../../commit/d76c96b7377ff97a43b8b0e412490608130433e6) [a12b25c](../../commit/a12b25c7dc30405ff081bc971abb3515c817ee27)
- More MSD Calc updates - [e7507f0](../../commit/e7507f0309d11fd79e24ee5cef1a2bf120da589a) [d13c90f](../../commit/d13c90f1c676c8cd7bc5bf69225bcc2563fc76d5) [1f6601c](../../commit/1f6601c151465041f3479e8d6e40b4bed43d3538) [f8f5282](../../commit/f8f52829422e419fbcd57237657ae54d180e722a) [15b7a57](../../commit/15b7a57a2f9bbf5da1b63c05ed4d4ce48282d122) [b54911a](../../commit/b54911af0632f56baae91632147b3ee3504f9b9c) [6ecc70b](../../commit/6ecc70b96ea2b191ad9521e26df36ee0ef7e067b) [e04a16d](../../commit/e04a16debf965f9b9bea0be9147cff4f7b6b9e2e) [dfb13f9](../../commit/dfb13f916c0983c2d998ffdda770307141f63223) [367f0d0](../../commit/367f0d00626d8892d6c947224e33f4b53cfb4bbd) [31ce05d](../../commit/31ce05d666b73345dd9bf55de41d1efd2fb5750e) [e91f433](../../commit/e91f43380b3852ed54b9e4bad2ae96f290d21ac7)
- Mouse Wheel Scrolling is faster - [32de0e2](../../commit/32de0e293d28994f4e35221407252ceabcb40d03)
- MSD Calc should build easier - [cff8283](../../commit/cff8283a42881bc022b68c1238b2a15e9f9042f1) [aaefb58](../../commit/aaefb58b8223e6f56c8e880614c5eecc8f2e8911) [853a9c1](../../commit/853a9c106a2d59d36ab0a44154c6313ddf5e8619)
- MSD Calc version is an int, not a float - [809ad61](../../commit/809ad61408af6bd186e5ab47c22ef359f9f50be7)
- MSD Values for Charts are cached - [4df4581](../../commit/4df4581fc458c5a6b9ebc662daf27febea8c4c41) [d38a10f](../../commit/d38a10f0df33efa11bdb857f7f550e45fd46011f)
- MusicWheelItems update permamirror, favorite, and goal info regardless of score info - [d35e3a4](../../commit/d35e3a4e522f6e77ead59f83c1ea42e97073ab2a)
- NetworkSyncManager doesn't need to look for SMO packets every frame - [e97ab8b](../../commit/e97ab8bed47d185601da2e7f9122627e900969bd)
- On game initialization, basically display a black screen instead of white - [d65b0ff](../../commit/d65b0ff4efe2ef664172ff7669a3a396f194c1c1)
- Optimize displaying Grades using score keys - [e7348fe](../../commit/e7348fe8619cdc0d25698acdac7ea936e574f80a)
- Optimize Estimated Time calculation for Non Empty Row Vectors - [daf3600](../../commit/daf36006fb38f55460e18513bcd938aaebf66103) [4080d27](../../commit/4080d273cce8e4f82e0d39d2217aef8bd8d3521b) [bbea048](../../commit/bbea048084ce9d855ef3a8b83c764137846282d6) [e746043](../../commit/e74604379622aad8f265ccddefcd0984a20a80cc)
- Optimize Estimated Time calculation for single bpm files - [10e9ec8](../../commit/10e9ec8e079385dc1aa3732dd6691318e6ba8824) [4080d27](../../commit/4080d273cce8e4f82e0d39d2217aef8bd8d3521b) [bbea048](../../commit/bbea048084ce9d855ef3a8b83c764137846282d6) [e746043](../../commit/e74604379622aad8f265ccddefcd0984a20a80cc)
- Optimize Loading from SMNoteDataString by not initializing empty Taps - [bbcf62e](../../commit/bbcf62ecf5b37375fec03dcc3bcf5c75b465b539)
- Optimize Offset Plot drawing to be a single MultiVertex - [d5cce8b](../../commit/d5cce8b0b345659c33db6d072f46a64a3dff7946)
- Optimize Offset Plot loading to let it be loaded anywhere - [3ad5aae](../../commit/3ad5aae56c63b266178b42e3c5c584a4130faaea) [26d0f3a](../../commit/26d0f3a354c365120f4a57c360bb82e54ba4149c) [f77f5ba](../../commit/f77f5ba96fccdc40432465469db824985768d369) [920e236](../../commit/920e236e3b87648f3557aa6d943bc66f0335f283)
- Optimize rgba to rgba RageSurface conversions using threads - [35ccdfd](../../commit/35ccdfda0a015b23b6dacf5894f8521bd5740ec8) [9b33207](../../commit/9b3320761e8a65f23b159b2befd4217cc5ed5548) [1a819db](../../commit/1a819dbbf9cef9a87ef79a3c2eac3a1a49f00284)
- Optimize size for many JPG & PNG files - [db1d035](../../commit/db1d035084ba4ffcea211b32c4b1c1f311421a38) [80ba803](../../commit/80ba8038fc30fac14d2ad27f3a1fc53a57427f01)
- Optimize Scores, basically rewriting them entirely - [989fc4f](../../commit/989fc4fa273a7dc85d9ed9bf452d9c4aba37e114) [08568eb](../../commit/08568eb6059d148ee8ef7414ed179352305e6ce0) [4402e2e](../../commit/4402e2eeec3539fa702b43a33e90a16dce68ba55) [e02d546](../../commit/e02d546a483707a9c0994fb85a0d75467f0db12f) [aa5a8cb](../../commit/aa5a8cb11b2080b9a3db6647ca5bb92e6a1e6ce4) [87d4ccd](../../commit/87d4ccd75e0c4af2fc4e162fb0d217d3af761cb7)
- Optimize some rendering code for a 20-60% performance boost - [c4f7ccc](../../commit/c4f7cccaaf09e058ae142f46da6a02a95ac819b4) [c217987](../../commit/c217987ca520123aca586e114293d3b6f5ae5b16) [4a566af](../../commit/4a566af49c86c4a240814604a6ca6d86c360f610) [3951fd3](../../commit/3951fd369a5e5add5e8c9a984b4d588ade07ef31) [51bcae3](../../commit/51bcae372ecedfc698b651a8dd2667b781683546) [1c612f8](../../commit/1c612f8982aee6749514ea891f0970732b126c17)
- Optimize Tap rendering by rendering Taps when they should be rendered - [65d0312](../../commit/65d0312c455be19d4b22c2b8986ce2fafef95755)
- Optimize vector use by using emplace instead of push - [a29333c](../../commit/a29333cf4323c293fb0e2ab8b32e537e49b02dd3)
- Preference related to the minimum percentage to save a score is renamed - [62de3c5](../../commit/62de3c55b9869962292e7a747d5ef3a7c6109886)
- Profile saving and loading operations have checkpoints - [edb50a6](../../commit/edb50a65b7605317d2cd5dfc52cc4c05afe4b081)
- Profile Tab Lua Optimizations - [9fc2a99](../../commit/9fc2a99394185ca08f53af77e0ded18de00e9a50) [be7b56d](../../commit/be7b56dc53355eeae83674f483a40c92bdfae395)
- Properly reset NERV/Serialized NoteData when making NoteData - [bc4b113](../../commit/bc4b113548966bb587c94d989cfa7b5006718c1a)
- Radar Values (note counts) are ints instead of floats - [d34bfe0](../../commit/d34bfe0efd03028cd4f74707fc0d31b03c3550cf) [47ca4f3](../../commit/47ca4f3aff711d1a886e7e2959f55d578dee623c) [b87a395](../../commit/b87a3953ecfb8d62c8e82816e2c98456b26f51da)
- Rates up to 3x are now supported - [48a799a](../../commit/48a799aaa95a4bd2c561799e83e2285c0fdb9dc6) [2b155de](../../commit/2b155de8cc337305891fc8279be56f8e9699c28b)
- Recalculate rating and SSRs for player loads - [eca7cbe](../../commit/eca7cbebcfc50e2eedb4794105bc2a23042429b7) [91882c6](../../commit/91882c69e257692b2272f55a71ffb52de3b3b32d) [79a0bf6](../../commit/79a0bf671e956c7964c94d8d7ebf3157a71ee509) [d0a9651](../../commit/d0a9651bc2de04b484b19d51875eef89f036580f) [35a6438](../../commit/35a643846a73d83a07a46af1ebc2c7978761f7e3) [9c151b3](../../commit/9c151b3258bc88e051280d9bfe4450e0849c8908) [6aa1055](../../commit/6aa10553b2d4613dda62bfcd3542d38ec2753399) [4fabc16](../../commit/4fabc16c6d5e7dd67ca1d4171cd8776009008a1e) [5ba773b](../../commit/5ba773b67f2e2caeb58bc9e5c6fb3afa74f857dd) [a0a6d9c](../../commit/a0a6d9c39992e1316269022b7f4229c53f421de3) [cbacc32](../../commit/cbacc32debe87e4806c5e57c5b6178892e65f4db) [b42c10e](../../commit/b42c10e07c25d756b4e05ffb73fb8b9ab9d7789b) [8ed6018](../../commit/8ed60181306a6d17acbc52bb202bb4ff84988165)
- Redirect some links from Stepmania to Etterna - [ebad781](../../commit/ebad7811e452b3dbd979981c33b0e614f368a912)
- Redo the LoadingWindow internally for smoothness and not so much reliance from multiple sections of code - [09fb85f](../../commit/09fb85f3c74e44a9486da49a28929fec416363f4) [084da49](../../commit/084da499a16237af7f91f9a95546845d56c1057e) [1b468fe](../../commit/1b468febfb4d144be432588619a0e90d52010d5a)
- Replace more typecasts with static casts - [8aee60d](../../commit/8aee60d906833a08f69f15daa3a8b994db8c4dac) [8fd4ea3](../../commit/8fd4ea34d5f072eb72f5ddedcf7866ccb5d557e6) [f87d1c4](../../commit/f87d1c49888a07e8a57a5947c7f28f0d9ca4b6be) [50161ce](../../commit/50161ce943aedc1c2f40c3f27e4e7908f224245b) [5ec7e84](../../commit/5ec7e84984607080df477ccdf2e54c302eee57c3)
- ReplayData saves separately from Profiles. Early InputData implementation - [909517c](../../commit/909517c0124e57e89721e743ef5496579a306223) [6219b9b](../../commit/6219b9b094d0499a16fc4cb926f919b335af4431) [1c1c3e3](../../commit/1c1c3e37d76883c6ccb356c98abc1f1f0215a85f) [feb50ec](../../commit/feb50ecc530a9d7663ceb58993a2b7437e11bbf3) [b6036d4](../../commit/b6036d45131847e907eb3cad80361336b025cb53)
- ReplayData should save and load properly - [21b82d3](../../commit/21b82d3a7bded725f743cd73c3571803eac1ab98) [63a4637](../../commit/63a46378a53be0fd997e8380e9f1d925e97141cb) [814214d](../../commit/814214df15d3249a1eecbcb364cbcbaa54a09be5) [ac84e24](../../commit/ac84e24635944a84704ab195b5e6dd31dbcfb63f) [d7c17dc](../../commit/d7c17dceccf2b6a12a3bbc6039a7a6b9ac02c4fd) [920e236](../../commit/920e236e3b87648f3557aa6d943bc66f0335f283)
- Sort all scores correctly on first load from XML - [e627277](../../commit/e6272778b069bde3d0cbcd83f60fa1957a04ebfc) [071c8f0](../../commit/071c8f0dc644b182f7c8d6fddc89e6aa90c98054)
- SSR Scaling maxes out at 97 - [6412d2e](../../commit/6412d2ee9b486753b36dcf82514245a700c8c6b2)
- Unordered map macros implemented for use in getting Scores by key - [279150b](../../commit/279150bc3b8bd224e98950ed1f75ebc14c73704e)
### Removed
- All references to Cabinet Lights - [23ee2c2](../../commit/23ee2c295254094361ecb600dd25a37b51af738a) [4445d68](../../commit/4445d68c0d3be934a4f623409e6f010da4b937d9) [1e66e3e](../../commit/1e66e3e3fc82611138e7a90acdcf95895b2e930d) [43314e8](../../commit/43314e8a2ff51ae243de5f25a336daf543080026)
- Attacks, Battle Mode, Rave Mode, related elements - [89bf1c8](../../commit/89bf1c8505d2b56889085bac2b6c2af70f8ae0b4) [c141e20](../../commit/c141e20d415b94c498dcef694194c157bf4c473f) [3e18ec0](../../commit/3e18ec0afa80afe693bc524230c423624f5acc04) [c0d754e](../../commit/c0d754e20526d81d9f367a93cebffd50148576b8) [d0f9900](../../commit/d0f99001fce6e97de24b24e687df08d7cb93f05b)
- Autogen not in Noteloaders - [6ad869d](../../commit/6ad869d32aacca09c81da43b4293ef637363cf43) [3e18ec0](../../commit/3e18ec0afa80afe693bc524230c423624f5acc04)
- Autogen from Noteloaders - [de6e19e](../../commit/de6e19e4bfea98d7a6403a7f4b88ddcba8a9158f)
- Autoconf files & Makefile.am - [b978a41](../../commit/b978a410ca8f326ee7c6dae42728044f8c23100c)
- Bookkeeper (CoinManager) - [5682c18](../../commit/5682c18fddbb96fb164815842fb0523c3d505c91)
- Chartkey records in Cache for debug - [2db2e79](../../commit/2db2e791c8eab195bf003ae278786b5fb6a883eb)
- Courses - [2afc922](../../commit/2afc9227450eba1dd62c516f9bc345c21e51be3a) [a09e150](../../commit/a09e15033e2d7218e5bad36e365277e286a8010f) [4d06e74](../../commit/4d06e747190577f902b5a0ea27f17dd2af0c1830) [80424d3](../../commit/80424d3d2dc3d67861027b401221a0df5e6bc958) [5095b95](../../commit/5095b95a08474f21f6576636c6782e4043385d20) [d51c33f](../../commit/d51c33f78dd8251dc82d61161361da21ef85dc98)
- Displaying Grades by Difficulty on the MusicWheel - [04dce35](../../commit/04dce3588cea4d4276b4bc314b450b5cd8858c50)
- Haste mod - [5a44b46](../../commit/5a44b46e11c4cb4f0f907a6c9ac1fd83079c9a74)
- Hidden songs - [477a424](../../commit/477a4248441e9f75e5e0951e532270cb312a7ad4) [6d2ae18](../../commit/6d2ae189a69faa219b51bfad8c3391d2003500a8) [ef24d4a](../../commit/ef24d4ad20be45ecc12aa5d5a54817a67d373ffc)
- Ingame Editor - [313e011](../../commit/313e011552db8e2a07353a5631ce4e098ae1203b) [3e888c5](../../commit/3e888c59b7f1b75e6a5c002b32a5dcbfdff74711) [3f32e4d](../../commit/3f32e4d6d908108351ae44e7fa9cb56f566d4c6d) [2cf03b1](../../commit/2cf03b14b619fe16f8477521d8dd16a15ddf6bc0) [80424d3](../../commit/80424d3d2dc3d67861027b401221a0df5e6bc958) [314cd3d](../../commit/314cd3d242a0b0d4f1985616ecc9c9e0c6b0b5bc)
- Jukebox & Demo - [08cccb0](../../commit/08cccb07aebba601ff2650af26a5919e36cd57ae)
- Kickbox Mode - [6219d33](../../commit/6219d336f0deae6c027a3f769b4969fc7c170a1b) [1a72adc](../../commit/1a72adc1d85545925e333c9b8274d662ab8064db)
- Machine Profile & UnlockManager (song unlocks) - [f87046d](../../commit/f87046df1e0ec7ec0d9b46bf6d3398b22b00918c) [e7e550f](../../commit/e7e550f0496ca77acf7bd78e9e7f03643f8a6307)
- MAD audio library - [21c3395](../../commit/21c33956f89781789eac6dff1c5058a735222328)
- MemoryCardManager - [446f809](../../commit/446f809f1488bdaed1107325615be342870f6181)
- MonkeyInput (random keyboard input) - [fe50843](../../commit/fe5084362a01bcd512991dc967248d8e6a0d24cb) [0258e9d](../../commit/0258e9d1f280192805829d6cfd11c07485fd7113)
- Old Radar Values (Voltage, Air, etc) - [b652bd7](../../commit/b652bd7edaab95bfc088c8ae843caefbc490bf77)
- References to Calories & Workouts - [ec208c31](../../commit/c208c31eb2c57e412ac12ba2960969cf7c5010c3)
- Roulette & Wheel Sort by Difficulty - [a61ffae](../../commit/a61ffaeec87d98b36f110775903d570e6713d3e3) [977cfb7](../../commit/977cfb70522c0710c03e9238f9da68797d4454f8)
- Saving profiles directly into the old format - [44d2f21](../../commit/44d2f218741a5aec1e6dea251a8013c5b81eb820)
- Scaled Wifescore because unscaled is the default - [61314b2](../../commit/61314b25db7a56121c233b321cb84249898fa235) [af7eee3](../../commit/af7eee3b87d6f0e2d2f2df60d1e8a0bd09bf2615)
- Size cap on the list of HighScores saved - [c10733b](../../commit/c10733b87204392a7ec2bfc77ac5f10f1179769c) [0c1c8bb](../../commit/0c1c8bb3974519ae8c1290f21b66acfaac7eb809)
- Some visual references to DP - [8936fcf](../../commit/8936fcfbcf74868ee1ff9d0a9970d0603ed8c1d3) [1fc8320](../../commit/1fc8320f4a4677f903be95e1bf91b7cc28d998d4) [f810663](../../commit/f810663483851108ebb4c12c23f2822ba7060673) [e6795b5](../../commit/e6795b584a79247f2367f50d8d273f3b87a3daca)
- Symlinks (Songs in multiple folders) - [062cba3](../../commit/062cba3ed48a90e1196f2f06eddc098c3cd4db6f)
- Theme Button template function - [1866a48](../../commit/1866a4832b557583ec6f4ebc3583968aaf4a9348) [9551832](../../commit/9551832f5d850e93a35f41f0d4dd34990d11f23b)
- TopSSRCalc functions & Score/PB by key functions replaced by faster implementations - [c65f4f8](../../commit/c65f4f8e7fc7344365349ee9efad8efd2c100262)
- Unused fallback theme CodeDetector instances - [db33591](../../commit/db33591da3d344996240930dbbb483216baf7f88)
- Unused NotePerSecond vector generators - [2b5e32e](../../commit/2b5e32eda80572e08b0e5901bc88655c9fcb4dce)
- X11 ScreenSaver stuff - [2da5dbc](../../commit/2da5dbc65932ce93818f2118feba4819b3dca386)

### Fixed
- Autosync broke in Gameplay due to pulling the precalculated values from TimingData - [c516aaf](../../commit/c516aafdf7b62804a2876f36a18bb89d27102073)
- Deleting FileSets leaked memory - [2c0385c](../../commit/2c0385c1c17faf030a2097bb31c84970c0861152)
- Float to String function usages were bad for the XML - [2bd3a28](../../commit/2bd3a28a4a4ba2b684f164e11b9e35415d4b03b1)
- Game crashed when managing old scores without a listed rate - [27659bb](../../commit/27659bb08b5c1d5ab12f62201af2bf69956fb51b)
- GetScoreByKey was broken - [c9e4e1f](../../commit/c9e4e1f7209e98c61cdb024d5fd428ff7986c68e)
- Init splash screen could not be skipped - [d79a5e3](../../commit/d79a5e31ddf20bb8abdc7b397489e5e5bf223232)
- Mac build didn't work - [a63348f](../../commit/a63348f3aa256813a1a3425c58371a09707b6d5b)
- Memory leaks were caused by MusicWheel scrolling generating new objects - [151e083](../../commit/151e0839ca6a2d77736d32117a6a88a54449e458)
- Mouse positioning coordinates were bad for Windows - [93f05ea](../../commit/93f05ead38228ad7d204fb0f779b156aa600867a)
- Online banners & MSD labels were broken - [a25f5b0](../../commit/a25f5b0a25093a8668c03fec6df6e4c8c16efd4a) [7d0a25d](../../commit/7d0a25d01e2734ef2e11c28d9e754897f91158d0)
- ReloadSongs button was broken - [7858734](../../commit/7858734fbf02528b8a3fa788170f92f24d95281d)
- SMZips didn't work - [e59c916](../../commit/e59c9166cad46c0e50745acfcbb0c28e96ac8f95) [9bc5440](../../commit/9bc544069b020a2f6398124858a832dff19fca01)
- Song Search didn't always land on a Song - [0b063d9](../../commit/0b063d96082d589a6783734252f378b682e8bc16)


## [0.54.2] - 2017-03-19 - Unreleased Iteration

Source only unofficial release.

### Added 
- Early support for Chord Cohesion Off - [599f08a](../../commit/599f08a38cc9c2a0eaf7d1c08a4e3bbf02e8b977) [7de6738](../../commit/7de6738db267fb34839f20da12ffa558ddfcbd57) [0b45e1f](../../commit/0b45e1f4473fb39e87554c9b03c5381be394bc05) [e6024e2](../../commit/e6024e2bb44cbdb97da0394ab6565d9eb535ea6c) [f5d6712](../../commit/f5d6712b62b676a48f10aa8f86a835df1dff788a) [5759c88](../../commit/5759c88ef1150f5b0724348fc4c2067a5a98ac2d) [0d2688c](../../commit/0d2688c2dac45632c284b779a3454d17f84b422b) [569bf34](../../commit/569bf34fed30ed4e24ba9cb49e45a65bf88b8571) [98302d3](../../commit/98302d32081834ce02d22f118913bd1fd2399e7f) [64a7df1](../../commit/64a7df183aa6d16ab0275bb5e11337aadbbafd81)
- Initial Customize Gameplay work. Movable Gameplay elements - [b77b548](../../commit/b77b548333f3656583db22a19f757a0ec288df40) [0a42a98](../../commit/0a42a980466ffde54aa5b147082d4280783c12a4) [d253366](../../commit/d2533665a65b9d4ce3a7ac6751cf33270ff95627) [a6e4f83](../../commit/a6e4f831c5481478fbbf3c879f04388d6b152703) [e977220](../../commit/e9772202c45700d93c05dd026bc82224c02bd837) [d31d1ef](../../commit/d31d1ef204b5c9b6dea2b447942635caa85ee051)
- Level system for Multiplayer only - [4086fbf](../../commit/4086fbf9c2664899ef53a999ed81f2a934117be1) [c7f2221](../../commit/c7f2221e65e7bca16c69c3a3040be4357729c95c) [801fb81](../../commit/801fb815436e7ff47ceef48b67154f9e6e5addfb) [bcd0477](../../commit/bcd04776c6ca28c75e037460b4a04e3968721462) [44aa0d2](../../commit/44aa0d2588cae4348577ad94ba304cd0526b25d7) [78ff27d](../../commit/78ff27df9e8435f891890142786e09b8c5650e0d)
### Changed
- CDTitles moved to a better position - [20816d3](../../commit/20816d38034aabe89dba7977994d3f23a67de6c8)
- Don't clamp judging old Noterows to a certain window in the past, but still clamp the future - [3716264](../../commit/3716264818878249de087d9ac73f242f60ea3af6)
- Don't garbage collect textures in Gameplay - [b988947](../../commit/b9889476fc6f0ea84b0d4fe596505953b15d1bb4)
- Font change on installer image - [e83ed4e](../../commit/e83ed4ee5b9a904c38bda6f30eea50f1a7a3c533)
- Generate Chartkeys slightly faster using fewer function calls - [db13407](../../commit/db1340700c2cce920eb06b9fd751396898ed1cc5)
- Improve drawing polygon lines by calculating each line before drawing the necessary quads - [d3d2f66](../../commit/d3d2f6605abf421d4bb78a2533fb77b7328f5587)
- Improve Predictive Frame Rendering by using C++11s std thread sleep since it works better - [3040671](../../commit/3040671c5560c5caf09cc56a652d79793676eaff)
- Modify Multiplayer visually to support more aspect ratios - [7ccdb63](../../commit/7ccdb63869c20d97161d624053ef56b435b7cabc) [40d1c9b](../../commit/40d1c9b7c2c8d28fd30f30c7d1d1f71d82ad576c) [e9c891e](../../commit/e9c891ee62d6eee08e67798a90f3efdd5442e5f0) [eeb2c4e](../../commit/eeb2c4e44dc3cdbf9eeb521bc810181561b3c999)
- Moved Filters to a new FilterManager for organization - [758fda7](../../commit/758fda7c4b8b8a1741ccbb10dd0607b9e635f1ad) [f4074a0](../../commit/f4074a0a5aab66edca28e00e2450c836f89f89a9)
- MSD Calc short file downscaler is lightened up - [9a633cb](../../commit/9a633cbcb561dd057b22b06cdd32d597266e0072)
- Only allow D3D multisampling if supported & don't round corners if SmoothLines is off - [df8a577](../../commit/df8a5779d3d419fe444cddee5e2695c74c4c474f)
- Optimize difficulty calculation by serializing NoteData - [3e2509a](../../commit/3e2509a801f0d8761282ecbea141b08081dcaaf4) [204c764](../../commit/204c76449930841147cd969850bad1c2841c1810)
- Optimize bitmap text rendering to use 1 draw call for all characters of a texture page (FPS improvements for all renderers) - [0bf1a6a](../../commit/0bf1a6ae613fcb8f30b05721ffa6691a14656147)
- Set DivideByZero to resolutions divisible by 16 - [f66406c](../../commit/f66406c3196c2382a6413912ecfce6e8790ed493)
### Removed
- Game dependencies in the MSD Calc - [395736f](../../commit/395736f260d2291596a2d159815bf6fb4dc71d68)
- Multiple Toasties - [e377aac](../../commit/e377aac0f19f2768380d0ad05f724c7dd6c8f465)
- Unused theme files - [afa6375](../../commit/afa63754015a877165dcbef8c8e57afae9c92a97)
### Fixed
- Crashed when using the mouse wheel after entering a song from Multiplayer lobbies - [17eb440](../../commit/17eb4407d7d2762cda1c849c4b511ab97e4352aa)
- Mines hit in DP to Wife rescoring didn't get considered - [87e1a59](../../commit/87e1a5992fbad403fa015903cb56f28448d8e135)
- Unicode Fonts loaded very wrong sometimes - [349f056](../../commit/349f0561ca016f7840fbd2e3ed87e0d68a951fae)


## [0.54.1] - 2017-01-30 - Release Candidate

Windows installer pre-release.

### Fixed
- Minor oversight in player rating calculations - [def0123](../../commit/def0123e7c4113cbc1d113c8794b5a48af100ee8)

## [0.54.1] - 2017-01-29 - Pre-release: DP to Wife Conversion

Windows installer pre-release.

### Added
- DP scores are converted to Wife on load - [3d5d780](../../commit/3d5d780d7d9dfb93acd353c8b6c443d8e1c9cd08) [a7c2077](../../commit/a7c207700d2eda16982c6759779b9c4a7b0d4201) [770dd77](../../commit/770dd7772a16b88dabcf405407ca5dc62f687631)
- Game Version Getter - [0ad919a](../../commit/0ad919a3c9a161f916f587f4260c0174e5972e4b)
- Instant restart button in Gameplay - [ba98107](../../commit/ba981076a1de0b2e103447d4bfccda3d05a7ddc6)
- Mouse clicking to scroll on the MusicWheel - [79bc414](../../commit/79bc414b307790b916f0e42c2d4674abf11d9bbb) [48e0fd8](../../commit/48e0fd856d4ce722a68c2f30bb41d6ff6ae47e7d)
- Mouse wheel scrolling on the MusicWheel - [daabe76](../../commit/daabe763cd59a6cb32ef8d3ce4da97b56643385f) [7acb1fd](../../commit/7acb1fdfb78f35a5ade191a60add3e96cd631123) [126f08c](../../commit/126f08c62c7368ae678154ea81fcd753b0ebe84b) [e24fded](../../commit/e24fdede98789d4cc23d6e1552f0205bb2f5ee45) [a8eb1f7](../../commit/a8eb1f74b3ab2cf962669a5454cde64bd81ec45f)
- "Validate All" button in the Profile tab - [3850176](../../commit/38501760679dc8bea44899373c108b39dc250ed7) 
### Changed
- Changing Skillsets on the Profile tab resets the page number - [3fa475f](../../commit/3fa475f0bd674e25bd4cf236090c5213d46d1144)
- DP to Wife conversions use mid-window values instead of worst case values - [d6aea48](../../commit/d6aea48d58bd45212b4a0debd23888449d052b6b)
- Early unreleased Etterna Multiplayer Protocol manages Chartkeys, hashes, Noterow counts - [c4ba026](../../commit/c4ba0261286414ef2b43761ebc857d0f90e866a0) [19fdf65](../../commit/19fdf65ba4770294846ee82098595eb38c761ac6) [8333509](../../commit/83335090eaaeaf5d666c88a97083703481d20414)
- Let SSR calculation functions work for any key that matches a loaded chart - [3b0fd08](../../commit/3b0fd08a0e3ba705f21fae256aa702bdf41c4180)
- MinaCalc.a is linked to Linux CMake - [c796589](../../commit/c79658957ef88b52017b968f7a576b779ffcbdcf)
- MSD Calc loads in a threaded way - [b25ccdc](../../commit/b25ccdc45a6bf91cac8994d2ba1a83ea0571e995)
- Multiplayer Chatbox Scrolling - [2b557c9](../../commit/2b557c964a86c7556b74108955844f5a24b2ee08)
- Only recalculate SSR/MSD values for certain files if they are valid - [dd92b88](../../commit/dd92b881f902ea9f7988516fe951c400bd0e1a4d) [d296982](../../commit/d296982a28e91b801bd39f9b155cd6673be55cbe) [ab67d00](../../commit/ab67d00e1ba19e2951c1aeca97a15f749c142672) [f0b3bb9](../../commit/f0b3bb9acdfaacab07fdfdd2c061a0e759f4cbdc)
- Optimize GetWifeGrade by not implicitly typecasting between floats and doubles - [cc8942d](../../commit/cc8942d315264c99af45bf2270873021b8ca384d)
- Optimize the recalculation of SSRs using yet another mapping technique with Chartkeys - [3008dca](../../commit/3008dca9a816968c29737b881a1d22a90b6f8a8b) [720ab62](../../commit/720ab62c7324e6eba2f2cb4cba31aad2d5e3ed7d) [90612b8](../../commit/90612b89844f07bebbf8de8ff2da087e7fd2ccce) [1a398dc](../../commit/1a398dc36b4d130f2e8c8622a691c4f0df3411ef) [0bd7207](../../commit/0bd7207ef1cfa25d29efe903b0cea693f68c3fe9) [988a0a3](../../commit/988a0a3e856c5947c6fdaee0666cf73ff13886a1) [f4cb23d](../../commit/f4cb23ddb2dfdbf39d452010f4a849f78650682e) [7f0278f](../../commit/7f0278f29f41102d4b0075c512526bc0ed4ddc09) 
- Replay Data Loading returns success information - [846ad3c](../../commit/846ad3cfea316153dbb977bb879688880ee9a45b)
- Wife-based Grades replace old Grades for HighScoreLists - [30b2fcc](../../commit/30b2fcc51f025b8b8fd4f8449173299af56d96ee)
### Removed
- TopSSRSongIDs and TopSSRStepsIDs due to being replaced by a more efficient use of Chartkeys - [13dedd6](../../commit/13dedd66eeffa084f3f85e934ec624e44464bd8d)
### Fixed
- Dropped or missed Holds rescored wrong - [6d24a62](../../commit/6d24a62109acfe5654726979fec01a098b67cc37)
- Getting skillsets for rates outside of 0.7x to 2x caused crashes - [31676c4](../../commit/31676c4eb0c11fed672dff2f1159deb7058a36fc) [597e3ba](../../commit/597e3ba541777c9d9302f91d0f85d6fe67b12e76)
- Rate displays on Song Select were weird and inconsistent - [51b22c2](../../commit/51b22c2b6bb404c3ccd26e3832eec5965713eb9a)
- Welcome window appeared twice in the installer - [951a524](../../commit/951a524f06197fec4f2567c42c43a60f0c52d3df)


## [0.54.0] - 2017-01-24 - Official v0.54 Release

Windows installer release.

### Added
- Early unreleased Etterna Multiplayer Protocol - [d54fb47](../../commit/d54fb4798ac78b106ba0315215912fdd8e338c3b) [3dcd1bd](../../commit/3dcd1bdd7fec749af3736d7f96ffd721c5c01d54)
- Judge Conversion Button on Evaluation screen - [7544862](../../commit/7544862422db1585a1685ebfcc1ff8e8863d8ee2) [6d817de](../../commit/6d817ded4cf54163417e39e9d1f385b9c08f0958) [c2f6311](../../commit/c2f631136befdfa9c910aeb1ae4679631273f7d6)
- New Etterna installer icons [35de48f](../../commit/35de48f9a9ed34c45c64d7cae1264f0386ef3b97)
- Save Profile button in the Profile tab - [9f8c9a7](../../commit/9f8c9a7caf1fc763bcee630fc75f21373da4b8ee)
### Changed
- Calculating SSRs is specific instead of calculating every single SSR for no reason - [de405af](../../commit/de405afb373bbe9f77d7937d8eea560e6c37cb46)
- Optimize offset plot loading - [fce2e49](../../commit/fce2e494f57ca960382ab0ca2e9aa313d3d57fd1) [a83ab16](../../commit/a83ab166e484e72a079e20c7b72305d7222c56bd)
- Profile Tab properly shows nothing for Steps of invalid entries - [253d009](../../commit/253d009608db207c708e34351272dc98686d0ef1)
- Rates up to 3x are supported by TopSSRs - [f26235e](../../commit/f26235e7f6d1988ea2c59f67101efc4d14232ce8) 
- Use std functionality instead of OpenMP for calculating Non Empty Row Vectors - [5a1a217](../../commit/5a1a21725cf0163b38de70e3c2f20ac8c0cb9e0a)
### Removed
- Old SM5 installer icons [b921cc3](../../commit/b921cc302deb58d8884d7bd6fa68c0e50a1bc427) [1c408a3](../../commit/1c408a375c99b1552325f7437184d11dcb4f5601)
- OpenMP - [5a1a217](../../commit/5a1a21725cf0163b38de70e3c2f20ac8c0cb9e0a)
- Theme Version Display - [35c8889](../../commit/35c8889a5707d0e11f409be2b2bf7867646256c0)
### Fixed
- Filter tab elements could be clicked in impossible ways - [7747210](../../commit/7747210461cee54d424441d13e9150f9d983541a)
- Scrolling while opening the Search Tab was sticky - [8b45309](../../commit/8b453098175fae6ffac1365a771b8a414c7e2d67)


## [0.54.0] - 2017-01-21 - Pre-release 2: Replay Data Saving

Windows installer pre-release.

### Added
- Filter Tab for Skillset & Rate Filtering - [d614537](../../commit/d6145377c585efe19ef9635d1f6fbf8671afbfa0) [17520b4](../../commit/17520b47b14167e01655473636720d35c02275be) [2ae00cf](../../commit/2ae00cff074f427ff3a9b127f7663a4bfe899993) [76767bf](../../commit/76767bffde825e17d98bb8985a304f3bc0f5c896) [14fd529](../../commit/14fd529a8351c255969d67f2049564da3194f46a) [41cb039](../../commit/41cb039af168ac87f06325f690ce6d427bde0199) [9686b5e](../../commit/9686b5ee36b0a0cdc8044e49b8b4a698ec80d5bb) [af0ece8](../../commit/af0ece8b118484610888b62059b3f97bb5a1c885) [b53b286](../../commit/b53b286f5ed522a07e30ec467645d79df198472c) [42296ef](../../commit/42296ef8f8c40c3e593050c10bf01749a4ccf19f) [fcb31fc](../../commit/fcb31fc6ef6031571df505cc1c79202e3b203cd2) [43f9411](../../commit/43f9411625f874334d6dcc6bd4a7d5119787d192) [56c151e](../../commit/56c151ef2f8c613d3b6f57068143f102bf13bf1d) [502c56c](../../commit/502c56ccb6bd04119e2dfb6cabc87c0bc6dd158d) [8ed006e](../../commit/8ed006ed24b37e1d8bc4fe591b86a31654d4a4ab) [8516861](../../commit/85168610db443754082b667e80b5c675a62bee4b)
- Hollow DivideByZero Noteskin variant - [930e2f7](../../commit/930e2f7f261ec1031f126c6b13c2b91f4be12b2c)
- Preference for not being forced to reset video parameters when a new video device is detected - [332bc78](../../commit/332bc785cc59627dbdc48af9b506039f9ca178d3)
- Proper top 3 Skillset Display for songs - [a5ab25f](../../commit/a5ab25f129701ce17d209b2115ef7ff9d467a129)
- Replay Data Writing & Saving - [c348b88](../../commit/c348b88c3cc3036460ecf4c0cd276a071fb6eadd) [1762008](../../commit/1762008d90eda7bbea077f42724304a1292dce56)
### Changed
- C++ handles index shifting for GetMSD calls instead of Lua - [26a2778](../../commit/26a2778933bdc66d02d3090dbd05d215e58b0dbc) [d974464](../../commit/d97446423595a76e38308830650f5a75549aa580) [070323c](../../commit/070323c3753ca834076b813831402757b532ade2)
- Changes to the Multiplayer visuals & functionality occur in line with offline changes - [b686dc5](../../commit/b686dc50de2b931d9514f4f1b9fce4ce743c1a38) [55828f7](../../commit/55828f7f540749b4307ee8e5908485b65b151718) [abff056](../../commit/abff05620ef3fd02ee5b000c6fa146caccdd699e) [eda999e](../../commit/eda999ea9a746ea105b371a4944bd132226ccbe2) [30c0d5f](../../commit/30c0d5f6da2b64ca82071c65a3a80e14cbc63add)
- Improve loading of the Profile tab for the first time - [34dbd39](../../commit/34dbd393e211709f78cfebf0c333608ad8d047ec) [a1f7923](../../commit/a1f7923a5f8a458784307937014182cb246494e3)
- LoadBackground on Sprites does not work as long as Tab is held - [176fa6d](../../commit/176fa6d9fd9c74df7b6b88f82d580967a28377bb)
- MSD tab average Notes Per Second scales with rate - [d51f154](../../commit/d51f154ac8aec90fe3f564b24bf8a618dce4f80e)
- Optimized DivideByZero Noteskins by falling back to the original & remove some tap explosions - [e008a9b](../../commit/e008a9b65f1bf4ffbd307e45ec4e76a1337aae40)
- Optimized DivideByZero Noteskin by setting AnimationLength to 0 (for FPS) - [142c8d5](../../commit/142c8d5e9e91f632be04c207c624923ac2aa5fb7)
- Profile tab page number & visual update - [f03dfe2](../../commit/f03dfe26425f06c99c05667b97470a4dbed5a191) [118833c](../../commit/118833cbd8e37e4eabad3b863adfadfee7ff772d)
- Renamed the bar skin to SubtractByZero - [012fb8d](../../commit/012fb8d05fb5871eb4e82941de0bf4977d64e318)
- SSR rank lists reload after Evaluation screen properly - [72dd7a1](../../commit/72dd7a1b782a67c192dd4bda3693c9f055a07b14)
- Uninstaller does not need to delete the Theme - [49aa617](../../commit/49aa617ce6e16c868a9b07a97afe4ec0434dcfc5)
### Fixed
- Background Type Player Preference didn't persist - [f022192](../../commit/f022192714c6456c27bb6e23e5cefa4bb80d8046)
- Favorites could be duplicated - [4889947](../../commit/4889947a20dd6fa57ee8278b671d153b88f53129) [ae2a9e5](../../commit/ae2a9e5f7b5fe6e7a72e53a09cf803fb0286aae2)
- Favorites didn't show up correctly - [5a1ca79](../../commit/5a1ca79beeb33889d41e3cd6f14b43ae7d681052)
- GetHighestSkillsetAllSteps failed due to a typo - [bcae636](../../commit/bcae63699c015c0d8fb87427b66ff2ad5e946da4)
- Game suffered obscure crashes when viewing profile due to invalid SSR values - [28a4129](../../commit/28a41294d1a4c151ff247bfa669f398b8ec199c0)
- Garbage information was outputted from HighScore lists after new HighScores were made - [3f1b927](../../commit/3f1b927126ba262d3114a569b3c3368c32f12f03) [49433af](../../commit/49433aff6d70cbfcffb25705e508269b06b220b4)
- Halved & Semihalved DivideByZero Noteskins had pixel artifacting - [032564d](../../commit/032564df43d9d6a97432765dd9d602e0f306dd9a)
- Lane Cover BPM display sometimes had weird floating point display errors - [5e40bf5](../../commit/5e40bf54c8ac5ee8cb752db822f822fe66d7d32a)
- Mini Player Option loaded wrong because of the load order - [7092e5e](../../commit/7092e5ed69e5424af5b3ea2478a4ef8cea92bd9c)
- Rates outside 0.7x to 2.0x caused crashes - [f8e746e](../../commit/f8e746e071a7f0a191b2a16678823cfe05fe4b81)


## [0.54.0] - 2017-01-12 - Pre-release: More Specific Skillsets

Windows installer pre-release.

### Added
- Appveyor & Travis - [a73fa11](../../commit/a73fa1109c0f597d95b1e48d1cb2088d80d0b387)
- DivideByZero Noteskin Variants
  - Bar version of the DivideByZero Noteskin - [44b90bc](../../commit/44b90bc685cbf7acc9131bb344e2ad40b73b2076)
  - Halved version - [c6bd0cc](../../commit/c6bd0cc4202666535898088098845b0c18c81dd4)
  - Orb version - [44a37a7](../../commit/44a37a7a511e637e51cab60ab42bc572285a52bd) [44b90bc](../../commit/44b90bc685cbf7acc9131bb344e2ad40b73b2076)
  - Semihalved version - [7e9ed57](../../commit/7e9ed571202008f586ef4f97c99aea3a22d9deda)
  - DivideByInf (192nd coloring) - [f2a7449](../../commit/f2a744948c056d4d0f80ff67f0664d682c06c280)
- Invalidating scores from ingame via the Profile tab - [0a364c1](../../commit/0a364c14d52b7863ded223c924a487bbb72d4c2d)
- Preference for Horizontal Scaling of NoteField - [c16d57d](../../commit/c16d57dc784fcb198cb0f35e13ab0a0e8fc79065)
- Profile Page shows top scores of each Skillset - [75f5d50](../../commit/75f5d50ad8245abfafa64babf7b41e465550ac67) [7238dc7](../../commit/7238dc7571144f7feab2ff0a87e553b9b5e816c2) [f0468f5](../../commit/f0468f599163de5039b6ce6f8bfe2bb90a753c3e) [3ce8baa](../../commit/3ce8baac632f4b5cbc556b9cc9f9e8d572256bc5)
- Skillset categories are split into more specific ones
  - New Technical Skillset - [1253f09](../../commit/1253f094c08fc77cd2422b1600c53e737c2e73c5) [58bb603](../../commit/58bb603b0dde34184f70378acb21a2531147aeec) [f2b8a6c](../../commit/f2b8a6c99c2574941f92724d89c6c5fd907b4a09) [57390b6](../../commit/57390b62ab89b901e627467201412e43aabdcefd) [86ebd28](../../commit/86ebd288474aa49d758159f0043679354d674283)
  - Jack is now JackSpeed & JackStamina - [854dc2a](../../commit/854dc2a418691b8b10ce773d86d51fed72fff917)
  - Speed is now Stream, Jumpstream & Handstream - [854dc2a](../../commit/854dc2a418691b8b10ce773d86d51fed72fff917)
- Song rates of 0.05x are allowed - [71ada86](../../commit/71ada86739b8cf215fac46c744d279664b9a8b5c)
- Tangent Tectonics by Sinewave as a new "official" Etterna song file - [df8b6dd](../../commit/df8b6dde151e05f2cdcca43db7c317f0255b31bf) 
### Changed
- Basic cleanup of MusicWheel messages - [04afbb7](../../commit/04afbb766886e3c7342d6c5f1c91acce0ae10141)
- Heavily improved Multiplayer visuals and playability, to be brought more in line with offline play - [fe2bd47](../../commit/fe2bd473784f50e32389d6b17d1deaf76b1146d8) [0bc44b5](../../commit/0bc44b5d06dd8d41fabfa1677c25e143a4b19642) [aa10d39](../../commit/aa10d39c792254989a868c3a95bbb3e2ab490a09) [8d12f9c](../../commit/8d12f9ccfc8d530542556b649c553bff0e192594) [9360c18](../../commit/9360c188c8974c10dfc4a3a2e60eddd83e3aed1e) [11e4ff0](../../commit/11e4ff0eabde24a97249151642bef3452b5e58a0) [a0b2355](../../commit/a0b23551889de73d11e0e3b183022f0dde0a0e33) [eaca47a](../../commit/eaca47a2220882246f562027c0472405dfc848cf) [aa8383e](../../commit/aa8383e170a4419262306613b42f0a2fb9501394) [3c12c85](../../commit/3c12c855b16482f92ff80d308d781bed3741174f) [431ef6b](../../commit/431ef6b4028a6e5a106c35a8b5cc3cf6868bf8db) [ffe1856](../../commit/ffe18561815b2a8201a7711778ce7b51e3077fd0) [545f29a](../../commit/545f29a550503d9a185cb98a4db44aa498b700bb) [753b2bd](../../commit/753b2bd50591e37c58151d61e6c3af6611fab80b)
- Improve cache space usage by the calc - [a7e0be4](../../commit/a7e0be41b1667e49d8e99817ade92519323d5cbd)
- Lua has access to Wife & Wife is more hardcoded - [5ee64fa](../../commit/5ee64fa06b08393f24bb9623f7f6c4b0618d991b) [45f497e](../../commit/45f497ea176dfe3717bac7b1c197475e67631ede)
- Many MSD calc updates - [c8a7c79](../../commit/c8a7c79563a526805b81ab6df52d22b6db80aca2) [d3b6c5d](../../commit/d3b6c5db3933975b78e65728a0ea41a30f943e83) [e0bbddd](../../commit/e0bbddd3c0ca0ecd8e7f8792eb1c9a27a5e51325)
- MSD interpolates for non-standard rates (non 0.1x intervals) - [44a80fe](../../commit/44a80fe662160af2724f9dbce68a3afb82548fe6)
- Note Per Second vector generator re-enabled - [fc51138](../../commit/fc5113849066669dca22a64268300c58b10656e8)
- Noteskin Player Option row is always 1 option at a time to prevent confusion - [a2e5646](../../commit/a2e5646fd7cf5d8b814f2e98e0c66777151aff05)
- Player Options slightly reorganized to be chronologically ordered - [f0fe666](../../commit/f0fe666e249758f332e93eea47647e9551acc283)
- Skillsets are more generalized internally - [a625318](../../commit/a625318b6b61a805482ba57379abb9e7a10505a7) [df526ed](../../commit/df526ed9399504fbacfb841df0e037fea80203c3)
- SSRs are not calculated for scores under 0.1% - [6f83788](../../commit/6f837887594bead76392018dc864922b4056f3c3)
### Removed
- IRC Notifications from Travis - [d9bd3f3](../../commit/d9bd3f3b01f76da959381011fbc7df83c6453d7e)
- IRC Notifications from Appveyor - [7c11e2b](../../commit/7c11e2b4588a486e076b26c14bbd2f62c85448a3)
### Fixed
- Crashed when moving to a negative location on the Profile page - [4722f33](../../commit/4722f33e95eb4fe2d6a2ead4996909964d3aa87b)
- GCC had a compile error due to non-const rvalue - [ba1e734](../../commit/ba1e734f628e97f080f6b8b2bfc23496225d1473)
- Lifts were not considered at all by the calc - [0785474](../../commit/0785474a34a784492dc9bff39eed14c0f3068684)
- Non-dance-single scores sometimes gave weird SSR values - [587b55e](../../commit/587b55e9a55cec51efa09698ea17fa1431612c6a)
- Parentheses placement for 1x and 2x rates on Scores was bad - [15c4faa](../../commit/15c4faa2bdab741279ec52cece45e4cec56bf3d1)
- Selecting a song from Lua didn't select a song - [ac955d7](../../commit/ac955d7acc1b49bfd6bfa68d6964fbd8df476df4)
- Window icon was giant - [dead101](../../commit/dead101d49beba0158d6b3740191585a4b55bd3f)


## [0.53.1] - 2016-12-25 - Hotfix

Windows installer release.

### Fixed
- Game crashed due to null pointer exceptions on Steps objects in the Evaluation screen - [081c546](../../commit/081c5460a3c6ff83bb0b8dd661f71150be7da1fe)


## [0.53.0] - 2016-12-24 - Skillset Ratings

Windows installer release.

### Added
- Colors based on MSD - [f712c4f](../../commit/f712c4f74be55251d960eb676b33fc235d626488)
- Colors based on file length - [e87a356](../../commit/e87a356304bf8003335f8d6663a5e291eaea69dd)
- Number of Favorites placed in bottom right corner - [630e753](../../commit/630e753b8724a223ea384f7fd5afd657488b007a)
- Session Timer - [81b47c3](../../commit/81b47c3bdde437f181c7b4b1a5132629f20231dd) [2b6f8ee](../../commit/2b6f8eed8cf4b604c2a5b74ea2b93080e3ff03d5)
- Skillset Specific Difficulties & Ratings - [39559e0](../../commit/39559e0e3a5ddae60efbf7e7af09325fcefca66f) [d4f2b04](../../commit/d4f2b040890f244f00d0f9a5f61d3880586df27c) [3551116](../../commit/3551116ac4ab3ba5c11d0041f8215cfb1322cf75) [104edea](../../commit/104edea8e011512d5800c78374d254ca160d7097) 
- Toasty - [8305648](../../commit/8305648f26b6c27183e8d3d8852327b4ba969fe2)
- "Worst Case" DP to Wife conversion - [07be19f](../../commit/07be19fdba343777227d78800a97a1fd3c4cb7b7) [d8520f0](../../commit/d8520f02a4ce0b497a01d0eb48c31ed5dcba75ce) [5343373](../../commit/53433736176ac10df6cd9305feae4809addf025b)
### Changed
- Autoplay scores intentionally score bad according to Wife - [a4ef6b6](../../commit/a4ef6b6eecf8a11c6a7031389040e3fbc03da883)
- BeatToNoteRow rounds again instead of typecasting - [f719a5a](../../commit/f719a5a4c0e43beee15bf768e843def338699e4f)
- Centered Player 1 preference will move Gameplay elements - [cf1ea4c](../../commit/cf1ea4c524d2bc5ecc08193abb6f22bd5505968e)
- Color Config allows MenuDirection input - [03f8947](../../commit/03f8947ebf02ab4bfafb35eafd1ec972ac60bbeb)
- Don't delay sample music playing - [bf8419a](../../commit/bf8419ab38903064af50a722ed2c1588e263fa68)
- Hold Enter to Give Up displays 0.2 seconds slower - [9f86da0](../../commit/9f86da0e2a9d71f758f19854a4d05c7e2c2caa60)
- Invalid scores should not be best scores - [8f66d61](../../commit/8f66d61c7932b9b11e19ff1c93444a474487f4a2)
- New Loading Splash & Window Icon - [2352dac](../../commit/2352dac6cd31bac18ef84b562f9e5aa4292ec0ec) [9701017](../../commit/9701017fde218493a918e0a9ca8cdc28326dadd2)
- Target Tracker properly interacts with set percents or personal bests - [9d65ca9](../../commit/9d65ca98373fcabac76b329f70829cbd708e7d17) [5a8ad7a](../../commit/5a8ad7a31582edea0cfbd4c4fdf03e83445f6781)
- Sort HighScores by rate forcibly when grabbing them for Lua - [568ba80](../../commit/568ba8058296164e1d99df023a8dd27fd8b3ebc7)
- SSR recalculation is only done to loaded songs - [bb2d5a5](../../commit/bb2d5a55879ad085c1e357dc940e1d970a0cac0f)
- SSR recalculation is handled in a better way to allow less hardcoding for recalculating - [d8a337d](../../commit/d8a337d17df707d9803f942fc1523b0667733dc9)
### Removed
- Text Optimization when not on D3D - [22b940b](../../commit/22b940b3e42c2531985a9c75ee93e41f4572a7ac)
- Various unused Lua Scripts files - [705612a](../../commit/705612ab9790961203b5d45bf72858111dca9f10) [afca92e](../../commit/afca92e9fa05884aff6db962edce941ad7cc1ea9)
### Fixed
- Background reloaded for no reason when switching theme tabs - [321092d](../../commit/321092d8b024cf3dd800edcc5951b1af3d6ff3d7)
- D3D Render States didn't function - [0394d35](../../commit/0394d35997365267f0a6055c38ac144706f2a1a6)
- Delayed texture deletion delayed infinitely - [6ac403d](../../commit/6ac403d642262e49e7a46c8e3877f11116cae05a)
- Display BPMs was not showing proper BPM when on rates - [1dddd9c](../../commit/1dddd9c1ba8ae851b253dc50d863fb51f861f9fa)
- Edit Mode crashed upon Hold judgments - [294b1cb](../../commit/294b1cbffcc3353cefad3f2314f70b7e01e7c406)
- OpenGL/D3D text rendering was broken to some extent. Pointers were nonsense, and some logic was broken - [59c2b7b](../../commit/59c2b7b2100ce68f0ee1524a9f6495257660d827)
- Multiplayer was visually broken. Redid almost everything - [0e3fa51](../../commit/0e3fa51e142bbc273c56fcebb7b484b8980cf742) [0e3fa51](../../commit/0e3fa51e142bbc273c56fcebb7b484b8980cf742) [2be527c](../../commit/2be527cd570f6338fa0724d095964c67d0b54b38) [6d4a71b](../../commit/6d4a71b12aed7457b78b534c70866f4052f1cfe2) [d21a97d](../../commit/d21a97daf290b9fc593dd726d3b1dd25a01d81d5) [ee807aa](../../commit/ee807aa4084e5afdc12bbb718fc92c143ef5bb8f)
- Some Evaluation elements were badly placed in widescreen - [845afd7](../../commit/845afd7216c3283e835b9c49c2312f50109c06c8)
- Theme options did not properly place the Progress Bar - [b21018c](../../commit/b21018ca372123466741a69b10c0f2dbae8fc9e1)
- Wife & DP rescorer didn't function on Score Tab - [2d0174e](../../commit/2d0174e54626145ea01ae8ee463f7571727fa7ba)


## [0.52.0] - 2016-12-14 - Song Favoriting

Windows installer release.

### Added
- D3D Render Target support - [a367c05](../../commit/a367c05eb67b3975c7bcd8813fd79740b72a6ae8) [577845e](../../commit/577845efca1bf9b0d4d4409e0852cc0b3f3b71cd) [ed45d5b](../../commit/ed45d5b439f81822863df53f66c7e19604db84b4) [f1cd28b](../../commit/f1cd28b5c09e0b42864bbdcbca2ac710f33982c6) [0b7e06d](../../commit/0b7e06dbbecb6e1bc1744bfadc8c0c17959bf2c1) [b3efd1d](../../commit/b3efd1d0778706f3c0946720c7c74083a51c2a9d)
- Judge Conversion & Chartkey History - [80c7ceb](../../commit/80c7cebd529cfd502d78f6a182b7991f5ebe5bda)
- Judgment Cutoff Lines on Offset Plots - [39e5fa8](../../commit/39e5fa8a3e5ea6a00af686fa2233143e46398efb)
- Song Favoriting - [7f6f053](../../commit/7f6f05337ca42f8874e5397631eb64b79714bf8a) [717e175](../../commit/717e175a40bc365f415bd12dce2703c0e679ef9f)
- Song Search can now search by Artist & Author - [4599f9f](../../commit/4599f9f6032e9937a91d11403750558af1f6a66c)
- SongUtil function for getting Simfile Authors - [43d3392](../../commit/43d3392d86f56be2e10957872645cd0c47d3e7d1)
- Sorting by Favorites - [428bf72](../../commit/428bf7222341e56418b2dab3534292c073006d43)
- Target Tracker can be set to Personal Bests & Target Percents - [cd890f5](../../commit/cd890f57a090fbe8321a34860cf076f60886e541)
### Changed
- ArrowVortex is mentioned ingame as a better editor in the Edit menus - [149945c](../../commit/149945c77b5ca89e0c49c54ee79b95c8508b39a2)
- Clean up DivideByZero Noteskin Rolls - [d9bbd95](../../commit/d9bbd9523dde2910efec5ebf121cd307030e72c2)
- Don't reset search strings when entering Song Search. Press Delete instead. - [8b1acaf](../../commit/8b1acafee7aeb62a13895e63871e581bdc1c510e)
- FailOff/FailAtEnd will not produce SSRs - [8737a14](../../commit/8737a1442ded55ff76d220c17b943fdb1f3aa852)
- Improve text character rendering - [e2041b0](../../commit/E2041B0478d766c2d7fed4ce08c5213bee56b990) [a04b891](../../commit/a04b891677012fa412296641f5155cc89e9f5977) [35d7c53](../../commit/35d7c5336722c853825d3bb055106bb577d8f736)
- Moved Wife2 function to RageUtil from Player - [868f329](../../commit/868f3291638d394ae457679f5e41e900ff45a067)
- More typecasts change to static casts - [75354e8](../../commit/75354e8581acc57f6583e5922c37708930063d1b)
- MSD Calc updates. - [01d58ad](../../commit/01d58ad0c7616186e249ef9a1e9e37251e280be3)
- Music display shows file length, not audio length - [b696378](../../commit/b69637866178d928d7ef9b7cd8c8892708a90c40)
### Removed
- Writing old Groove Radar data for HighScores - [e04f7e0](../../commit/e04f7e0d5cb0f257b5f7099c2b22d840799c51af)
### Fixed
- DWI files crashed on first load - [1b2294c](../../commit/1b2294cb4cfdaca3e6be169ed58f9405e9fad32b)
- Edit Mode crashed immediately on playback - [f1e4b28](../../commit/f1e4b280739a27cb0af624a3f088af64a481d2ee)
- Linux could not compile due to some type issues - [eb76405](../../commit/eb7640534de58d4226d1e4cc1b8eb4cb2059a21c)
- Wife Percent in various locations had bad decimal placement - [f29df05](../../commit/f29df050bf4b3cd02442f5a21ad30b284679542b) [d45e482](../../commit/D45E482924f6d81a558e764eabaad358d0d61ab0)

## [0.51.0] - 2016-12-07 - Song Search

Windows installer release.

### Added 
- DivideByZero Noteskin Rolls - [6f19852](../../commit/6f198526182c78ac6f6c8cce55d276e55b91a86f)
- Grades based on Wife Percent - [e97836d](../../commit/e97836da3a71ae3636669a66058e330f9ac3c535)
- Player SSR (Score Specific Rating) recalculator functions - [a1f4df8](../../commit/a1f4df86aa00dc9d5f57f96b0873de741e212aaf) [8edfc38](../../commit/8edfc3824cf40031f130a9a1fe649ad75bc9c805) [1a0339b](../../commit/1a0339b8a69c4dba45b164deff3fa980f0e0a7ea) [1741687](../../commit/1741687ee9834bf98fdcde9c9b0a562eb7cfada6)
- Song rates up to 2.5x - [9543b49](../../commiT/9543B499774d672f038ff1666e2a28e30cb29a54)
- Song Search - [4795319](../../commit/4795319b309d8af2370ce6fbf7afe81a3bd57bae) [4f64c6b](../../commit/4f64c6bbb84278c331c2d0bb1f400288ed56fbc5) [5a65b28](../../commit/5a65b28338a3782b96726963eac41f06d8b6f5a5)
### Changed
- Installer won't overwrite preexisting files & won't delete the song cache - [ad5b0ff](../../commit/ad5b0ff0f725577f89a8ed31e3b347789eef4b82)
- More typecasts changed to static casts - [772c359](../../commit/772c35957ecefd57c3faa72b55e7ff33cdda9ef6)
- Max player name can be 64 characters instead of 32 - [a1f4df8](../../commit/a1f4df86aa00dc9d5f57f96B0873DE741e212aaf)
- Use the floor of Wifescore values when displaying them - [0028bd2](../../commit/0028bd23d92cea8518c6ea29f37422d1b1ea53c5)
### Removed
- 90% Wife Score requirement for SSR calculations - [65a620d](../../commit/65a620d6e981aeb77963562cbd51137fbea6126e)
- Default SM5 theme & Default SM5 Noteskins - [182ea03](../../commit/182ea03cb472940c8b61b4f7303ac0da88b81323)
- SM5 Song Folder - [56609bc](../../commit/56609bc33cae0c53e98ae25ec65725625e2d9ffd)
### Fixed
- Default scroll speed increment adjustments were not useful - [7e399fb](../../commit/7e399fbe3d793b4bf7a2796e3d989d9029fa4c2e)
- Game lagged when modifying Lane Cover - [db03a65](../../commit/db03A65108D1649697887815f48cf04b76469e10)
- Judgment Counter & Display Percent Gameplay elements had bad layering - [9191d54](../../commit/9191d54d8fc89ec0689cf4064213fe0dd2a35d7e)
- MaxHighScoreList preference was not properly ignored - [271a4be](../../commit/271a4bea60f3ee4116a4287e922020c0de6e2bbd)


## [0.50.0] - 2016-12-04 - Public Beta Release - MSD & Wife

Windows installer release meant to be installed in its own clean directory.

### Added
- Installer will install VS2015 C++ x86 redistros & portable.ini - [cc8add3](../../commit/cc8add3af0041be3bca33124b2975757fa8fe103) [5a5eec8](../../commit/5a5eec8681705e4d3ecc6b201190b31526fe0808)
- Millisecond Scoring system (MSS & Wife) - [0e02141](../../commit/0e0214155a23570f23bd9a68332dd6f7d30aaf11) [0c1eaf3](../../commit/0c1eaf311c05fcf7e1498f691386df1a505c7aa8) [bc26e1d](../../commit/bc26e1dd2ccda6a317ff9696228676238fe2af7e) [92f3398](../../commit/92f33981e6449141bf07b88bdf5990e11a12ae6f) [bd9ad63](../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
- Mina Standardized Difficulty (MSD) - [bd9ad63](../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0) [065aef4](../../commit/065aef43d98e8dcf3738028b3d533e756af159ed)
- Polling on X11 Input Thread - [e8964fc](../../commit/e8964fcb5753ba9734ed978e8950f4a7af31acb9)
- Predictive Frame Limiter - [8fbd324](../../commit/8fbd32486587ae5bfaf7e9386a8ba5c81fb5d45f) [b27d673](../../commit/b27d673159969efbe94ea4a4540c0f5e42886b0c) [3d0f895](../../commit/3d0f8955f419d9dce968aff57d6d588a27e57201) [4a1dca9](../../commit/4a1dca90bd321c25367b2f2ed0957ccb0fe4774b) [edbb2df](../../commit/edbb2df59893a942cd9d76f6935c4e9edd03fda9) [a77e8b7](../../commit/a77e8b7af40aaf12f3f573d77e122ac0c96375dc)
- Preference for Allowed Lag in milliseconds - [87a8085](../../commit/87a80858e21969ea52bcda603503cd29b3876a28)
- Some new songs come shipped with Etterna - [bd9ad63](../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
- SongFinished message for Fail Grades - [70a432f](../../commit/70a432f58be30ff2a02d786d6141dc8d038f5766)
- The Til Death theme & some optimized Noteskins are now shipped with Etterna - [bd9ad63](../../commit/bd9ad635bbb65ba6b1b1568d103ec6bb0050f4b0)
- Uninstaller will uninstall all proper files - [5abe56f](../../commit/5abe56f64b7d07328f23aae5f1e61bbdce535fe3)
### Changed
- C++ Standard is now C++11. CMake standard is now 3.2.0. - [89eda0d](../../commit/89eda0d5c1d5efbf13bc8f8e447565a093dacce1) [b0821f8](../../commit/b0821f858c328c4f4793ce8b2a3d7b8a8bd23839)
- Caching and Uncaching Noteskins no longer calls MakeLower several times - [f7f2e52](../../commit/f7f2e52a1a2c605cbf96bbfea9ce5d25010ae11e)
- Calculate instead of search for visible Beats for Taps - [cc491ff](../../commit/cc491ff538bbaf8de99c83ffd6228931d58aa3d2) [028b8f2](../../commit/028b8f2ee3f1a8d5e3d9c60cba1ea06cd0f51072) [1e16e08](../../commit/1e16e087f43655780508749ed892ebcd1ac36084) [26b8064](../../commit/26b80644b59bce730b793bdd22cc401ad14531ba) [28cce6d](../../commit/28cce6dbd244894b011b14be6c52168a0bd9581c) [7efa383](../../commit/7efa3834adee23242e1b4c7a8d504870d762c4d0)
- CrossedBeat Messages can be sent - [e973335](../../commit/e973335c4a877814c8363f2a32b7e45c8a854bc9)
- D3D Display Resolutions are now unsigned ints because they can't be negative - [aec2955](../../commit/aec295548e28ffc46a3d0f212c92a8bc8b6b410d)
- DancingCharacters & Lights are allowed to update - [fa2870d](../../commit/fa2870d7c32d3dc1a9fc139cdbbcd4295434748f) [956b81c](../../commit/956b81c6357ce625bd3dbead9a52900ae0c49862)
- Default preferences & bindings are more the commonly used ones - [16d03b2](../../commit/16d03b212440002ab4a6292fcc28f59818070d67) [e8964fc](../../commit/e8964fCB5753BA9734ed978e8950f4a7af31acb9) [1b49619](../../commit/1b496196d8a98b9b9996d1e88242f93919d213bb) [053a655](../../commit/053a655f2a43f0aa6eaffa1b6061074c79391c58)
- DirectX is the default renderer on Windows - [5a5eec8](../../commit/5a5eec8681705e4d3ecc6b201190b31526fe0808)
- Don't autogenerate courses by default - [935d764](../../commit/935d764d24e70c1286e7612029794355ad954f35)
- Don't autogenerate lights data by default - [d973824](../../commit/d9738246f231f1364be43a20289d7c001bf7b270)
- Don't calculate the NonEmptyRow Vector elapsed times every time NoteData is loaded - [1f1b75c](../../commit/1f1b75c102c8cc74c9f0050639c6a05ecaf1c8b0)
- Don't get a song cache hash unless needed (songload optimization) - [2b3722c](../../commit/2b3722ccdbdffe2ee1d3c3f0b2a442c98a020cf7)
- FPS only updates per rendered frame - [6404aee](../../commit/6404aeef183b5a74fbb261d90c98a5bea1b8be28)
- Generate Chartkeys in the load from disk to cache process - [c21c490](../../commit/c21c4909ca9c2cf087a46247db924dde60ea5466) [c154cb8](../../commit/c154cb87c6af0ecef6e046ab93c2fc5cc0eb42e6)
- Improve song load speed by optimizing a for loop - [10808b4](../../commit/10808b4b707c4e2908e0ffffd3af618f6606bb73)
- Improve stat calculation lag & use std::chrono for timers such as device timestamps and fDeltaTime - [8bec801](../../commit/8bec801c67bb4a70ad8919ceec57aade10b08bfc) [57bc635](../../commit/57bc63571aead7b8bfdbf8c06faffe8a6058cd3b) [d59c4eb](../../commit/d59c4ebc9af13992422af7f39cb8d08aebb2bbe4) [3a1cab2](../../commit/3a1cab21551e4c45A21801FAb7906d052327794e) [33802bf](../../commit/33802BF5104a2fc0e469c85d49a05a076d9a281c)
- Invalid (autoplay & negative points) scores don't save - [de0eb09](../../commit/de0eb0947dbe3699aa8becbd37c4f629ba49e92a)
- NoteDisplay updates every frame again, since the game runs faster - [150325c](../../commit/150325ca733e0765099a934bf91ee8ef7ec0b771)
- Optimize calls to BeatToNoteRow by replacing it with a function that uses the calculated timestamps from the Non Empty Row Vectors - [de1ebe0](../../commit/DE1EBE095C44dbc951c34292e24d3fd3ff6296aa)
- Optimize FindEvent function in TimingData - [dd6cb56](../../commit/dd6cb560b5699b44247f0167f6d90888213c19b8)
- Pulled SM 5.1 changes for cache writing & loading some SSC files - [2196aa0](../../commit/2196aa00de6a75f1cecbb76f2b74e95d055263cb)
- Pulled SM 5.1 changes to remove the custom Mersenne Twister, replace it with std::shuffle - [c158f80](../../commit/c158f80bd5e745a0ee89162a997965650e5e17b2)
- RStrings and some other things are now almost always passed by reference - [dac3888](../../commit/dac3888b71d368dbcbf3bca520f8b511afe43070) [3ef472b](../../commit/3ef472bcca457f04e006c856640ef4f8104beefd)
- Replace Rage toUpper/toLower functions with std functions for a 32% speed boost on song loading & optimizations for Screens with lots of text - [d4331d8](../../commit/d4331d806df4fd2ce2d3748684ec2e2c140e856d) [e778a35](../../commit/e778a359c418d3c3e8618de270e4e52cbd937e6e)
### Removed
- Redundant NoteDisplay IsOnScreen function only checked what is already checked for all cases elsewhere - [0de46d1](../../commit/0de46d1737515665aa52efb8c172008fa9ce553a) 
- Unneeded Hold body rendering overshoot - [dd0c1f9](../../commit/dd0c1f9023bbf37f7b1634dfc61054e4100ff3d7)
### Fixed
- Edit Mode was crashing upon any Gameplay Judgments - [b5be683](../../commit/b5be683a86fa17b192d261357892c6d28a3648ad)
- Holds in skins which don't explicitly flip the head/tail were improperly rendered - [b337430](../../commit/b33743081eee80693a7cd320ba6d7d6809887e7f) [acdefc2](../../commit/acdefc2805c134cfec7396ff9b45f70945035f10)
- Hold tails were wrongly flipped - [aca2bd4](../../commit/aca2bd4fc3b4475f22bf91287dfa0ca4f2f5489c)
- Installer didn't work - [46bdf4b](../../commit/46bdF4BD7DFED965135e481dfbf395816aa3f1a2)
- Unlocked Gameplay FrameLimiter froze the game sometimes - [2b4d67e](../../commit/2b4d67e0c075f6f4a54247602d020a06891f03dc)



## [0.03.0] - 2016-09-09 - More FPS Improvements

Binary only release.

### Added
- Chartkey and lists of non-empty songrows to NoteData objects - [b6c0d03](../../commit/b6c0d03580667ddb637ab748c7c9eaf31087ad05)
- Preference for PitchRates - [7526783](../../commit/75267835ada422ea014be09b408a78bdc98f0752)
- Preference for FrameLimit & FrameLimitGameplay - [c7107ef](../../commit/c7107efbd6cf90e1728fd4d3f8bb84e8bc90593b) [68fbbc0](../../commit/68fbbc09e4e3dd04068809052c21a2308e8b917f) [4872f09](../../commit/4872f09a4fd5969830401d705e97db68855fdb13)
### Changed
- Don't update NoteDisplay every frame (twice) - [568d9fe](../../commit/568d9fe9181d9930fa4252fd340001d123dc40ed)
- Don't care about setting frames if there are no frames to animate on NoteSkinAnimations - [a100fc0](../../commit/a100fc00add95d933be939af4554aa45b040ddf6)
- Don't update Tornado/Beat/Invert when they aren't on - [aabcc15](../../commit/aabcc1539b16776d02436876466cda3074af26dc)
- Don't update DebugOverlay if it's not visible - [35a33b1](../../commit/35a33b16c137b242279f09465532d83564206792)
- Lights and DancingCharacters no longer update in Gameplay - [99fab7c](../../commit/99fab7c1eee68e389b143cafda1101af30890f03) [fb2266f](../../commit/fb2266ff4930af00884ec6011a6560115046e8dd)
- MusicWheelItems show the highest grade for a file - [14a6bed](../../commit/14a6bed739193154852c53935755beb34f425aa8)
- Optimize calls to get the elapsed time for Beats or NoteRows - [a6f9831](../../commit/a6f9831afc6a1ba33182f872ff4cab9e5c4c2fcc) [41aafa4](../../commit/41aafa4cd3e0ebeb160569ef0ac12a17bdba09a8) [ddf6bc6](../../commit/ddf6bc6c3708e5e08895b332ae4fca4123214c56)
- Pointerise GraphcsWindow's GetParams for an FPS boost - [563eb77](../../commit/563eb77c3d5f87a4519b6bcede732420725a2b23) 
- Replace many typecasts with static casts - [cf92975](../../commit/cf92975dcb871909e1a856a58a391084404dc640) [69e5251](../../commit/69e5251e5ab8aa15d10ef3f125cc08c18352d807) [8d9d23a](../../commit/8d9d23a45cac5f81194bf4558358a2bbac213196)
- When clearing FileSets, only iterate through what we need - [4a36892](../../commit/4a3689216321d0fa6616e514b9299493e838377c)
### Removed
- CrossedBeat messages for every Gameplay update - [151c550](../../commit/151c55046812d4754db897aac910a3d57bddb389)
### Fixed
- Gameplay was stuttering when using DirectSound-sw, caused by duplicate positions returned from sound drivers - [427f9f0](../../commit/427f9f0ff2648a4d2281995f814cde331f66e372)

## [0.02.0] - 2016-08-31 - Early Experimental FPS Improvements

Binary only release to replace Stepmania.exe.

### Changed
- For Windows, use D3DX instead of RageMath for some calculations - [2d4c053](../../commit/2d4c0538a683da87c0a09ac68304f953b04542fa)
- FPS with Holds on screen improved by 37%
  - Don't draw the Tap part of a hold within DrawHoldPart - [3ba8bd5](../../commit/3ba8bd503a207e285880b199d30887d9b74ade01)
  - Don't glow the Hold until it should glow - [3ba8bd5](../../commit/3ba8bd503a207e285880b199d30887d9b74ade01)
  - Don't draw the Hold body head if the head is past the tail - [434eaa8](../../commit/434eaa846f5e8b43b139589132ec26a37d335cf0) 
- Reduce the internally bloated nature of the LifeMeterBar & StreamDisplay (for FPS) - [6b05c2b](../../commit/6b05c2b89ee2bcbb6ec6fabce09a3e953a0c09e7) [a2129a3](../../commit/a2129a39e9747e2bcf5d2a8b5f5e0a2017693352) [bbd0be0](../../commit/bbd0be03edffc47e7a926bbe12f134c6e82ec535)
- Replace nearly all lrintf calls with lround - [a559386](../../commit/a5593868c478a1b6c66b628f28805190818f8bb3)
- Truncate instead of round in a critical "float to char" function - [2faa10b](../../commit/2faa10b023a66745fe23aa0a8aed96fd0341264a)
### Removed
- The need for Windows Aero for VSync Off in D3D - [8e0a94f](../../commit/8e0a94f0c7806a850d84df65750189a8d5e95ef7)
- Unnecessary check for judgeable NoteRows - [d412cbf](../../commit/d412cbf23aebe6f464a48aec7b9b6f5bf1795524)


## [0.01.0] - 2016-08-26 - Early Experimental Release

Binary only release to replace Stepmania.exe. Initial branch off of [Stepmania 5.0.12](../../tree/v5.0.12)

### Changed
- Replace BeatToNoteRowNotRounded with BeatToNoteRow & typecast instead of round in BeatToNoteRow, because rounding is slow - [eed2f6e](../../commit/eed2f6e7c2ebb36af7b31b3d1cc4ba5992a88ba0) 
- Set the start of songs to be constant, independent of playback rate - [63c5efe](../../commit/63c5efe778efc7c853c0636641e9e7d5c1570d2e)
- Try to allow VSync off using Aero in D3D - [d51205b](../../commit/d51205b174ced006aace4ac9a7d44affa0bfe872)
### Removed
- 1ms sleep in the Frame Limiter on Windows - [e490364](../../commit/e4903649377257957728e907f313705dc4f18858)
- Groove Radar Calculations (nobody uses them) - [fa53caf](../../commit/fa53cafb80ee8f450cad4baf6fbcc0d2156d71aa)
- Multithreaded D3D (for FPS) - [e0f4d7c](../../commit/e0f4d7c43c649f3f83d703f35779a0ff53553ba6)
### Fixed
- FPS dropped in gameplay due to large number of bpm changes throughout the file - [e3e3460](../../commit/e3e346075f6411b648eed7b8fdf940287333a855)
- FPS dropped when exiting gameplay due to unnecessary looping -  [a7ca8c7](../../commit/a7ca8c7a5ec955430cd3fa55f056ba408bffa10f)
- Rate System with Pitch was put back together. It was previously removed by the SM5 devs - [b5f7cc7](../../commit/b5f7cc7707a5735bece3498ea2aac822ec484699)
