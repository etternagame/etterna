# Changelog
All releases of Etterna are listed in this file as well as all of the major changes for each. All changes for each version apply in supplement to the ones below it. Changes are not in chronological order, only versions are.

## [0.57.1] - 2017-11-15 - Last Big MSD Update & Bug Fixes

Windows installer release. Due to merging, some features of this version and the previous one may be confused.

### Added
- Allow ScoreManager to purge scores - [8da6004](../../commit/8da60041ea588dcda204bf65877c9c2d47a0242a)
- Allow pasting from the clipboard in various places - [f0c1bc0](../../commit/f0c1bc0474bd035a25dfa753b8d8d444d9651725)
- Add LibCURL properly for ingame downloads - [62c0e71](../../commit/62c0e718519e423790408d87faf8d57e4bf94b73)
- Hilarious splash screen - [19ea4af](../../commit/19ea4af11c54b0498947b568e95132c1a435a35d)
### Changed
- JackStamina shouldn't be ignored in Player Overall Calculation - [2201b59](../../commit/2201b5928f6f2b81288c9f42200efbb72deef6f4)
- Drop the lowest skillset when calculating Player Overall - [063f5e0](../../commit/063f5e0224fa4d97bb8d9027ccda6230fe3a6ce8)
- JackStamina has been replaced with Chordjacks - [3065bb5](../../commit/3065bb579eaa189f3b403d80734dfb94eb59711c) [d29f7a9](../../commit/d29f7a93563735cad9cbb363a5ceb7cfb3e8b840) [b80925b](../../commit/b80925b06070eb894abbcf4131fe59b8c6b39148)
- Discord connection properly closes when closing the game - [31e0210](../../commit/31e0210af83f913d841304219032d2d9ec4c70cf)
- Replays after Chord Cohesion changes go into a different directory - [02d839f](../../commit/02d839f1acbac4f15bdd3cc21786eacde47b88ad)
- Don't let Lua mess with internal rescoring functions - [2fc75f9](../../commit/2fc75f90f990e66e162ae6d6c772ca2c4d913866)
- Multiplayer improvements to input redirection & room searching - [0b00b5a](../../commit/0b00b5a64bd8813a895182830708a1bba24bcdd7) [505ab41](../../commit/505ab4121293d0266886178b2ad36f286096c63e)
- Don't care about input debouce for the mousewheel - [c5a97e3](../../commit/c5a97e3d2c209c0bbd12fd52e302992af669031d)
- Initialize MessageManager earlier in the creation process due to undefined behavior - [dc484e6](../../commit/dc484e68bc792821b00c913a0a427e63522f21c6)
### Removed
- Levels - [0b00b5a](../../commit/0b00b5a64bd8813a895182830708a1bba24bcdd7)
- Unused theme file - [55935e7](../../commit/55935e7c057290c4f50f857758e07a771ca8c5b6)
### Fixed
- Up doesn't reset Judge conversions in Evaluation - [d791e9f](../../commit/d791e9fbc98738123a9ca9e21421f95804271dc2)
- The 180ms window cap doesn't work on Replay-Loaded plots - [903375c](../../commit/903375c33e3bf714de01c730515551adebf381d9) [2349890](../../commit/234989037e2b81bd168f6009d4196612bc6228b3)
- Loading ReplayData fails - [88f2262](../../commit/88f2262b7bcd33069618bdf9d46091b67540d890)
- RoomWheel is broken - [6b33880](../../commit/6b33880c28cfdbc820cd646964e03ab6e4347cfc)
- Somewhere along the line Profile Name & Rating disappeared from the frame - [26db3d4](../../commit/26db3d41472b2328d76b0272aa7dc84d5013d6c0)


## [0.57.0] - 2017-11-11 - Discord Rich Presence

Windows installer release. This version was released as 2 separate installers, which are effectively the same.

### Added
- Discord Rich Presence is added to most screens - [b141923](../../commit/b141923975d882edeb900dd9e4071733621ac1f8)
- Lua has access to OS for os.time - [d2686d2](../../commit/d2686d28b486c7c4f3eaa1d98e2cc287651fea32)
- Ingame Pack Downloading through curl - [dfde109](../../commit/dfde1092912ce8e11032f537b8e32c00d66b59b9) [259b574](../../commit/259b574f1199ccdff5f388c77da0926107a895ef)


## [0.56.2] - 2017-11-10 - Client Bugfixes & Previous Calculator Updates

Windows installer release. 

### Added
- Custom Judge Windows & Custom Evaluation Rescoring - [199cd64](../../commit/199cd649a403038d1f8f339c2ca579bb0b643391)
### Changed
- Boo window is now capped to 180 milliseconds for all Judges - [1864e48](../../commit/1864e4806d60b28ada654111743978b0b972d7a4) [8e08eb5](../../commit/8e08eb587030ff84d38a955743110bb6c343f23c)
- Use WifeGrade instead of Grade in many displays - [0a4bfbd](../../commit/0a4bfbd3b22f0ceee7ccbd79537fa2bcbeabd1fe)
- Score tab shows highest SSR for scores - [9c3611a](../../commit/9c3611a205a3b1be7629b59dff2ad8514f4cb421)
### Fixed
- Grades don't show on the MusicWheel until it moves - [c503302](../../commit/c503302dc64554650dc8f837b19dbb68a170a77e)

## [0.56.2] - 2017-11-05 - Pre-release: Calculator Updates required for EtternaOnline

Windows, Mac, and Linux pre-release. The MSD Calculator is receiving heavy chiseling.

### Added
- TopScore flag introduced for HighScores for identifying Top Scores properly for EtternaOnline parsing - [d518aed](../../commit/d518aed9f83ae3a59710d1cb9605feb18a1a9171) [5df2fc0](../../commit/5df2fc054545ec13a16a78fd25c2349cfd543e78)
### Changed
- The Calculator has been updated - [dd1ed41](../../commit/dd1ed41ebda7617e2f354c2d45c8b1310853645b) [7e0bcc7](../../commit/7e0bcc772ce9290247ca47fd9d58bf48a408d2da) [0848477](../../commit/08484777b7277805959f9d58ce210ff638dfd3d3)
  - The SSR cap is now 97% instead of 96%.
  - The distribution of all Stamina files is a bit more even.
  - Stamina associated with Stream or JS/HS is better rated.
- Player Rating Calculation ignores CC On scores, is a flat average of skillsets, and had the internal algorithm slightly changed - [c7cc786](../../commit/c7cc7865c0bdcad3ba82bff66840ca84f0c984d9) [bb6dfc5](../../commit/bb6dfc5cc1d8a48814c083862c649a4f6798f521)
- Calculating the Player Rating also sets Top Scores - [ff9bee9](../../commit/ff9bee9518cfae946407fff97fe756e3f84d522f)
- TopScores should not be read from the XML, and always calculated - [f2216ac](../../commit/f2216ac716d2911432c59df0253515df52a21759)
- Only TopScores are considered in Player Rating Calculation - [3cd733a](../../commit/3cd733aa59351d0da323a24e9312f2c99055c313)


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
- After syncing a file, it is reloaded from disk - [8eec96f](../../commit/8eec96f5bc107c11aa57ff81c88989fb266bafc8)
### Fixed
- MP3 files are off sync due to incorrect math when calculating the next source frame - [52fbe37](../../commit/52fbe3752d493902e8d9ef0e35676a559a3c337c)


