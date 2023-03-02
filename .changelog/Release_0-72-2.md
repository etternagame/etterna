# Release Changelog


## [0.72.2] - 2023-03-01 - Hotfix

### Added
- Custom Window Config file has had some short instructions added for recommended use. No plans to move the config to a Save file, because of the nature of saving a Lua function in text - [9e92f37](../../../commit/9e92f37884f8114f9d0801fa7d5b19ab287cf7f0)
- Lua can take advantage of new Message Broadcasts that occur when certain F2 related events occur, ReloadedScripts, ReloadedMetrics, ReloadedOverlayScreens, ReloadedTextures - [c7b68fa](../../../commit/c7b68fa2c5006e4751f424d264702ca887b9fb20)
- Rebirth color config allows changing the title screen logo, gradient, and triangle - [5f67bb7](../../../commit/5f67bb70ab2087254e2b275afd427c4922796c6a)
- Rebirth has a new option for Wheel Spin Speed - [f0eb69a](../../../commit/f0eb69a1b85c10e74d7ec1be8c2d1fbdf74419af)
- Rebirth hovering your local player name has a new tooltip saying that clicking will rename your profile - [b0be391](../../../commit/b0be3912a944b5e035626ab3d11c1bf91c1b8e9e)
- Til Death has a new Mean display added to gameplay. It defaults off. Its customization keys are `M` and `,`. You can turn it on with an option next to the Percent display - [e1e78dd](../../../commit/e1e78dd0c77d65964b95ca4ddf285baf6d9eb58c) [02ce2d8](../../../commit/02ce2d8a25126c80e578e40700b5e355e8a1feec)
- Tooltips can have their text size changed from Lua - [4533e70](../../../commit/4533e70352e25d94486793a2eef29ce04fa44f55)

### Changes
- GetDirListing API has been changed to allow filtering to only real files. The old API will work as it already has - [71991a7](../../../commit/71991a707c6037485873bb6c79cc04182239bee5) [209837b](../../../commit/209837b22abdb5cf418cb3bfce60ac0a18752f3a)
- Image loading should now be slightly faster in certain conditions, particularly on the D3D renderer - [#1239](../../../pull/1239)
- Network Options is hidden from the Rebirth menus because it isn't functional right now - [f52f4b4](../../../commit/f52f4b4d34f2474da367a05636c8250855d6569b)
- Rebirth MeasureBars option was moved to the ThemeConfig internally, which means that setting is reset for everyone - [298bec9](../../../commit/298bec9f894fab1b354651e112682edf7989e35c) [34986b9](../../../commit/34986b9bfd99a6240e0328c502a5a39c6a477204)
- Rebirth Player Config is now loaded per profile. Profiles load from the old place in `/Save/Rebirth_settings/` unless not present, so no change is necessary for the user - [6ebb86a](../../../commit/6ebb86af507a5bf0a9a3c00fe9cc486028f7fff2) [2c53bd5](../../../commit/2c53bd5d042a768a999be7fde348ed8a417684c3)
- Rebirth Settings allows hovering the empty space between option names and option values to select them - [b96cdba](../../../commit/b96cdbadb9f7ceef719244326d1f7d63472122e9)
- Til Death Tooltip size reduced a bit so CDTitle hovers aren't huge - [495545f](../../../commit/495545f6d5c6736a8da134549b9be0bcd0184d76)

### Fixed
- BPMDisplay stopped showing special random/unknown BPM types because SetFromSteps didn't support it - [24bb49b](../../../commit/24bb49bf929f72a23700bc70b36cd19c55442071)
- Crashes by abusing Rebirth ColorConfig and CustomizeGameplay functionality - [603c0bc](../../../commit/603c0bc15405939bda5106656028310f6c331330)
- Crashes by placing random audio file in a pack folder, or by naming a folder with an audio, image, or Stepmania extension - [71991a7](../../../commit/71991a707c6037485873bb6c79cc04182239bee5) [209837b](../../../commit/209837b22abdb5cf418cb3bfce60ac0a18752f3a)
- Crashes by finishing a download between the moment when you select a song and before entering gameplay - [40f0550](../../../commit/40f055016caf839363073b7d48ae7a8aebb066ab)
- MacOS support for getLanguage slightly fixed for older versions. This may reduce rare crashing - [0ddd71a](../../../commit/0ddd71ad22f3276468611938b3fdac670492219a)
- Mouse cursor froze when reloading scripts or otherwise breaking mouse overlays - [77ee5a6](../../../commit/77ee5a694c5fecf0f3698ea71ed3b3d37febaec1)
- Rebirth Music Wheel should no longer randomly stop on files for 1 frame, causing a huge stutter - [232be6d](../../../commit/232be6dd2552ab1a93a50cb1cfd8a7ac19d401d1)
- Rebirth Song Search would appear to do nothing if a search returned a chart that appears on multiple songs, but one of the song results was filtered out - [5fb7850](../../../commit/5fb7850c0facc856a3e07687ddec2055aa31a47f)
- Replays with ghost taps before the beginning or ghost taps in large breaks, but close to incoming notes, would cause the rest of the replay to be offset and broken - [3f8f572](../../../commit/3f8f5728c82cd328ceec70b89dcee5e958fbf24a)
- Replays recorded with one particular set of Song Offset and Global Offset settings were wrong when played back after changing settings - [6a75685](../../../commit/6a75685c1e2bfb413988af6946b90ed7bd5f4d48)
- ScreenTextEntry should no longer cause the music to stop playing, breaking the visualizer - [64d23ef](../../../commit/64d23efc643457b5a321d135843744d54be846bd) [5a71afe](../../../commit/5a71afed147b1f7ce0289e6984f86f8089de9e00)
- Til Death Evaluation showed the wrong mean if you hit a mine on a live play (not replays) - [424eba6](../../../commit/424eba6249221e339eea1a37d643fb38e02893e0)
- Tooltip overlay shouldn't produce so many errors when reloading scripts - [4918feb](../../../commit/4918feb1390ff4b40229893f4cab3200f6eb5e37)