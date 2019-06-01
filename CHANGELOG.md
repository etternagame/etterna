# Changelog
All releases of Etterna are listed in this file as well as links to files detailing all of the changes for each. All changes for each version apply in supplement to the ones below it. Changes are not in chronological order, only versions are.

## [Unreleased] - TBD - Codebase Restructure & Quality of Life Patch 2

Windows x64, Windows i386, and Mac installer future release.
### Added
- Asset Settings to pick Toasties, Judgments, and Avatars - [#538](../../pull/538) [3162b2e](../../commit/3162b2e4334538eeca413ed3419cd3d28bfb2b57) [615d748](../../commit/615d748605aa77eea93f87986055c800f9f27093) [35c7505](../../commit/35c7505da16d3e17fbcbcdd5dd0976334df47f21) [471b205](../../commit/471b20506c1b23bd9130f076c2556ceb4b42105a) [21062e7](../../commit/21062e74e0ec01f6e6e91a16d4a1352289feca41) [12515ef](../../commit/12515ef64edb48c03f4f2aa43d09a819bea24e29) [dbebb58](../../commit/dbebb58659d939980ae86fb6f0a71aa5044ae1c8)
- Button to enter sort mode - [a1dbd45](../../commit/a1dbd4515ecca6e2e0f3266a0bb03b3ceb945468)
- Combo Text Hiding option is now available in Player Options - [5bc8790](../../commit/5bc8790dad5c64fcfbf1b9c4a644bafded151358)
- Current Sort indicator text functions as a Random Song Button - [26fa5ef](../../commit/26fa5ef3e8aac33f52fcf8dfcaa404060d1a0ea7)
- Debug Menu has a new Misc Page with many new options - [339b91b](../../commit/339b91b3b30403e1ca35d9a192e23e636d90f6fc) [efa0767](../../commit/efa0767a3a0912854264bc98f8050efc0725ce20) [5738f17](../../commit/5738f17fdb04350bb708ca945e4306b186a65310)
- Hotkey to toggle Practice Mode in SelectMusic on `Ctrl+O` - [a47f233](../../commit/a47f233d9da291841c030d9abb3d32fa5e8fd8fe)
- Legacy Toasty & Default 2x6 Judgment - [a54a46e](../../commit/a54a46e9a1c682f66128289ab3284c17e487a2de)
- Lua access to Pack Download URL - [56c5d61](../../commit/56c5d61e8d41dc6205ec511d150112b3d1094f74)
- Minanyms - [03e0704](../../commit/03e0704f93575dbd40d7ebe868e5100e76c7a4f7)
- NowPlaying text file output for Streamers - [2015e2b](../../commit/2015e2bd39505ca1aa4a66285f03931c6020f83e) [22b8522](../../commit/22b8522c982f383c22bf14b274ef10e3b995cdb1) [f17f954](../../commit/f17f954c0be92b96a8d348168a2cec7ce3ae529f)
- Support for building with Ninja & VS 2019 - [#514](../../pull/514)
- Support for 2x6 Judgments (Early/Late Dots) - [73d0dcf](../../commit/73d0dcfd03bd4040b779697f7da7e54ffb47cbf9)
### Changed
- Avatar Select now leads directly to Asset Settings - [4986a69](../../commit/4986a69fd7394c2b9b2b8664d501cb28a7adc51b) [70dfee2](../../commit/70dfee26b3becb28030f090c5548c17dd54ddc15) [615d748](../../commit/615d748605aa77eea93f87986055c800f9f27093)
- Chart Leaderboard Entries have a mouse hover for extra decimal precision - [02da394](../../commit/02da394ea4bbe9754d50d260df057bc45d9e62dc)
- Chart Preview will no longer have a 0.05 second delay to regenerate when changing style - [211f1e2](../../commit/211f1e230f752baab77d4d5e70224461cb524ebe) [211f1e2](../../commit/211f1e230f752baab77d4d5e70224461cb524ebe)
- Deleting entries from the Favorites Playlist fails due to internal issues. Disable the ability - [20e94f0](../../commit/20e94f0ab3368a87c577e6e7826dae1863d39c10)
- Explanation Text for several Player Options have been updated - [a15c5e3](../../commit/a15c5e31ac18c4e14efb306f9424fabe42df6803)
- Help Screen (F12) has been updated - [30f5ac8](../../commit/30f5ac8f0dae2f8b4563737c846ab5bf286d2798)
- Hidden & Sudden should properly slowly fade Taps, not instantly - [e12144e](../../commit/e12144ea68f5c147a38ba1accf6c63851cbb068c)
- "Judgement" has been globally replaced with "Judgment" - [7c6d0af](../../commit/7c6d0af21822da28cbd06a4dad575eae2df808fa)
- Miscellaneous Options Buttons on SelectMusic now highlight - [ec51a8c](../../commit/ec51a8ce89e0fbb163fb8a334a215d2af0b55a06)
- Notedata Cache Output to File slightly restructured - [bd66ea5](../../commit/bd66ea59d916424563e885af3f35dd8d2ae925fd)
- NoGlow Metrics option is now a Preference, in the Appearance Options - [df47749](../../commit/df4774992db4c334cfe26f72a20968ba35f7b3dc)
- Pack Downloads can always be instantly left by pressing Escape - [2449295](../../commit/24492955bc87adee045be1e19477b86db926056c)
- Packs about to be downloaded will open a browser link if they are too large - [564e041](../../commit/564e041d20dd99eb10900395df8ca3a8fb77a6f9) [e828140](../../commit/e8281404528635cbe675515c3d8ce7ea0afe7da9)
- Practice Mode should stay ingame after the song ends - [8045bce](../../commit/8045bcee99fde317a1c504a0885eaf830c335a17)
- Rates can be set as low as 0.5x now - [9a03a39](../../commit/9a03a39deb22a8cd76061eb715c155faa9467127) [c07940c](../../commit/c07940ce85dc4f6e78e1c57b3d3f7ffbe98ff288)
- Redid the face of the repo - [#513](../../pull/513) [eba134d](../../commit/eba134d48af64881b1601501360e39a9f923e10c)
- Reorganize the source file structure to match the VS Project Layout - [#455](../../pull/455)
- Rewrote all old changelogs to be more comprehensive and have a somewhat standard format - [#522](../../pull/522)
- Rewrote json and websockets code to rapidjson for speed & to reduce "bloat" - [#547](../../pull/547)
- Rewrote the CMake system & Building Instructions - [#481](../../pull/481) [#468](../../pull/468) [#520](../../pull/520)
- RString has been replaced by std string in various XML, Gameplay, and Scorekeeping files - [#552](../../pull/552)
- Some submodules are replaced by subtrees - [#481](../../pull/481)
- Textbox in Pack Downloads is autofocused when entering the screen - [17ae528](../../commit/17ae528b5a43da20c13cdddebf021091ff2793c1)
- Toasties may linger on screen if they are too large when exiting. Just fade them out - [7123243](../../commit/71232430058fc225ca6fae5cbcf50813f3b60b6f)
- Use stb image to write images (Screenshots) - [#489](../../pull/489)
- Windows installer is now generated by CPack, which basically means we don't manually define everything for it - [#481](../../pull/481)
### Removed
- AttackMines - [8b42207](../../commit/8b42207b111be1f98962aafb271f783492d68b36)
- EZSockets - [b1ec80b](../../commit/b1ec80b0aef22036bef9b8399c484ba98a484074)
- Force Crashing from the Debug Menu - [5fa6d9c](../../commit/5fa6d9c670c1fe35bfeacc6db3e2265b338c0cbd)
- Libjpeg replaced by stbi_write_jpg - [#489](../../pull/489)
- Jsoncpp - [7b5c562](../../commit/7b5c562af4eaaea6a07cd87a679541badcfbb2a0) 
- Nlohmann Json - [2c5fbfe](../../commit/2c5fbfedd47cb3825976af63ddfe4fda223cb959) [79f3d33](../../commit/79f3d330614d011b141f300ca8d2f6715d0a717b)
- References to para, techno, and lights except for controller support - [#531](../../pull/531)
- Show_Skips for lag logger in the Metrics - [8a30ce8](../../commit/8a30ce87d4b4f53c445a7f10a59cb6daa67eba5e)
- Some references to Player 2, Routine, Couple, and Versus - [#531](../../pull/531)
- Unused Lights file "Parallel_lights_io.dll" - [5b83340](../../commit/5b833401ad048e8ec672eb9232bee2052b0078cf)
- Unused Noteskin Folders (kickbox, para, techno, lights) - [7e18897](../../commit/7e188972c442ea9c2b8925de17248da1db0e0c19)
- Unused Stepmania files - [08adaec](../../commit/08adaec016efa5895627d8594da60b8b052c5b22)
### Fixed
- Banners didn't show up for some autocomplete cases - [edf8a72](../../commit/edf8a7257c0955f503c3aa31c5b42e73ed948b4a)
- Changing Judges on Evaluation didn't update CB Counts per hand - [4a3ec19](../../commit/4a3ec190b8d14eafd97d6f905d3f68b93c332c51) 
- Chart Preview appeared again even after being turned off when changing style - [20bc0d4](../../commit/20bc0d4d9e634d1a2e9c8f44e9a525aaf2bf4a0a)
- Differential Reloading disconnected the user from Multiplayer if it took more than 10 seconds - [#516](../../pull/516)
- Game was hanging sometimes due to the Audio Visualizer (Deadlocks) - [#536](../../pull/536)
- Game was stuttering when plugging or unplugging devices on Windows ([Issue 537](../../issues/537)) - [#539](../../pull/539)
- Game crashed when decompressing zip files (Pack Downloads) larger than 2GB - [2deae2d](../../commit/2deae2db37c581cb4bc7f52aae66658a5a73dc0a)
- Game crashed when opening Player Options if not hovering a Song - [e42c304](../../commit/e42c3049a1f6a563304e3a12030767cc2c368811)
- Game crashed when Songs picked in Multiplayer if the picked Song is filtered out by some means - [17dcfbc](../../commit/17dcfbc4d0f54cb81bd7c26110a6939f31760c62) [f19b168](../../commit/f19b1686a09aead81a32856368c16127b2cf18f3)
- Mac always hanged when disconnecting from multi ([Issue 475](../../issues/475))
- Mac-specific cleanup functions for loading Songs broke some stuff - [1a31e74](../../commit/1a31e7441fe9e53476897b22e43f2bd5eb05e560)
- Multiplayer Chat disappeared when minimizing after scrolling up - [3efd130](../../commit/3efd13082a0ddde8c0f3626cb65099d9aa4b8422)
- Multiplayer Evaluation Scoreboard entries didn't always work ([Issue 462](../../issues/462)) - [#549](../../pull/549)
- Multiplayer Ready & Force buttons had weird Font issues for some builds - [45cd0a0](../../commit/45cd0a02672423e4a515dc956c54a6deac6f3e4a)
- MusicWheel Scrollbar was able to be clicked through the Til Death frame - [7bff5c3](../../commit/7bff5c3a0b713a20246314af2cf7a021c9de1162)
- Nonzero Visual Delay settings were affected by Song Rates ([Issue 485](../../issues/485)) - [#534](../../pull/534)
- Osu files with the Standard difficulty wouldn't load - [ed0c393](../../commit/ed0c39396a7e0d6885462238289ba69bedafa0ff)
- Practice Mode kept Judgment counts at all times, causing weird issues. - [4a5968d](../../commit/4a5968d8c50436c04af278eec29d30078d7343d7) [17ed183](../../commit/17ed1830974e91c2570c4d0ccee0d9614fa1f9f3)
- Practice Mode instantly judged every Tap since the beginning of the file every time you seek as a Miss - [72dbeb0](../../commit/72dbeb0e8730ea7dbe84c7d47f91203db132793c)
- Profile Stats (Play Time...) never updated unless finishing a Practice Mode play - [c1b8e06](../../commit/c1b8e06c80b8e5cc203ddfcf94ad8b44567bf8d4)
- Rare crash occurred when exiting Practice Mode due to Fail settings - [c16a0f5](../../commit/c16a0f5e63592ed56967a573b64b481eda20afc5) [6271e50](../../commit/6271e50c139369373a7bb8e56a3eccfa3fbb89aa)
- Replays used to have a slider that worked. It stopped working. - [14744f4](../../commit/14744f430e79f89e299b8cca6553460df8d35cdc)
- Widescreen caused the target tracker, progress bar, and judgments to be offset in Gameplay - [3b27bf6](../../commit/3b27bf6fca39997732767a28b287040d31faa9f3)
 
## [0.65.1] - 2019-03-10 - Quality of Life Patch

Windows x64, Windows i386, and Mac installer release. Linux tar available.
- Many bugfixes and performance improvements to mostly Practice Mode.
- [Notes](.changelog/Release_0-65-1.md)

## [0.65.0] - 2019-03-01 - Practice Mode

Windows x64 and Windows i386 installer release.
- Practice Mode, Audio Visualizer, and Common Pack Filtering in Multiplayer added. Many old Stepmania assets removed.
- [Notes](.changelog/Release_0-65-0.md)

## [0.64.0] - 2018-12-04 - Public Multiplayer

Windows x64, Windows i386, and Mac installer release.
- Multiplayer is now public and available from the client. 
- [Notes](.changelog/Release_0-64.md)

## [0.63.0] - 2018-11-23 - Performance Boost

Windows x64 and Windows i386 installer release. Mac zip included.
- Replay Watching is improved to use offset values for Taps when available. Various performance improvements.
- [Notes](.changelog/Release_0-63.md)

## [0.62.1] - 2018-11-14 - Hotfix

Windows x64, Windows i386, and Mac installer release.
- Bugfixes and Optimizations to Chart Preview & ScreenSelectMusic.
- [Notes](.changelog/Release_0-62-1.md)

## [0.62.0] - 2018-11-11 - Chart Preview

Windows x64 & i386 installer release only.
- Chart Preview added. CustomizeGameplay now has mouse support.
- [Notes](.changelog/Release_0-62-0.md)

## [0.61.1] - 2018-10-17 - 64bit D3D Crash Hotfix

Windows x64 installer release only.
- Release to fix crashing on 64 bit D3D.
- [Notes](.changelog/Release_0-61-1.md)

## [0.61.0] - 2018-10-12 - Replay Watching, Multithread Song Load, and 64bit

Windows x64, Windows i386, and Mac installer release.
- 64 bit version added. Replay Watching & Evaluation Viewing from Replay Data added. Osu Noteloader & Pack Downloader mirrors added.
- [Notes](.changelog/Release_0-61-0.md)

## [0.60.0] - 2018-08-18 - Online Integration & LuaJIT

Windows and Mac installer release. Multiplayer is functional but private, so the option is hidden in this release.
- Integration with EtternaOnline, Pack Downloader, and Song Tags added.
- [Notes](.changelog/Release_0-60.md)

## [0.57.1] - 2017-11-15 - Last Big MSD Update & Bug Fixes

Windows installer release. Due to merging, some features of this version and the previous one may be confused.
- Pasting from the clipboard added. MSD received its last update: Chordjacks replaced JackStamina.
- [Notes](.changelog/Release_0-57-1.md)

## [0.57.0] - 2017-11-11 - Discord Rich Presence

Windows installer release. This version was released as 2 separate installers, which are effectively the same.
- Discord Rich Presence added.
- [Notes](.changelog/Release_0-57-0.md)

## [0.56.2] - 2017-11-10 - Client Bugfixes & Previous Calculator Updates

Windows installer release.
- Custom Judge Windows & Rescoring added to Til Death. Boo Window is capped at 180ms minimum.
- [Notes](.changelog/Release_0-56-2.md)

## [0.56.2] - 2017-11-05 - Pre-release: Calculator Updates required for EtternaOnline

Windows, Mac, and Linux pre-release.
- MSD Calculator scaling capped at 97% now. Only Top Scores count in Player Rating, and only Chord Cohesion Off counts.
- [Notes](.changelog/Release_0-56-2-pre1.md)

## [0.56.1] - 2017-10-30 - Pre-release: Calculator Updates

Windows only installer pre-release.
- MSD Calculator improved by rating jacks more fairly.
- [Notes](.changelog/Release_0-56-1.md)

## [0.56.0] - 2017-10-17 - Pre-release: Calculator Updates

Windows, and Mac pre-release.
- MSD Calculator improved by distinguishing skillsets better.
- [Notes](.changelog/Release_0-56-0.md)

## [0.55.3] - 2017-10-12 - Hotfix 3: MP3 Sync Fix

Windows, Mac, and Linux release.
- More bugfixes to attempt to fix desync issues on MP3s.
- [Notes](.changelog/Release_0-55-3.md)

## [0.55.2] - 2017-10-03 - Hotfix 2: Weird Wifepercent Fix

Windows, Mac, and Linux release.
- More bugfixes and bandaids to previous hacks.
- [Notes](.changelog/Release_0-55-2.md)

## [0.55.1] - 2017-09-18 - Hotfix: Things We Didn't Notice

Windows, Mac, and Linux release.
- Support for rates up to 3.0x and various bug fixes.
- [Notes](.changelog/Release_0-55-1.md)

## [0.55.0] - 2017-09-08 - EtternaOnline, Chord Cohesion Off, and More

Windows, Mac, and Linux release.

This version coincided with the launch of [EtternaOnline](https://etternaonline.com/).
- Chord Cohesion Off is official. Goals & Playlists added. Gameplay is now fully customizeable. Etterna.xml format introduced.
- [Notes](.changelog/Release_0-55-0.md)

## [0.54.2] - 2017-03-19 - Unreleased Iteration

Source only unofficial release.
- Early work on CustomizeGameplay, Multiplayer, and Chord Cohesion off.
- [Notes](.changelog/Release_0-54-2.md)

## [0.54.1] - 2017-01-30 - Release Candidate

Windows installer pre-release.
- Release to fix a bug in Player Rating calculation.
- [Notes](.changelog/Release_0-54-1.md)

## [0.54.1] - 2017-01-29 - Pre-release: DP to Wife Conversion

Windows installer pre-release.
- DP to Wife conversion is enabled for each load. Mouse support on the MusicWheel is added. RestartGameplay button added.
- [Notes](.changelog/Release_0-54-1-pre1.md)

## [0.54.0] - 2017-01-24 - Official v0.54 Release

Windows installer release.
- Judge Conversion on the Evaluation Screen introduced.
- [Notes](.changelog/Release_0-54-0.md)

## [0.54.0] - 2017-01-21 - Pre-release 2: Replay Data Saving

Windows installer pre-release.
- Filter Tab for filtering Songs by Skillset & Replay Data writing introduced.
- [Notes](.changelog/Release_0-54-0-pre2.md)

## [0.54.0] - 2017-01-12 - Pre-release: More Specific Skillsets

Windows installer pre-release.
- Appveyor & Travis builds introduced. DivideByZero Noteskin variants added. Skillsets are split into more specific categories.
- [Notes](.changelog/Release_0-54-0-pre1.md)

## [0.53.1] - 2016-12-25 - Hotfix

Windows installer release.
- Update to fix Evaluation Screen crashing.
- [Notes](.changelog/Release_0-53-1.md)

## [0.53.0] - 2016-12-24 - Skillset Ratings

Windows installer release.
- DP to Wife conversion introduced (not enabled). Skillset Specific Ratings introduced (different from Score Specific Ratings).
- [Notes](.changelog/Release_0-53-0.md)

## [0.52.0] - 2016-12-14 - Song Favoriting

Windows installer release.
- Song Favoriting introduced in Etterna. Target Tracking introduced in Til Death. Internal judge rescoring algorithms introduced.
- [Notes](.changelog/Release_0-52.md)

## [0.51.0] - 2016-12-07 - Song Search

Windows installer release.
- Song Search introduced in Til Death. Moving gradually to Wife-based Grades.
- [Notes](.changelog/Release_0-51.md)

## [0.50.0] - 2016-12-04 - Public Beta Release - MSD & Wife

Windows installer release meant to be installed in its own clean directory.
- Mina Standardized Difficulty & Millisecond Scoring integrated into Etterna. Til Death Theme & Etterna Song Pack ships with the new installer.
- [Notes](.changelog/Release_0-50.md)

## [0.03.0] - 2016-09-09 - More FPS Improvements

Binary only release to replace Stepmania.exe.
- Frame Limiter, PitchRates Preferences and optimizations
- [Notes](.changelog/Release_0-03.md)

## [0.02.0] - 2016-08-31 - Early Experimental FPS Improvements

Binary only release to replace Stepmania.exe.
- Large FPS Improvements & optimizations
- [Notes](.changelog/Release_0-02.md)

## [0.01.0] - 2016-08-26 - Early Experimental Release

Binary only release to replace Stepmania.exe. Initial branch off of [Stepmania 5.0.12](../../tree/v5.0.12)
- Small experimental FPS improvements & bug fixes
- [Notes](.changelog/Release_0-01.md)
