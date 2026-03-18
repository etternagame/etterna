# Release Changelog


## [0.75.0] - 2026-08-26 - (WIP) Etterna 10th Anniversary

### Added
- Linux ARM64 support - [#1396](../../../pull/1396) [c0d2c47](../../../commit/c0d2c476455d798fa84c549a53286b7030880b26)
- HoldReleases added as a PlayerOption, which basically functions as a No Regrabs option. You can release late or exactly on time to get an "OK" - [4940ef2](../../../commit/4940ef23a8499093d63f472d5e15be622349fe01)
- Linux implementation for cursor hiding - [#1417](../../../pull/1417)
- Lua can use `TEXTUREMAN:RageTextureFromBase64(str)` to try to generate a texture to put on a sprite with a base64 data string - [b16b4c2](../../../commit/b16b4c2ee1fb13fc569eba5dc5590562f430a0fe)
- Lua hook for `ActorFrame:AddChild(t)` to allow passing a Actor Def table instead of a file - [9693c91](../../../commit/9693c916276cd557949164f0e7dfbfdb7db83a84)
- Lua hook for `TimingData:GetElapsedTimeFromRowNoOffset` and `GetElapsedTimeFromBeatNoOffset` - [8e3152d](../../../commit/8e3152df2eba4e290ab700a7097ae080997d0f2c)
- Lua hook for `SCREENMAN:GetOverlayScreens()` to have some sort of access to the overlay screens instead of none at all - [f9694c0](../../../commit/f9694c0b5e5d2a2942452face340b30ef65a2252)
- MultiToasty option is added to Advanced Options and Rebirth Theme Options - [55d50e9](../../../commit/55d50e9e16d19a7a8b700b433ac9f9e6e5e629db) [a8b1692](../../../commit/a8b1692c050c2ac1b65ecf4f331aeb4bf95583eb)
- Pack banners loaded from online API can be shown ingame - [78bf78c](../../../commit/78bf78cb64149e3d32d5e77dcc2ff23ae3cd503e)
- Replays show the hit offsets of the taps, controlled by Preference "ReplaysShowOffsets" and controllable by Noteskin - [c6f63d5](../../../commit/c6f63d57fca83b925208c049f5b5cc8eb4b017a3) [1d67714](../../../commit/1d6771448570824d4fd6f35f3ae201a15cb57916) [9937c07](../../../commit/9937c07096486427afe3bec604b3b6c0c04b145d) [#1413](../../../pull/1413)
- SetPage Lua hook added for pack download paginations - [647659d](../../../commit/647659d49aff971899b9d2cda632935d17a2f0d0)
- Sprite Lua hooks related to CustomTexCoords and CustomPosCoords - [081a681](../../../commit/081a681ca387ef5ef0b01df07e7a2f9d18253e9a) [29fb334](../../../commit/29fb334ee6a88a8797b58e497bfb0789f30f510f) [1c012f3](../../../commit/1c012f35f941bc36bd1acac0d2b68965596c994a)
- Til' Death Evaluation offset plot allows click dragging a region to see the stats for that region - [79d1632](../../../commit/79d16323a0abf0b1fe62ad11264a46041eafe661)
- Workflow for Linux ARM64 - [959cbed](../../../commit/959cbed468d0e9b374803e4e193a1bbb5cf7d93d)

### Changes
- ARROW_SIZE is more well type-defined in the C++ codebase - [93c7b48](../../../commit/93c7b4803c6e14b696367ba35c25851ef3d4b8e2)
- Banners on the preinstalled themes now support spritesheet animation - [8d9d8e9](../../../commit/8d9d8e9a27fc91adcf1d0e4c042c0637deae5967)
- boost.nowide updated to v11.3.0 standalone - [#1401](../../../pull/1401) [de5b6df](../../../commit/de5b6df043202f7d3f63677f128b6fb7e9fa2dcf)
- Calc debug Lua code moved to central location - [4003532](../../../commit/40035326ef493de80d6d4091f2c0ed2cb8b0c946) [466d4be](../../../commit/466d4beb1cdb8d53f7a7ef718d1b83c990683f6d)
- C++ style safety for C array size macro - [da91291](../../../commit/da91291c2f635280de1aff8e23fefd49aaec707d)
- Discord RPC replaced with Discord Social SDK 1.6.12170 - [ed7b178](../../../commit/ed7b1781261f9e78e42f93677c7e1a88b72216ef) [46b8f7b](../../../commit/46b8f7b9250d0857f6f71b961d19e0e862924fd5)
- Functionally illegal upward traversals in directories won't crash the game, but still won't work - [272d6bb](../../../commit/272d6bb2cb668d92e593fd9daea88b109446076f)
- Lua calls to `Song:GetTimingData()` replaced with `Steps:GetTimingData()` - [2e11c32](../../../commit/2e11c3266721083a50857efc8b2178a53d3f2576)
- Lua function `isOver(x)` supports variable arguments, where it returns true if mouse is over ANY given Actor. `isOverAll(...)` is added for the other logical case - [8a6c9e0](../../../commit/8a6c9e0d0dec043c04c4578e4d2487b98d85117b)
- Mac might properly prompt for input permissions on some platforms like 10.15 - [c45d547](../../../commit/c45d547f5f6e03254d8e82507069fbace9e53b68)
- MinaCalc pump/5k support enhanced - [#1351](../../../pull/1351) [731f270](../../../commit/731f270dba03f2ebc83f7b09a8f7bf799cf08dd1)
- MinaCalc various changes, chordjack re-re-rereimplementation, reimplement pre ver 263 pmods - [d248df4](../../../commit/d248df412a99a772ac926017bee0b20d617d630e) [b731de7](../../../commit/b731de7960ee49dbbe222d7a230617897bdb1109) [0ff8667](../../../commit/0ff8667215aa1e5aa3994ef464187fba07af3b96) [657314c](../../../commit/657314cb3fb23f23062cd40006f168499b0906ca) [39c1425](../../../commit/39c1425f3695e69f86c809bac651b0aea24b25e0) [#1403](../../../pull/1403)
- Moved to OpenSSL 3.5.x - [d3dd7de](../../../commit/d3dd7de8c5d3cf50d6c4c1018a2e38c955ecbcfc) [0414f3c](../../../commit/0414f3c2908aebcb8395e1e3309c47bbd1a9671d)
- Libcurl updated to 8.13.0 - [6a10dd4](../../../commit/6a10dd4b5e5d7017df7a7ea99dc9ceac9b7f2997)
- Rebirth Evaluation screen elements slightly moved around - [2dc2eba](../../../commit/2dc2eba3c3c1b3eed9550646650d510b0d11b1b5)
- stb_image error messages improved - [ac8bc5f](../../../commit/ac8bc5f0ec38967634a91b5dadc8d8fa36692aa0)
- stb_image replaces the BMP write in the game, which is unused - [#1408](../../../pull/1408)
- Sync changes of any kind in Gameplay will prevent saving the score - [e11c1c3](../../../commit/e11c1c30639d686b146532bd79bd525d90555bbc)
- Theme specific lane cover setting is moved into the Theme Options in the Player Options screen - [3efa708](../../../commit/3efa708ab9e2fc0dd60ef8f580a8160c3e90b310)
- Til' Death help menu is enabled again by default - [1dc1d16](../../../commit/1dc1d160983c100ae3a8697838f88de8e952ae79)
- `TimingData::WhereUAtBro` is renamed to `GetTimeFrom__Fast` and the usage in the code base is cleaned up - [92b1c21](../../../commit/92b1c21addfd0e2a6331d665f6c9352d6337cc57) [4d0ffc2](../../../commit/4d0ffc249bfc9be5d7acf61d8c5474c167f54c19)
- Toasties are allowed in Replays - [dca497b](../../../commit/dca497bab1bcd8c45eb42c85bb4958db65def053)

### Removed
- GameTools old Font stuff because antivirus is flagging for no reason and we don't need it - [b575565](../../../commit/b575565afa782587104717d58108a0b960842ad2)
- Unused GLES2 implementation - [6b99cf4](../../../commit/6b99cf4134491266b4e037ccd571136e3bcad926)

### Fixed
- Asset Settings menus had weird scaling blip for Judgments - [a9a68fe](../../../commit/a9a68fe4d44f5ef58bc992426494bd63e2ca9c52)
- Calc debug assert - [164bd55](../../../commit/164bd55187fd7e29c978432bd4982c39dfe1c795)
- Debug menu texture/metric reload didn't trigger ReloadedMetricsMessage and ReloadedTextureMessage - [0f5a96c](../../../commit/0f5a96c05aebe82d31937c3c4ac58f1c29573ee0)
- Linux input devices (obscure) might load now - [2f1b598](../../../commit/2f1b5984c0be2f109e0c9b23cd958b4abb971338)
- Lua hook for `HighScore:GetCountryCode()` was wrong - [152d947](../../../commit/152d94732e4123206b2705fd0f539e4a3c4823f4)
- Lua settings_system could be broken by saving content with a Lua keyword in it - [f068bf4](../../../commit/f068bf43d2e7cb154a703aefbf1c1b31a8ea243c)
- Til' Death Lua errors in SelectMusic - [ceb88aa](../../../commit/ceb88aac4866f9bfed17a5f2fac008d9077a48ee)
- Til' Death and Rebirth NPS graph drift - [5107bcb](../../../commit/5107bcbf800e5a29a9f64221d0c65fe192fa227e)
- TotalHolds in the Etterna.xml now functions and represents all Holds and Rolls hit for all scores. To find the amount of Holds, subtract the TotalRolls - [e330b32](../../../commit/e330b32bd565fe48a7129202fa3d78ec1dbf4b1d)