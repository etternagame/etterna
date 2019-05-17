# Release Changelog


## [0.65.1] - 2019-03-10 - Quality of Life Patch

Windows x64, Windows i386, and Mac installer release. Linux tar available.

### Added
- Combo Text Coloring in the Color Config - [e33e1c5](../../../commit/e33e1c57e6975b5312149e49cfc39e2254f74669)
- Lua is notified of Options Screen Exit - [2abb7e1](../../../commit/2abb7e191da7cc0794f74933efaebc20bf0be1c2) [5a45f29](../../../commit/5a45f292a956e4c895f4d84af1065d72b43e3a8b)
### Changed
- Calculate the name positioning on the userlist with more correctness - [7699738](../../../commit/7699738b1d5b3f89059d034782ce01d09ba75686)
- Common Pack Filter Option should persist for the whole session - [531cd21](../../../commit/531cd2140a8ab33786df997c5453336405673613)
- Don't need to parse for comments being inside of value fields in simfiles - [f372151](../../../commit/f3721519b11a0fe66ca043015ffce7ea7630c645)
- Forcibly log out of Multiplayer when entering the Title Screen if logged in due to tough branching logic - [ec40e66](../../../commit/ec40e66e3291f3a9f0f49d9ecebbdfad21a3dd59)
- Lower bound for Roll Life should be set to Judge 7's window since anything harder is basically impossible - [49457ca](../../../commit/49457ca78f72556b43b895326a9994d42a893e2e)
- Metrics-generated Multiplayer User List should not be visible in Til Death - [0383411](../../../commit/0383411907d7087297bfab27c8ceaae4b3cf39ae)
- Multiplayer should not start extremely quick when on rates. Make it all consistent - [11ee54b](../../../commit/11ee54b7246894955cb2f5b7cc13d0bee1f80879)
- Only delete the installed Noteskins on uninstall/reinstall - [5dbb444](../../../commit/5dbb44479629cce10ce46c34bd199fbfbf79cf35)
- Optimize & Improve Practice Mode
  - Cursor should appear in Practice Mode - [6932fce](../../../commit/6932fcea895f514a68b5689cb5e18a3117a48155)
  - Don't reload the NoteField when moving - [8cf63f5](../../../commit/8cf63f5ccfed4ca8b8ab0fd317ae2d6762e70abc)
  - Don't send Judgment Messages for Misses when seeking to prevent tween overflows - [dc08985](../../../commit/dc0898537a4a7e7b96fed6631c49a6630de629d3)
  - Resize the Chord Density Graph properly - [cb849e5](../../../commit/cb849e5d921f1930802703a723d5516643c470b1)
  - Update relevant numbers when changing rates (for Replays too) - [679e671](../../../commit/679e6710017a029ce0fcd067b1111dcc8327372a)
- Reduce the number of builds on the master branch on Appveyor - [d6736a6](../../../commit/d6736a64607fbaaffb736124a75f5371389afbfb) [945acc0](../../../commit/945acc0279524de1734a54e129fa7f165208eff2)
### Removed
- Arcade Options and the last Preference on that Screen - [92819b3](../../../commit/92819b3d91d408507c32a627e686a25b2af768ad)
- Last Quirks Mode Reference - [2849f88](../../../commit/2849f88502af6bb13478bcd3d5e3a9c0e36ed68c)
- LibPNG to be replaced by stb_image_write - [3047db4](../../../commit/3047db410bed28c2731848905025a72574ae7fb1)
- Useless Lua input callbacks - [b4f5c61](../../../commit/b4f5c6164de17d228cb6370e2026ea0671e45b7f)
### Fixed
- Changing Judge in the Options didn't update on the frame - [5a45f29](../../../commit/5a45f292a956e4c895f4d84af1065d72b43e3a8b)
- Combo Breaks per hand counter on Evaluation Screen didn't work beyond 4 keys - [cf0c5e7](../../../commit/cf0c5e7123df17545624de3a039e2a147bd6ce01)
- ErrorBar movement didn't save - [ca99351](../../../commit/ca99351345a467ad4f90c7cd3284d2d437434522)
- Exiting Network Options had a weird branching issue that led to the wrong Screen in some situations - [a64486d](../../../commit/a64486dc8201bf263b7e513211115b4284768cec)
- Holds/Rolls in Practice Mode sometimes appeared covering the whole screen - [84c8eca](../../../commit/84c8ecacd23a39e642c863ee8eaf5ff89253bc80)
- Invalid UTF-8 in Charts in Multiplayer crashed - [303bb5b](../../../commit/303bb5b180f755a078dbc9dcecae05ddd683653d) [8101004](../../../commit/8101004e5117fd3693bec84325abe1aa355dd962)
- Loading from Cache was extremely slow. Fix by doing the following things
  - Save full paths to all Song assets on load from disk - [993dc77](../../../commit/993dc77936cb703d926bc24ff1c0a15806fd7c41) [1c7fa2d](../../../commit/1c7fa2d0fac5f80373569526fdff11d75fdc4aee)
  - Don't generate all full paths every startup, use the values placed into Cache by the above change - [6a5ae27](../../../commit/6a5ae27490db27983ef786d3bc996ca371860047)
  - Lyrics don't save right, fixed that - [a672e95](../../../commit/a672e9587e9683b9f0975ad7195d8466fceeee0e)
  - Correctly load all assets on first load - [9bf1f46](../../../commit/9bf1f467efd293a9d99a1fce2b7c01ae24b55518) [677a439](../../../commit/677a439bcc2a8ebf367eeb7cd190054986806253) [43ae1a0](../../../commit/43ae1a03f251070d624dc86a180d12c1215eb336) [262f314](../../../commit/262f314d0e5564cdc463996e4166c3488aee2716) [2264db4](../../../commit/2264db4b7ef209735ea6542573876896e6b47d6a)
- Lua errored when not in Multiplayer due to unchecked attempts to reload the userlist - [c121787](../../../commit/c121787367f5201ec1f7a171cd9374a601d5635f)
- MusicWheel jiggled when picking songs - [5b1e8ec](../../../commit/5b1e8ec9593112a7430302a9ea43192174ad1589)
- Playlists reset the NoteField Y position for no reason - [54c390b](../../../commit/54c390b8ead4e9221c2e928031550051c25dd42b) [9fbb044](../../../commit/9fbb044aeccbd826f59036066a7d4e16dc5f5ee3)
- Profile Options Screen crashed when you pressed anything other than exit - [b7e8469](../../../commit/b7e84690d6625524d9d46359b1126b9767d035a8)
- Startup rarely crashed due to non threadsafe function - [c521f1c](../../../commit/c521f1c3c5758043f1a02e654fc11c821c18e1e8)
### Note
- Temporary (reverted) change: MP3 Chart Preview Sync Fix Attempt - [4fc06ba](../../../commit/4fc06ba4cd8de84e5b9e39ce8d7df80e4c096f7e) [62a9b78](../../../commit/62a9b787caa5048508f54f4dcc84528fe8c31dd1)