## [0.55.2] - 2017-10-03 - Hotfix 2: Weird Wifepercent Fix

Windows, Mac, and Linux release.

### Changed
- Use NormPercents from the SSRs to set PB keys instead of Wifescore - [c692da0](../../commit/c692da02d52f96aa531fd3be53e2b5a0ee964af2)
### Fixed
- Some scores show up as 11111.00% - [f7a4311](../../commit/f7a43118efc2537021a46246208a2de962637650) [e1ed93e](../../commit/e1ed93eac91dfee0638d985a74dcb81135b9b703) [4c7e2c8](../../commit/4c7e2c871e55a2f5bfc377212e6e69368fb93a28)
- Some CC Off PB Keys are picked or set wrongly - [4ff3fb3](../../commit/4ff3fb3fad1725c29512f3d6328e2fdf69dea743)
- Some files cause a crash due to an invalid Difficulty Enum - [82ca9a0](../../commit/82ca9a07461187be92d7320e52bbe84e69f999a0)


## [0.55.1] - 2017-09-18 - Hotfix: Things We Didn't Notice

Windows, Mac, and Linux release.

### Added
- Rates up to 3x are available in Player Options - [0ff5102](../../commit/0ff510201bd2f20a3fff2f17d91790ba1734f73d)
### Changed
- Set the video troubleshooting URL to a real informational Stepmania video instead of Google.com - [acd1ceb](../../commit/acd1ceb443dbc4cbe202a0109e345179a11d89d1)
- Don't disable Gameplay Sync controls when Autoplay is on - [8b9a92a](../../commit/8b9a92a2c8c8aaadfe3e5440954d24bb9935925a)
- Fallback to basic old MusicWheel sorting for broken sorting methods - [1b78562](../../commit/1b7856217680e9257d24ed72ae2374f4caf95aae)
- Doubly verify a Score is obtained using Chord Cohesion off by counting the hit Taps - [2208f4e](../../commit/2208f4e5b1155e935c0cdce47853e856de1ac5d3)
- Keep track of PBs, specifically PBs which are Chord Cohesion Off - [49c669f](../../commit/49c669f3853ff6daf487bbee82ec132b2783b56c) [c7addfc](../../commit/c7addfc4e62ba2b4897eccbf812db4e1cf39ec12)
- NoteData must exist when writing SSC files for sync changes - [286645e](../../commit/286645e7ebb3dbaf739a62e7f928eaa7d9d01bf3)
### Deprecated
- Deprecating GAME:CountNotesSeparately() in Lua in favor of GAMESTATE:CountNotesSeparately() - [7585ee8](../../commit/7585ee82ef48c8cd0b115f388bae442eea21a90a)
### Fixed
- Mines count in the NotesPerSecond counter - [42207be](../../commit/42207bec8878168092a1caaf3e54d942a5eb9f4b)
- Crash loading a playlist with no charts - [4279a10](../../commit/4279a1077e82497af62cefb2307626831361f2bd)
- F9-F12 don' work for syncing due to a logic error - [263b797](../../commit/263b797e9fd09548a35095b81227508fbab1aab7)
- Changing offset messes with MSD calculations; fix it by normalizing the first row to time 0 - [8d0454f](../../commit/8d0454f6bb7254cff0acd27e48a367450874fe70)


## [0.55.0] - 2017-09-08 - EtternaOnline, Chord Cohesion Off, and More

Windows, Mac, and Linux release.

