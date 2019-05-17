# Release Changelog


## [0.54.0] - 2017-01-21 - Pre-release 2: Replay Data Saving

Windows installer pre-release.

### Added
- Filter Tab for Skillset & Rate Filtering - [d614537](../../../commit/d6145377c585efe19ef9635d1f6fbf8671afbfa0) [17520b4](../../../commit/17520b47b14167e01655473636720d35c02275be) [2ae00cf](../../../commit/2ae00cff074f427ff3a9b127f7663a4bfe899993) [76767bf](../../../commit/76767bffde825e17d98bb8985a304f3bc0f5c896) [14fd529](../../../commit/14fd529a8351c255969d67f2049564da3194f46a) [41cb039](../../../commit/41cb039af168ac87f06325f690ce6d427bde0199) [9686b5e](../../../commit/9686b5ee36b0a0cdc8044e49b8b4a698ec80d5bb) [af0ece8](../../../commit/af0ece8b118484610888b62059b3f97bb5a1c885) [b53b286](../../../commit/b53b286f5ed522a07e30ec467645d79df198472c) [42296ef](../../../commit/42296ef8f8c40c3e593050c10bf01749a4ccf19f) [fcb31fc](../../../commit/fcb31fc6ef6031571df505cc1c79202e3b203cd2) [43f9411](../../../commit/43f9411625f874334d6dcc6bd4a7d5119787d192) [56c151e](../../../commit/56c151ef2f8c613d3b6f57068143f102bf13bf1d) [502c56c](../../../commit/502c56ccb6bd04119e2dfb6cabc87c0bc6dd158d) [8ed006e](../../../commit/8ed006ed24b37e1d8bc4fe591b86a31654d4a4ab) [8516861](../../../commit/85168610db443754082b667e80b5c675a62bee4b)
- Hollow DivideByZero Noteskin variant - [930e2f7](../../../commit/930e2f7f261ec1031f126c6b13c2b91f4be12b2c)
- Preference for not being forced to reset video parameters when a new video device is detected - [332bc78](../../../commit/332bc785cc59627dbdc48af9b506039f9ca178d3)
- Proper top 3 Skillset Display for songs - [a5ab25f](../../../commit/a5ab25f129701ce17d209b2115ef7ff9d467a129)
- Replay Data Writing & Saving - [c348b88](../../../commit/c348b88c3cc3036460ecf4c0cd276a071fb6eadd) [1762008](../../../commit/1762008d90eda7bbea077f42724304a1292dce56)
### Changed
- C++ handles index shifting for GetMSD calls instead of Lua - [26a2778](../../../commit/26a2778933bdc66d02d3090dbd05d215e58b0dbc) [d974464](../../../commit/d97446423595a76e38308830650f5a75549aa580) [070323c](../../../commit/070323c3753ca834076b813831402757b532ade2)
- Changes to the Multiplayer visuals & functionality occur in line with offline changes - [b686dc5](../../../commit/b686dc50de2b931d9514f4f1b9fce4ce743c1a38) [55828f7](../../../commit/55828f7f540749b4307ee8e5908485b65b151718) [abff056](../../../commit/abff05620ef3fd02ee5b000c6fa146caccdd699e) [eda999e](../../../commit/eda999ea9a746ea105b371a4944bd132226ccbe2) [30c0d5f](../../../commit/30c0d5f6da2b64ca82071c65a3a80e14cbc63add)
- Improve loading of the Profile tab for the first time - [34dbd39](../../../commit/34dbd393e211709f78cfebf0c333608ad8d047ec) [a1f7923](../../../commit/a1f7923a5f8a458784307937014182cb246494e3)
- LoadBackground on Sprites does not work as long as Tab is held - [176fa6d](../../../commit/176fa6d9fd9c74df7b6b88f82d580967a28377bb)
- MSD tab average Notes Per Second scales with rate - [d51f154](../../../commit/d51f154ac8aec90fe3f564b24bf8a618dce4f80e)
- Optimized DivideByZero Noteskins by falling back to the original & remove some tap explosions - [e008a9b](../../../commit/e008a9b65f1bf4ffbd307e45ec4e76a1337aae40)
- Optimized DivideByZero Noteskin by setting AnimationLength to 0 (for FPS) - [142c8d5](../../../commit/142c8d5e9e91f632be04c207c624923ac2aa5fb7)
- Profile tab page number & visual update - [f03dfe2](../../../commit/f03dfe26425f06c99c05667b97470a4dbed5a191) [118833c](../../../commit/118833cbd8e37e4eabad3b863adfadfee7ff772d)
- Renamed the bar skin to SubtractByZero - [012fb8d](../../../commit/012fb8d05fb5871eb4e82941de0bf4977d64e318)
- SSR rank lists reload after Evaluation screen properly - [72dd7a1](../../../commit/72dd7a1b782a67c192dd4bda3693c9f055a07b14)
- Uninstaller does not need to delete the Theme - [49aa617](../../../commit/49aa617ce6e16c868a9b07a97afe4ec0434dcfc5)
### Fixed
- Background Type Player Preference didn't persist - [f022192](../../../commit/f022192714c6456c27bb6e23e5cefa4bb80d8046)
- Favorites could be duplicated - [4889947](../../../commit/4889947a20dd6fa57ee8278b671d153b88f53129) [ae2a9e5](../../../commit/ae2a9e5f7b5fe6e7a72e53a09cf803fb0286aae2)
- Favorites didn't show up correctly - [5a1ca79](../../../commit/5a1ca79beeb33889d41e3cd6f14b43ae7d681052)
- GetHighestSkillsetAllSteps failed due to a typo - [bcae636](../../../commit/bcae63699c015c0d8fb87427b66ff2ad5e946da4)
- Game suffered obscure crashes when viewing profile due to invalid SSR values - [28a4129](../../../commit/28a41294d1a4c151ff247bfa669f398b8ec199c0)
- Garbage information was outputted from HighScore lists after new HighScores were made - [3f1b927](../../../commit/3f1b927126ba262d3114a569b3c3368c32f12f03) [49433af](../../../commit/49433aff6d70cbfcffb25705e508269b06b220b4)
- Halved & Semihalved DivideByZero Noteskins had pixel artifacting - [032564d](../../../commit/032564df43d9d6a97432765dd9d602e0f306dd9a)
- Lane Cover BPM display sometimes had weird floating point display errors - [5e40bf5](../../../commit/5e40bf54c8ac5ee8cb752db822f822fe66d7d32a)
- Mini Player Option loaded wrong because of the load order - [7092e5e](../../../commit/7092e5ed69e5424af5b3ea2478a4ef8cea92bd9c)
- Rates outside 0.7x to 2.0x caused crashes - [f8e746e](../../../commit/f8e746e071a7f0a191b2a16678823cfe05fe4b81)