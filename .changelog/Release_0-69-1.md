# Release Changelog


## [0.69.1] - 2020-00-00 - Hotfix

### Changed
- AAA minimum moved to 99.7 - [e4f8def](../../../commit/e4f8def6dd840225e8eeb73ac01917df7f57afbe) [6d46e26](../../../commit/6d46e260103639af6dc27c4b26bc6f27f0624ec5)
- Chart Leaderboards will floor Wifepercents - [4b63725](../../../commit/4b637259be7121963537d666cb980fc3dda30dca)
- Chord Cohesion setting when recalculating SSRs is simplified due to the Wife3 changes - [3cf42c3](../../../commit/3cf42c3260666662f5d695f0400b0c8fe08753c7)
- Manually uploaded scores doesn't need to check the rescored flags, just try it - [3a8cdf6](../../../commit/3a8cdf64b07af8faf15f7d3bcdbc677dbd43628e)
- Replays found to be garbage due to data writing issues will be filtered out and appear in logs - [393da39](../../../commit/393da39d09a302c120a491fa2cfdb0dbd9f69f22)
- Replays placed in the wrong folder no longer crash without a clear reason - [77e7d1c](../../../commit/77e7d1c73263e2ab43d24045db034654b39560bc)
- Replays on the v1 version will load if placed in the v2 folder - [780cc58](../../../commit/780cc581a0d0faf17c78d0d37cf90095b75c8e6b) [369d85b](../../../commit/369d85bad8966daa485080876bf8f7d2b6523dfa)
- Rescorer allows fails since other functions handle them - [53a19a5](../../../commit/53a19a5670353998f79bd42f164897ffb3870240)
- Rescorer will avoid needlessly loading Notedata from file a bit more - [9f566ad](../../../commit/9f566adb895fa9d4f9b2aec48e3b939b6f416010) [7b66869](../../../commit/7b66869c878e5f8d39989bd4da5257b298ce946c) [39ae0db](../../../commit/39ae0db131bad91a6f30fde4ff5006d20dbdb3c4)
- Rescorer will avoid rescoring Chord Cohesion On scores - [bea64d1](../../../commit/bea64d1cb1f73ea5860c2fe33253dec7bde3663d)
- Rescorer will valid-check scores only for SSR calc - [70dbc76](../../../commit/70dbc76e56ab396f1e838798ab32ab2bac12e005)
- Score sync will automatically upload the scores that haven't been uploaded as Wife3 - [9e5bfca](../../../commit/9e5bfca47b95be362f69ca53b9d699f265fed308)
- Solo calculator now has capped SSR at 96.5% - [9f5e73e](../../../commit/9f5e73e3bd093725b8fa28cd52a0e4cf95162894)
- Uploaded scores no longer care about the TopScore field - [d41847a](../../../commit/d41847ad6603d9fa19d34f6f3a1ea477f13f0f1d)
- Uploaded Wife3 scores should avoid reuploading over and over - [53beb94](../../../commit/53beb948c64dc3b5ed9524d3ae8561849253557f) [17f2f2f](../../../commit/17f2f2fd3214e291968110c8e3e9358da894a298)

### Fixed
- Coverity complained - [6501e6e](../../../commit/6501e6edc1bdaaeec81973983d1bbe4e8c18ed16)
- Cursor stopped appearing on Bundle Select - [20e5fae](../../../commit/20e5faef2cfe5f830891b9706acb7c9f352070de)
- Gameplay threw Lua error when turning off Combo - [c2ffc91](../../../commit/c2ffc91c22c41a0351cc42456041b116de5a7c44)
- Logging in failures due to parsing errors broke the state of DLMan - [da16186](../../../commit/da161866f6d4169b119f1001230d7e6ebad1b403)
- Solo files with 1 "interval" crashed when calculating difficulty - [f830e5e](../../../commit/f830e5ec40d317e85a1de5f4bca3ca331f8f094e)
- Replay Data pretended to not exist if picking 2 scores from the same person on the same leaderboard - [f3a5832](../../../commit/f3a583206413060df3c88e64c4a1eda57a7bcb61)
- Upload function used to cause issues if there was nothing to upload - [7697634](../../../commit/76976343cd5e5dd426b158d843ed512e4cb5a161)
- Uploaded scores for non-pbs per rate tended to overwrite site scores - [e9c4e12](../../../commit/e9c4e12fe696210e5b71170dd09019c68afcade0)