This version coincided with the launch of [EtternaOnline](https://etternaonline.com/).

### Added
- Better Chart Goals - [ee3c6ec](../../commit/ee3c6ecb8de05202fc8e3aa7eae843e53406da1f) [aae1e3c](../../commit/aae1e3cea4efdc486aff7634c07e8d2eb6acd8df) [67b18f2](../../commit/67b18f278437f332fb3f58154b6d3300d1e769df) [cff3868](../../commit/cff386855d6e27374d4ecc936366ab4ac4e38edf) [c62e0cb](../../commit/c62e0cbf26a65b0f71099a61ea51377a4f8bf881) [206ee3e](../../commit/206ee3e8a0f1ef7359721636072237664c48aa26) [e918b68](../../commit/e918b68736c6f1ec11f291faa23339cbfd36fbf1) [360050b](../../commit/360050bc99442e0f4042fecbdd14f83fbad9700a) [ca7ec56](../../commit/ca7ec562a157788c920db7915d5696ebdf899d4b) [510d7be](../../commit/510d7bea827d8e3dac6962015edd49578257ab90) [4f10ee9](../../commit/4f10ee906c53da223f8477a320f51547c4423956) [ae60130](../../commit/ae601308684e1b0f466038d0d7c057b92294c63b) [dd04262](../../commit/dd0426272335ec63bdc43922e057c10964767e66) [9bcc516](../../commit/9bcc516610512381ec9b313fe966c198edf26026) [09c2773](../../commit/09c2773076637702286297103caa2a6aa9202a49) [0290698](../../commit/02906983c4f3d26565cff7673118dbd4d36028bf) [e0f53c5](../../commit/e0f53c5e4db89eb5f4372721ae40f2c19c1750f6) [176b6f7](../../commit/176b6f7ffb90e733699906af55b5987befd60652) [74c2051](../../commit/74c20513dd4c2fc4b4219f8f87b97a3f73a1221d) [44ec91e](../../commit/44ec91e942f1527aed3f64818599d4cc257f2336) [5c5d834](../../commit/5c5d834d67ed2712ef47e0ecc353f580c547d828) [0e48965](../../commit/0e489657699c27b7f0e785631250a6cf5a307146)
- Chart Goal Interface - [574ffa1](../../commit/574ffa1874e2b5272bb0d6a607c26f5dc5ea366b) [4036134](../../commit/4036134ba17315469f8ff673699392f3c963e55c) [2bd7476](../../commit/2bd74768ea5bbf3a3a16ea142ff8f837c63a9e8d) [5cbde03](../../commit/5cbde033b835d11af65ddb5bb7f93725a06cd98d) [708a829](../../commit/708a829309ecf0a90b0494a2259c3fe39ae159dd) [de4e192](../../commit/de4e192a3dbeb9066e385c462e116172138736c0) [f25a925](../../commit/f25a9258f63e420295a3c25a1551434e8cd45a5d) [c317251](../../commit/c31725122d08c5f435278665065e472cb22bf37b) [c86bd88](../../commit/c86bd888bc516daea9424402aebd242276cc1c9c) [05738db](../../commit/05738dbe445c1755755fb5b6158d07272f2e3e59) [cfc9719](../../commit/cfc9719e7bd47a8578f7df1870a5cf31a1e20b37)
- Lua methods to get Songs & Charts by Chartkey - [d0e0c2f](../../commit/d0e0c2fce2c1e5b7f8b7a0afc5b73e2411f4777d) [789181f](../../commit/789181f81aa3ff2b02235650728b228832b6ad71)
- 5 new Etterna pack songs by Alexmandi, Elekton, IcyWorld & Cataclysm - [65a0174](../../commit/65a0174634799ba2b3e19c3e09c6eb4b5628e228) [5c85e12](../../commit/5c85e12a8efddace8fc85975e7e8f29ef9498042) [f29d0b9](../../commit/f29d0b97f689e2643bca3274b6f0e4033e4a3a39)
- Recording Wifescore over time - [804aa0c](../../commit/804aa0cd6d5decc58e33ab9ce43413ef515a6d23) [859116f](../../commit/859116fcccc86a79528840b3767f0c5575303c7a) [bb065fc](../../commit/bb065fc343a66ecbbc150b7f0c07d5e6bad76b26)
- New Etterna.xml save structure, like Stats.xml but without ReplayData - [82a0b9c](../../commit/82a0b9c13571339bd4e586d23f58ff786e9f3329) [b5ae71c](../../commit/b5ae71c960eb266f85935a21d0b9cdce9ce7ea93) [4f3ed7d](../../commit/4f3ed7da600e463cbbb67f5f02aff70db770d4af) [2146655](../../commit/21466551533e53188769a3103781401a1f987875) [868f31a](../../commit/868f31ac8dc99a12a6f58b8e62c3380338a4fef0) [e7602e0](../../commit/e7602e023976d839675ff9c9664e01c5a8b2fbb4) [4502a04](../../commit/4502a043a3983e0ba7a82cd2ae4fe0fff8753d54) [4073f0a](../../commit/4073f0add736e0ed7121d2c653d50193af309b3e) [f455c7e](../../commit/f455c7e8fc60997bbee0fe0a18e73f0af79fc72d) [f893394](../../commit/f893394709aa7eab319845dac8b9bc4823a589af) [e94420c](../../commit/e94420c794b0f8964a0d0812b847df22cc21d94b) [6a47206](../../commit/6a47206b3d9f1db58ca91a2de492cae381ad9b17) [b318a42](../../commit/b318a4282ddd6deb32c938775206a123b719a281) [ac71458](../../commit/ac71458d53da1f0c697aea3b151454db415743fb) [074812d](../../commit/074812d151ab30508c1dbd9f47fb02f380b871d1) [277e3be](../../commit/277e3be48960be8a25d3c989a0e9078022da3bef) [f06bffb](../../commit/f06bffb6cff2b082710a72a0b46d904199d7819d) [3856357](../../commit/385635795f2acb314e46e1fa441cb6a08fa42294) [3d0a90e](../../commit/3d0a90e2b7a23d5021713add8318e46ee16c3181)
- Stats.xml to Etterna.xml converter (and SM3.95 support) - [7647cce](../../commit/7647cce1fdd208589a5b35e553ac5f3b9b033f9d) [86a50c6](../../commit/86a50c681da31278bac12c17cfd142648a9be58c) [d013f3f](../../commit/d013f3ffc966fc723bcebf0ab44221e9f8203dfd) [e8ec832](../../commit/e8ec832f551ab08483fd42931bb74ee61c82960a) [4b77582](../../commit/4b775828fec5f41499d084b8f3a508aa806b34a4) [65ca243](../../commit/65ca24391d8b9b6f1bff77a3f8ca93fb4230d020) [ee613c8](../../commit/ee613c85ceb25bb163345e3d2821e890c4fe3af9) [41fc83f](../../commit/41fc83f9a093177dc7edd9800a705933d1f83277) [3021745](../../commit/3021745dc0916472bfb38cbac07b60c011f9751e) [8a8e7d4](../../commit/8a8e7d47341dd0ab44f43413552193b4dc31abfa) [1e3fa56](../../commit/1e3fa56a4d82a83aebbb213927d4c75d31ba70ae) [ad58aa3](../../commit/ad58aa3f8f8232e27daa4138f72ba7c1d0f4ac2b) [769eee3](../../commit/769eee3763ed93226d7e5b652c8a90ea8872115c) [25540d0](../../commit/25540d05d097d48aeed824f7ba445082231f77b1) [5ecb98a](../../commit/5ecb98a516dec689beee97fbe23047a14d40db95) [8f4fbc6](../../commit/8f4fbc66f116a1c6dc7ed90eb4e055fb8f5fd8a8) [6cbd15e](../../commit/6cbd15ec0f6eadb9018e143272a26e0167f285da) [22b7fc9](../../commit/22b7fc991c16ffac47761d66f0cc9d629f1171a5) [cbddb1e](../../commit/cbddb1e0272463bd0629662e233c29ab1d55bfc8) [879cf79](../../commit/879cf79f212bc53cba4c3aad2b24fc0775990cc4)
- Song Search tries to return Packs if it has no results - [363c36b](../../commit/363c36be89dc9997b01fbedf9c2e58728f8351ee) [f4b6e4e](../../commit/f4b6e4e4c7568202b9b198a8f46254621889c6cc)
- Playlists - [6f0e2a1](../../commit/6f0e2a182980b1231a282dcfd800a1921c13a453) [2f94614](../../commit/2f946149335396e507d49e97e5349bca1bba626c) [aa727d7](../../commit/aa727d7f767882d2c5b16a8ede5a3c6400764884) [809698d](../../commit/809698d98d8e39c155d42b31538817cdeac40d3f) [1909f38](../../commit/1909f38d73bd5ba7d66411856a04658e548ae366) [6bffed9](../../commit/6bffed9284b50823fcd26b985865a92ce186c9ef) [aced494](../../commit/aced49498e02f7772821bb75fce95d43bab12482) [dc3c054](../../commit/dc3c054465a6272a5eaa755e93cf0c720abb428d) [610c38c](../../commit/610c38c0e919305dd19b27500e0ee15ee132cbc8) [f5021b3](../../commit/f5021b3ae25075d0332bd16f1ffe1eeb96a9be4c) [c6d8e34](../../commit/c6d8e34a98eccaf79053f5f41488e48a1882daed) [f051a5f](../../commit/f051a5f99acc6f4956610d2f9fc86ba1f191dca4) [8bf3b3d](../../commit/8bf3b3dbbe2cb641be4c32ada2a98f63ede24b47) [f9c3284](../../commit/f9c32845384d2e0b627a2cbeb781d5a52eacab48) [8f8f0af](../../commit/8f8f0af37a64db956a6581c56458706cdd43184a) [05898fc](../../commit/05898fc87d77f3d9df8ce17dc9f2fbd35836acb3) [5f4ab03](../../commit/5f4ab03941df32b66bb243134708cf93076341c6) [39580f8](../../commit/39580f80851b6520b6e0de8286619ec8bf3b3da5) [f79c1de](../../commit/f79c1dec45d81df5bcd48994a7e387e39ca7cb7f) [d0a4175](../../commit/d0a4175f2d7343e546d7cf7d4f0d4b91df19760d) [30ab098](../../commit/30ab098d4fff0b48443fa3bc82d5b1585fa4da17) [599e9f5](../../commit/599e9f549c37f0dbed76e04344601d0fb5c0ffee) [bb64dbd](../../commit/bb64dbd8fdbc90fabc6a39f9887f6c2920faeca1) [612d5ff](../../commit/612d5ff01bb9911949f991abd016add49ec44697) [16b4810](../../commit/16b48100e3433674861dff993898999a3f79322a) [8fb457b](../../commit/8fb457b794503806cbb2831d6fc5ac38ae6f485a) [b4138dc](../../commit/b4138dcad71f432616c7d098be7f76e567418319) [5cc615c](../../commit/5cc615c8b7cd66c8ad67398ba987df74a79e718e) [781a57d](../../commit/781a57dc4e32816b0866dd675c60efa66ea2f2f7) [e950df4](../../commit/e950df43b7165384efb516230b039e783dd84071) [3fd8f48](../../commit/3fd8f489d732cc05778155368603d085f2f6509d) [4770443](../../commit/47704431e1357f54e1982ad75a06a26b03d3206f) [bdfd39d](../../commit/bdfd39db5b840913040f296b7936cefc4e6fc7a3) [c96bcd5](../../commit/c96bcd5337113559d55f0195e8c9b22827134955) [32e07dd](../../commit/32e07dd07c338d5f3fda5ef9877222080d3131df) [9005ee5](../../commit/9005ee583c50a22098a048e5b5d76a43151333d9) [3718be2](../../commit/3718be29fc23bc3e88d917bd357dd7901b375908) [a1ac458](../../commit/a1ac4581f1d3822922397eaf5de3d6e90584c791) [c5463c9](../../commit/c5463c98994a2d00695b350a425ff92b3451e731) [b66b4fa](../../commit/b66b4fad4241b3b3fc0b8a0abd49de6560a3ac0a) [9f3c791](../../commit/9f3c7918ef2756b95ff9b3592fd8c7323c94d22d)
- Differential Song Reload - [943935a](../../commit/943935ad51f55bc3102c0f3a2eaff1dd408c5baf)
- Per-chart mod Permamirror - [fc8692e](../../commit/fc8692e6b7745e9a48e2b61cd0dcd622611c9bdc) [c0c1333](../../commit/c0c13339f6e9bf9658eca941d53f347a44aa41b4) [109fcf5](../../commit/109fcf5900bc4d315d01cf3ffdcd183d42a84d2b) [87706ab](../../commit/87706ab808fa90038b0354667ccf1b16cba3abdb)
- Save profile hotkey - [c82080d](../../commit/c82080db97b05ca8393fde2f704c4e02953db029)
- Preference to hide the lag window with the FPS counter - [befe017](../../commit/befe017fc879f8c816d34c0b4703bc089719989a)
- Full CustomizeGameplay Implementation - [8791e91](../../commit/8791e914546ba9e3c1c38abd138bbedd3c1eaef6) [81a0b5d](../../commit/81a0b5dc9d41d1b111eb4e0308c086660cc20c77)
- Validation keys for MSD Tampering - [fb39253](../../commit/fb39253e5bf9cacfe5fb496a203d717a171e06e3) [0910fea](../../commit/0910fea8b26d05b8e1dafd392d0c796434cc156f)
- Other Validation keys - [22f2f72](../../commit/22f2f7225c247390338253e2b42cbe6148c87fa7)
- Offset Plot viewer to view Replay Data - [920e236](../../commit/920e236e3b87648f3557aa6d943bc66f0335f283)
- Minanyms - [85a1e49](../../commit/85a1e49cc7f6aa7c65c8d6580a929eb4d3b76b2d) [f2a9fc5](../../commit/f2a9fc583775dfb7afa18d52fec4c33394a3a759) [0037787](../../commit/0037787bb10bd1b68e4a016f3a8ab84d995c1184) [19eeeb4](../../commit/19eeeb4bb158724a8fa2b89d7e14b8c126690b08) [1d9de24](../../commit/1d9de249d7a98e6e7ba5d737c4fe100f4cb835ed) [dd59276](../../commit/dd59276cc58861046fd7b594f28453bc7d00370b) [41e3550](../../commit/41e3550b50961bc83f7c1ac292244742693002f5) [2b61bc3](../../commit/2b61bc3d9673b2ea212eb245a4b792f0cfe25db1) [e8c4731](../../commit/e8c47313ed7e2b0b3bf2b191235616b5beb18e2d)
- Player Rating appears on ScreenProfileLoad - [d0847ce](../../commit/d0847ced4360a6dbd46eb57d3ae8fef545b5361d)
- Chord Cohesion is disabled by default - [4bd00d1](../../commit/4bd00d123a26347a8d958d24e07c8ecc16c0b84c)
- Chord Cohesion enabled scores are invalid - [d50db3e](../../commit/d50db3e041a601e396a08c2beb539051928eb916)
- Playlists show up as Groups on the MusicWheel - [eab1dfa](../../commit/eab1dfabc16cccafb6e27abddc2ba341141440bf)
- Reconciling broken Chartkeys is possible - [4020933](../../commit/40209337eaaefc47604eb2dbe689cda4a3d97d50) [18657d3](../../commit/18657d3b4bbb015dd857f5b3b14648ed184041e5)
- Experimental NoteData cache - [735348d](../../commit/735348d430a6229879088bacf8d694f5644905a8) [e76366b](../../commit/e76366bdba5dd273d1c706f1b67d8fe46a9c0dde) [8cb704c](../../commit/8cb704c82b94cac9e0976340ccbbd14611d61236)
- Experimental Rating Over Time implementation - [dae82c0](../../commit/dae82c0e8d79c5a81ea58a3992dffbfdae29b912)
- Travis runs Coverity - [0a607f7](../../commit/0a607f73635e012bfcf9addeca475af1f6bdaaff)
- Window size getters are placed in ArchHooks - [455b70d](../../commit/455b70d44210568ce9204dd4b2383ea832ab2764)
- Replace MAD with FFmpeg - [3958470](../../commit/3958470636aad59d4746a15003aefdad554f8360) [21c3395](../../commit/21c33956f89781789eac6dff1c5058a735222328)
### Changed
- Optimized size for many JPG & PNG files - [db1d035](../../commit/db1d035084ba4ffcea211b32c4b1c1f311421a38) [80ba803](../../commit/80ba8038fc30fac14d2ad27f3a1fc53a57427f01)
- Hold Shift to save a compressed Screenshot rather than the other way around - [1ecde1c](../../commit/1ecde1cf13854b5e71bba4bc29e237d773c2e523)
- Instant restart button will not work online - [03a2a83](../../commit/03a2a838d5f55975bee6babfec24ef86ea52b4ca)
- FFmpeg updated to 3.3.3 - [ec92941](../../commit/ec92941d5cde2f8c8c474763cd3c57d54502b27d)
- FFmpeg GPU acceleration disabled on Linux - [20e0301](../../commit/20e03016af76dbc8f8ca4c1ec5d631da0f6e4713)
- Checking for a changed input device happens once per second, not once per frame - [01b930f](../../commit/01b930f84c1b3d3d6468b85e8644d46393a37125)
- NetworkSyncManager doesn't need to look for SMO packets every frame - [e97ab8b](../../commit/e97ab8bed47d185601da2e7f9122627e900969bd)
- Don't allow the Sync overlay help page to repeatedly spam Commands for showing - [c46dd91](../../commit/c46dd9156d9fcddb7f5fee66275c4f3a8ad3c6df)
- Optimize some rendering code for a 20-60% performance boost - [c4f7ccc](../../commit/c4f7cccaaf09e058ae142f46da6a02a95ac819b4) [c217987](../../commit/c217987ca520123aca586e114293d3b6f5ae5b16) [4a566af](../../commit/4a566af49c86c4a240814604a6ca6d86c360f610) [3951fd3](../../commit/3951fd369a5e5add5e8c9a984b4d588ade07ef31) [51bcae3](../../commit/51bcae372ecedfc698b651a8dd2667b781683546) [1c612f8](../../commit/1c612f8982aee6749514ea891f0970732b126c17)
- The Edit option on the Main Menu is now a link to ArrowVortex - [5034aa4](../../commit/5034aa4f1dbff52a4564b79a2988abd4caa39011)
- Redo the LoadingWindow internally for smoothness and not so much reliance from multiple sections of code - [09fb85f](../../commit/09fb85f3c74e44a9486da49a28929fec416363f4) [084da49](../../commit/084da499a16237af7f91f9a95546845d56c1057e) [1b468fe](../../commit/1b468febfb4d144be432588619a0e90d52010d5a)
- Installer refers to Etterna as Etterna and not Stepmania - [3ec7644](../../commit/3ec7644883a096837f950c06300e773835e279db)
- More MSD Calc updates - [e7507f0](../../commit/e7507f0309d11fd79e24ee5cef1a2bf120da589a) [d13c90f](../../commit/d13c90f1c676c8cd7bc5bf69225bcc2563fc76d5) [1f6601c](../../commit/1f6601c151465041f3479e8d6e40b4bed43d3538) [f8f5282](../../commit/f8f52829422e419fbcd57237657ae54d180e722a) [15b7a57](../../commit/15b7a57a2f9bbf5da1b63c05ed4d4ce48282d122) [b54911a](../../commit/b54911af0632f56baae91632147b3ee3504f9b9c) [6ecc70b](../../commit/6ecc70b96ea2b191ad9521e26df36ee0ef7e067b) [e04a16d](../../commit/e04a16debf965f9b9bea0be9147cff4f7b6b9e2e) [dfb13f9](../../commit/dfb13f916c0983c2d998ffdda770307141f63223) [367f0d0](../../commit/367f0d00626d8892d6c947224e33f4b53cfb4bbd) [31ce05d](../../commit/31ce05d666b73345dd9bf55de41d1efd2fb5750e) [e91f433](../../commit/e91f43380b3852ed54b9e4bad2ae96f290d21ac7)
- MSD Calc should build easier - [cff8283](../../commit/cff8283a42881bc022b68c1238b2a15e9f9042f1) [aaefb58](../../commit/aaefb58b8223e6f56c8e880614c5eecc8f2e8911) [853a9c1](../../commit/853a9c106a2d59d36ab0a44154c6313ddf5e8619)
- Favoriting and Unfavoriting is a single hotkey instead of 2 - [249c16a](../../commit/249c16a5327b0fd40b3ba7c9fcc00e5c4940fc11)
- Files with under 200 notes are invalid - [fd16570](../../commit/fd165708e11735863e3960cc4df4559ddeb3acf5)
- Profile Tab Lua Optimizations - [9fc2a99](../../commit/9fc2a99394185ca08f53af77e0ded18de00e9a50) [be7b56d](../../commit/be7b56dc53355eeae83674f483a40c92bdfae395)
- Historic Chartkeys are just Chartkeys now - [4eaa892](../../commit/4eaa8929d7655ec79b5ccf4910c1ae81a9136c6e)
- All pointers to HighScores and SSRs are sorted by Skillset Rating - [d0b12ae](../../commit/d0b12ae36e732d333d8d8c2c7bc71fd9a9798e3d)
- LessThan operator for HighScores now considers only WifeScore instead of Grade - [3f0ed1a](../../commit/3f0ed1a197be38888f7bb566c9b0c54a1c627dd7)
- MusicWheelItems update permamirror, favorite, and goal info regardless of score info - [d35e3a4](../../commit/d35e3a4e522f6e77ead59f83c1ea42e97073ab2a)
- Replace more typecasts with static casts - [8aee60d](../../commit/8aee60d906833a08f69f15daa3a8b994db8c4dac) [8fd4ea3](../../commit/8fd4ea34d5f072eb72f5ddedcf7866ccb5d557e6) [f87d1c4](../../commit/f87d1c49888a07e8a57a5947c7f28f0d9ca4b6be) [50161ce](../../commit/50161ce943aedc1c2f40c3f27e4e7908f224245b) [5ec7e84](../../commit/5ec7e84984607080df477ccdf2e54c302eee57c3)
- HighScoreLists in Profiles are now maps of Rates to HighScore vectors and Chartkeys to HighScoreRateMaps - [884b450](../../commit/884b450b776733c6a927f227272e65c07a7d44cb) [cf8283b](../../commit/cf8283b8183716efd416f36c5ebaf7bf8abdc66e)
- Mouse Wheel Scrolling is faster - [32de0e2](../../commit/32de0e293d28994f4e35221407252ceabcb40d03)
- Judgment Messages send unscaled Wifepoints by default - [087c022](../../commit/087c022ab9121a71bfa51c5ec9a4cfe13ea4e899)
- Clicking a category on the MusicWheel opens it - [412f5b2](../../commit/412f5b2c4382de99e6d8be5627daadbea0f23252)
- Getting WifePersonalBests uses Chartkeys and Rates - [869bc14](../../commit/869bc14a1c4baf8add32de223cc6f6005150e8ac) [515a8f7](../../commit/515a8f7c9a856b269596fb8091ac0362af39fd04)
- Lua must force Gameplay input redirection repeatedly in Playlists - [b712fba](../../commit/b712fba00fa2cddebfda44b0bd3eed4d29d169df)
- Optimize displaying Grades using score keys - [e7348fe](../../commit/e7348fe8619cdc0d25698acdac7ea936e574f80a)
- Sort all scores correctly on first load from XML - [e627277](../../commit/e6272778b069bde3d0cbcd83f60fa1957a04ebfc) [071c8f0](../../commit/071c8f0dc644b182f7c8d6fddc89e6aa90c98054)
- Try to make ReplayData save and load properly - [21b82d3](../../commit/21b82d3a7bded725f743cd73c3571803eac1ab98) [63a4637](../../commit/63a46378a53be0fd997e8380e9f1d925e97141cb) [814214d](../../commit/814214df15d3249a1eecbcb364cbcbaa54a09be5) [ac84e24](../../commit/ac84e24635944a84704ab195b5e6dd31dbcfb63f) [d7c17dc](../../commit/d7c17dceccf2b6a12a3bbc6039a7a6b9ac02c4fd) [920e236](../../commit/920e236e3b87648f3557aa6d943bc66f0335f283)
- ReplayData saves separately from Profiles. Early InputData implementation - [909517c](../../commit/909517c0124e57e89721e743ef5496579a306223) [6219b9b](../../commit/6219b9b094d0499a16fc4cb926f919b335af4431) [1c1c3e3](../../commit/1c1c3e37d76883c6ccb356c98abc1f1f0215a85f) [feb50ec](../../commit/feb50ecc530a9d7663ceb58993a2b7437e11bbf3) [b6036d4](../../commit/b6036d45131847e907eb3cad80361336b025cb53)
- Optimize Offset Plot loading to let it be loaded anywhere - [3ad5aae](../../commit/3ad5aae56c63b266178b42e3c5c584a4130faaea) [26d0f3a](../../commit/26d0f3a354c365120f4a57c360bb82e54ba4149c) [f77f5ba](../../commit/f77f5ba96fccdc40432465469db824985768d369) [920e236](../../commit/920e236e3b87648f3557aa6d943bc66f0335f283)
- Optimize Offset Plot drawing to be a single MultiVertex - [d5cce8b](../../commit/d5cce8b0b345659c33db6d072f46a64a3dff7946)
- Optimize Scores, basically rewriting them entirely - [989fc4f](../../commit/989fc4fa273a7dc85d9ed9bf452d9c4aba37e114) [08568eb](../../commit/08568eb6059d148ee8ef7414ed179352305e6ce0) [4402e2e](../../commit/4402e2eeec3539fa702b43a33e90a16dce68ba55) [e02d546](../../commit/e02d546a483707a9c0994fb85a0d75467f0db12f) [aa5a8cb](../../commit/aa5a8cb11b2080b9a3db6647ca5bb92e6a1e6ce4) [87d4ccd](../../commit/87d4ccd75e0c4af2fc4e162fb0d217d3af761cb7)
- Optimize Estimated Time calculation for Non Empty Row Vectors - [daf3600](../../commit/daf36006fb38f55460e18513bcd938aaebf66103) [4080d27](../../commit/4080d273cce8e4f82e0d39d2217aef8bd8d3521b) [bbea048](../../commit/bbea048084ce9d855ef3a8b83c764137846282d6) [e746043](../../commit/e74604379622aad8f265ccddefcd0984a20a80cc)
- Optimize Estimated Time calculation for single bpm files - [10e9ec8](../../commit/10e9ec8e079385dc1aa3732dd6691318e6ba8824) [4080d27](../../commit/4080d273cce8e4f82e0d39d2217aef8bd8d3521b) [bbea048](../../commit/bbea048084ce9d855ef3a8b83c764137846282d6) [e746043](../../commit/e74604379622aad8f265ccddefcd0984a20a80cc)
- Optimize Loading from SMNoteDataString by not initializing empty Taps - [bbcf62e](../../commit/bbcf62ecf5b37375fec03dcc3bcf5c75b465b539)
- Improve Profile tab code & sorting - [eae3632](../../commit/eae3632f0e6c2cd39856eb58a0b15e1d237ff7e3)
- Rates up to 3x are now supported - [48a799a](../../commit/48a799aaa95a4bd2c561799e83e2285c0fdb9dc6) [2b155de](../../commit/2b155de8cc337305891fc8279be56f8e9699c28b)
- Don't cast a Cstring to an RString - [3d89914](../../commit/3d899146930b027efeda03451352196e36e466ba)
- ETT NoteData compression method is changed - [426fb61](../../commit/426fb61f263e0406a09b5a57e5c0d19cfbfb1a4c) [3724b16](../../commit/3724b1653f5d93e708f4775de1e0c757f8e001a1)
- MSD Values for Charts are cached - [4df4581](../../commit/4df4581fc458c5a6b9ebc662daf27febea8c4c41) [d38a10f](../../commit/d38a10f0df33efa11bdb857f7f550e45fd46011f)
- Radar Values (note counts) are ints instead of floats - [d34bfe0](../../commit/d34bfe0efd03028cd4f74707fc0d31b03c3550cf) [47ca4f3](../../commit/47ca4f3aff711d1a886e7e2959f55d578dee623c) [b87a395](../../commit/b87a3953ecfb8d62c8e82816e2c98456b26f51da)
- Unordered map macros implemented for use in getting Scores by key - [279150b](../../commit/279150bc3b8bd224e98950ed1f75ebc14c73704e)
- Banners have capped resolution sizes - [2525925](../../commit/2525925554f5d32329fdffebd15cc8f3be8fd3cb) [dfe35bc](../../commit/dfe35bc297d5f42c9c837763f7a3a943f82a3af8) [c768ecc](../../commit/c768ecc6fa7c74cf5ef4f76a98f24103b4afd60c)
- Chartkey Generation & MSD Calculation only happens outside of Gameplay - [a2b4b34](../../commit/a2b4b34948e5a7376a96fedaee3de02c904a5f42)
- Properly reset NERV/Serialized NoteData when making NoteData - [bc4b113](../../commit/bc4b113548966bb587c94d989cfa7b5006718c1a)
- Grabbing Songs & Steps is done by Chartkey instead of ID - [3477591](../../commit/3477591b26ab29e8107bf580ac8cdd58e1c8c383)
- Many instances of RString are replaced by std string or Cstring - [de63892](../../commit/de6389291e670335a81a0a0ca36e4fb85a1424aa) [e45d4eb](../../commit/e45d4eb8a45ecc9d1f8d798bf8b38edfb45dd511) [a7326c8](../../commit/a7326c872022a0668830350b973fdd419e7c9c8d) [6a47206](../../commit/6a47206b3d9f1db58ca91a2de492cae381ad9b17) [d76c96b](../../commit/d76c96b7377ff97a43b8b0e412490608130433e6) [a12b25c](../../commit/a12b25c7dc30405ff081bc971abb3515c817ee27)
- Redirect some links from Stepmania to Etterna - [ebad781](../../commit/ebad7811e452b3dbd979981c33b0e614f368a912)
- Optimize vector use by using emplace instead of push - [a29333c](../../commit/a29333cf4323c293fb0e2ab8b32e537e49b02dd3)
- Preference related to the minimum percentage to save a score is renamed - [62de3c5](../../commit/62de3c55b9869962292e7a747d5ef3a7c6109886)
- Autoplay is not human - [15bcdba](../../commit/15bcdba1a7cfcca4fc0b31830655d64c636626f8)
- MSD Calc version is an int, not a float - [809ad61](../../commit/809ad61408af6bd186e5ab47c22ef359f9f50be7)
- Recalculate rating and SSRs for player loads - [eca7cbe](../../commit/eca7cbebcfc50e2eedb4794105bc2a23042429b7) [91882c6](../../commit/91882c69e257692b2272f55a71ffb52de3b3b32d) [79a0bf6](../../commit/79a0bf671e956c7964c94d8d7ebf3157a71ee509) [d0a9651](../../commit/d0a9651bc2de04b484b19d51875eef89f036580f) [35a6438](../../commit/35a643846a73d83a07a46af1ebc2c7978761f7e3) [9c151b3](../../commit/9c151b3258bc88e051280d9bfe4450e0849c8908) [6aa1055](../../commit/6aa10553b2d4613dda62bfcd3542d38ec2753399) [4fabc16](../../commit/4fabc16c6d5e7dd67ca1d4171cd8776009008a1e) [5ba773b](../../commit/5ba773b67f2e2caeb58bc9e5c6fb3afa74f857dd) [a0a6d9c](../../commit/a0a6d9c39992e1316269022b7f4229c53f421de3) [cbacc32](../../commit/cbacc32debe87e4806c5e57c5b6178892e65f4db) [b42c10e](../../commit/b42c10e07c25d756b4e05ffb73fb8b9ab9d7789b) [8ed6018](../../commit/8ed60181306a6d17acbc52bb202bb4ff84988165)
- Building the Estimate Time vectors in the NonEmptyRowVectors, reset the previous ones if rebuilding - [c1c960e](../../commit/c1c960e5cb2f95c85dc190b69f1305a4071b097c)
- Don't generate Chartkeys for files with no NoteData - [f0d645b](../../commit/f0d645bdd56c77bc26cd79dfb793916f2496478a) [dc3cbf9](../../commit/dc3cbf9995d2876d99583ed1dfd8e32f4bce20bf)
- Don't consider scores from old MSD Calc versions in player rating calculation - [6f3555f](../../commit/6f3555f6c7ba5295f232f3501e859f8bab20b79f)
- Don't reset an SSR calculation if the Chart is not loaded - [e40d3c1](../../commit/e40d3c1c542debf1c4c8a2121acc970777082140)
- Don't consider invalid scores in player rating calculation - [69314fd](../../commit/69314fd40bb1dcf7fb51bb9919168d890a2b03b9)
- SSR Scaling maxes out at 97 - [6412d2e](../../commit/6412d2ee9b486753b36dcf82514245a700c8c6b2)
- Empty Steps are ignored for Radar Calculation & Caching - [920d502](../../commit/920d5029a70f5dd9f268b2bcd76a992346730585) [41fe93c](../../commit/41fe93c34b482cce353728e88f92c5b685c5d035)
- Autoplay is now even more intentionally bad - [98fe95a](../../commit/98fe95a540ce996e243922177efa81f969d79cb2)
- Chartkeys are private fields of Steps - [39a3134](../../commit/39a31347ce66c57c4615c4741874d0f41f0336e2)
- Don't check to see if a set of Steps is invalid, because Chord Cohesion is the only reason that existed - [66f6fcc](../../commit/66f6fccdf8801ba7a4f0f56e80240504f1b2c10b)
- Don't grab the loading window from NetworkSyncManager - [084da49](../../commit/084da499a16237af7f91f9a95546845d56c1057e)
- Profile saving and loading operations have checkpoints - [edb50a6](../../commit/edb50a65b7605317d2cd5dfc52cc4c05afe4b081)
- Favorites are not a vector, they are a set - [27b97b2](../../commit/27b97b259b5b3393baa42c1e4d5afc8b23ab4ef3) [6140fd2](../../commit/6140fd2210cad1067191c9c73462adb20069f72a)
- Optimize Tap rendering by rendering Taps when they should be rendered - [65d0312](../../commit/65d0312c455be19d4b22c2b8986ce2fafef95755)
- Load a fake profile if there are no profiles, in lieu of there being no Machine profile - [23b513c](../../commit/23b513cf32c736fa4a3ece38ee2b025965ed64bb)
- Don't try to render BitmapText shadows because they don't work - [ce38224](../../commit/ce38224b6030687850d2ef32ed8a4837bc1a6386)
- On game initialization, basically display a black screen instead of white - [d65b0ff](../../commit/d65b0ff4efe2ef664172ff7669a3a396f194c1c1)
- Filter tab supports 3x rate - [bae2ca8](../../commit/bae2ca8b4c2ef93d966cb2958a970cebf28772d3)
- Optimize rgba to rgba RageSurface conversions using threads - [35ccdfd](../../commit/35ccdfda0a015b23b6dacf5894f8521bd5740ec8) [9b33207](../../commit/9b3320761e8a65f23b159b2befd4217cc5ed5548) [1a819db](../../commit/1a819dbbf9cef9a87ef79a3c2eac3a1a49f00284)
### Removed
- MAD audio library - [21c3395](../../commit/21c33956f89781789eac6dff1c5058a735222328)
- Autoconf files & Makefile.am - [b978a41](../../commit/b978a410ca8f326ee7c6dae42728044f8c23100c)
- Saving profiles directly into the old format - [44d2f21](../../commit/44d2f218741a5aec1e6dea251a8013c5b81eb820)
- Scaled Wifescore because unscaled is the default - [61314b2](../../commit/61314b25db7a56121c233b321cb84249898fa235) [af7eee3](../../commit/af7eee3b87d6f0e2d2f2df60d1e8a0bd09bf2615)
- MonkeyInput (random keyboard input) - [fe50843](../../commit/fe5084362a01bcd512991dc967248d8e6a0d24cb) [0258e9d](../../commit/0258e9d1f280192805829d6cfd11c07485fd7113)
- Some visual references to DP - [8936fcf](../../commit/8936fcfbcf74868ee1ff9d0a9970d0603ed8c1d3) [1fc8320](../../commit/1fc8320f4a4677f903be95e1bf91b7cc28d998d4) [f810663](../../commit/f810663483851108ebb4c12c23f2822ba7060673) [e6795b5](../../commit/e6795b584a79247f2367f50d8d273f3b87a3daca)
- TopSSRCalc functions & Score/PB by key functions replaced by faster implementations - [c65f4f8](../../commit/c65f4f8e7fc7344365349ee9efad8efd2c100262)
- Chartkey records in Cache for debug - [2db2e79](../../commit/2db2e791c8eab195bf003ae278786b5fb6a883eb)
- Autogen from Noteloaders - [de6e19e](../../commit/de6e19e4bfea98d7a6403a7f4b88ddcba8a9158f)
- All other Autogen - [6ad869d](../../commit/6ad869d32aacca09c81da43b4293ef637363cf43) [3e18ec0](../../commit/3e18ec0afa80afe693bc524230c423624f5acc04)
- References to Calories & Workouts - [ec208c31](../../commit/c208c31eb2c57e412ac12ba2960969cf7c5010c3)
- Courses - [2afc922](../../commit/2afc9227450eba1dd62c516f9bc345c21e51be3a) [a09e150](../../commit/a09e15033e2d7218e5bad36e365277e286a8010f) [4d06e74](../../commit/4d06e747190577f902b5a0ea27f17dd2af0c1830) [80424d3](../../commit/80424d3d2dc3d67861027b401221a0df5e6bc958) [5095b95](../../commit/5095b95a08474f21f6576636c6782e4043385d20) [d51c33f](../../commit/d51c33f78dd8251dc82d61161361da21ef85dc98)
- Ingame Editor - [313e011](../../commit/313e011552db8e2a07353a5631ce4e098ae1203b) [3e888c5](../../commit/3e888c59b7f1b75e6a5c002b32a5dcbfdff74711) [3f32e4d](../../commit/3f32e4d6d908108351ae44e7fa9cb56f566d4c6d) [2cf03b1](../../commit/2cf03b14b619fe16f8477521d8dd16a15ddf6bc0) [80424d3](../../commit/80424d3d2dc3d67861027b401221a0df5e6bc958) [314cd3d](../../commit/314cd3d242a0b0d4f1985616ecc9c9e0c6b0b5bc)
- All references to Cabinet Lights - [23ee2c2](../../commit/23ee2c295254094361ecb600dd25a37b51af738a) [4445d68](../../commit/4445d68c0d3be934a4f623409e6f010da4b937d9) [1e66e3e](../../commit/1e66e3e3fc82611138e7a90acdcf95895b2e930d) [43314e8](../../commit/43314e8a2ff51ae243de5f25a336daf543080026)
- Old Radar Values (Voltage, Air, etc) - [b652bd7](../../commit/b652bd7edaab95bfc088c8ae843caefbc490bf77)
- Kickbox Mode - [6219d33](../../commit/6219d336f0deae6c027a3f769b4969fc7c170a1b) [1a72adc](../../commit/1a72adc1d85545925e333c9b8274d662ab8064db)
- Unused NotePerSecond vector generators - [2b5e32e](../../commit/2b5e32eda80572e08b0e5901bc88655c9fcb4dce)
- Jukebox & Demo - [08cccb0](../../commit/08cccb07aebba601ff2650af26a5919e36cd57ae)
- Machine Profile & UnlockManager (song unlocks) - [f87046d](../../commit/f87046df1e0ec7ec0d9b46bf6d3398b22b00918c) [e7e550f](../../commit/e7e550f0496ca77acf7bd78e9e7f03643f8a6307)
- MemoryCardManager - [446f809](../../commit/446f809f1488bdaed1107325615be342870f6181)
- Bookkeeper (CoinManager) - [5682c18](../../commit/5682c18fddbb96fb164815842fb0523c3d505c91)
- Attacks, Battle Mode, Rave Mode, related elements - [89bf1c8](../../commit/89bf1c8505d2b56889085bac2b6c2af70f8ae0b4) [c141e20](../../commit/c141e20d415b94c498dcef694194c157bf4c473f) [3e18ec0](../../commit/3e18ec0afa80afe693bc524230c423624f5acc04) [c0d754e](../../commit/c0d754e20526d81d9f367a93cebffd50148576b8) [d0f9900](../../commit/d0f99001fce6e97de24b24e687df08d7cb93f05b)
- Theme Button template function - [1866a48](../../commit/1866a4832b557583ec6f4ebc3583968aaf4a9348) [9551832](../../commit/9551832f5d850e93a35f41f0d4dd34990d11f23b)
- Hidden songs - [477a424](../../commit/477a4248441e9f75e5e0951e532270cb312a7ad4) [6d2ae18](../../commit/6d2ae189a69faa219b51bfad8c3391d2003500a8) [ef24d4a](../../commit/ef24d4ad20be45ecc12aa5d5a54817a67d373ffc)
- Displaying Grades by Difficulty on the MusicWheel - [04dce35](../../commit/04dce3588cea4d4276b4bc314b450b5cd8858c50)
- Unused fallback theme CodeDetector instances - [db33591](../../commit/db33591da3d344996240930dbbb483216baf7f88)
- Haste mod - [5a44b46](../../commit/5a44b46e11c4cb4f0f907a6c9ac1fd83079c9a74)
- Symlinks (Songs in multiple folders) - [062cba3](../../commit/062cba3ed48a90e1196f2f06eddc098c3cd4db6f)
- Size cap on the list of HighScores saved - [c10733b](../../commit/c10733b87204392a7ec2bfc77ac5f10f1179769c) [0c1c8bb](../../commit/0c1c8bb3974519ae8c1290f21b66acfaac7eb809)
- Roulette & Wheel Sort by Difficulty - [a61ffae](../../commit/a61ffaeec87d98b36f110775903d570e6713d3e3) [977cfb7](../../commit/977cfb70522c0710c03e9238f9da68797d4454f8)
- X11 ScreenSaver stuff - [2da5dbc](../../commit/2da5dbc65932ce93818f2118feba4819b3dca386)

### Fixed
- Crashing on old scores without a listed rate - [27659bb](../../commit/27659bb08b5c1d5ab12f62201af2bf69956fb51b)
- GetScoreByKey is broken - [c9e4e1f](../../commit/c9e4e1f7209e98c61cdb024d5fd428ff7986c68e)
- Song Search doesn't land on a Song - [0b063d9](../../commit/0b063d96082d589a6783734252f378b682e8bc16)
- Float to String function usages are bad for the XML - [2bd3a28](../../commit/2bd3a28a4a4ba2b684f164e11b9e35415d4b03b1)
- Memory leaks caused by MusicWheel scrolling generating new objects - [151e083](../../commit/151e0839ca6a2d77736d32117a6a88a54449e458)
- SMZips don't work - [e59c916](../../commit/e59c9166cad46c0e50745acfcbb0c28e96ac8f95) [9bc5440](../../commit/9bc544069b020a2f6398124858a832dff19fca01)
- Can't skip the init screen - [d79a5e3](../../commit/d79a5e31ddf20bb8abdc7b397489e5e5bf223232)
- The ReloadSongs button is broken - [7858734](../../commit/7858734fbf02528b8a3fa788170f92f24d95281d)
- Autosync breaks in Gameplay due to pulling the precalculated values from TimingData - [c516aaf](../../commit/c516aafdf7b62804a2876f36a18bb89d27102073)
- Online banners & MSD labels are broken - [a25f5b0](../../commit/a25f5b0a25093a8668c03fec6df6e4c8c16efd4a) [7d0a25d](../../commit/7d0a25d01e2734ef2e11c28d9e754897f91158d0)
- Deleting FileSets leaks memory - [2c0385c](../../commit/2c0385c1c17faf030a2097bb31c84970c0861152)
- Mouse positioning coordinates are bad for Windows - [93f05ea](../../commit/93f05ead38228ad7d204fb0f779b156aa600867a)
- Mac build doesn't work - [a63348f](../../commit/a63348f3aa256813a1a3425c58371a09707b6d5b)


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
