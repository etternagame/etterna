# Release Changelog


## [0.72.2] - 2023-03-01 - Hotfix

### Added
- Custom Window Config file has had some short instructions added for recommended use. No plans to move the config to a Save file, because of the nature of saving a Lua function in text - [9e92f37](../../../commit/9e92f37884f8114f9d0801fa7d5b19ab287cf7f0)
- Rebirth has a new option for Wheel Spin Speed - [f0eb69a](../../../commit/f0eb69a1b85c10e74d7ec1be8c2d1fbdf74419af)
- Tooltips can have their text size changed from Lua - [4533e70](../../../commit/4533e70352e25d94486793a2eef29ce04fa44f55)

### Changes
- Image loading should now be slightly faster in certain conditions, particularly on the D3D renderer - [#1239](../../../pull/1239)
- Network Options is hidden from the Rebirth menus because it isn't functional right now - [f52f4b4](../../../commit/f52f4b4d34f2474da367a05636c8250855d6569b)
- Rebirth MeasureBars option was moved to the ThemeConfig internally, which means that setting is reset for everyone - [298bec9](../../../commit/298bec9f894fab1b354651e112682edf7989e35c) [34986b9](../../../commit/34986b9bfd99a6240e0328c502a5a39c6a477204)
- Rebirth Player Config is now loaded per profile. Profiles load from the old place in `/Save/Rebirth_settings/` unless not present, so no change is necessary for the user - [6ebb86a](../../../commit/6ebb86af507a5bf0a9a3c00fe9cc486028f7fff2) [2c53bd5](../../../commit/2c53bd5d042a768a999be7fde348ed8a417684c3)
- Rebirth Settings allows hovering the empty space between option names and option values to select them - [b96cdba](../../../commit/b96cdbadb9f7ceef719244326d1f7d63472122e9)
- Til Death Tooltip size reduced a bit so CDTitle hovers aren't huge - [495545f](../../../commit/495545f6d5c6736a8da134549b9be0bcd0184d76)

### Fixed
- Crashes by abusing Rebirth ColorConfig and CustomizeGameplay functionality - [603c0bc](../../../commit/603c0bc15405939bda5106656028310f6c331330)
- Rebirth Music Wheel should no longer randomly stop on files for 1 frame, causing a huge stutter - [232be6d](../../../commit/232be6dd2552ab1a93a50cb1cfd8a7ac19d401d1)
- Rebirth Song Search would appear to do nothing if a search returned a chart that appears on multiple songs, but one of the song results was filtered out - [5fb7850](../../../commit/5fb7850c0facc856a3e07687ddec2055aa31a47f)
- Replays with ghost taps before the beginning or ghost taps in large breaks, but close to incoming notes, would cause the rest of the replay to be offset and broken - [3f8f572](../../../commit/3f8f5728c82cd328ceec70b89dcee5e958fbf24a)
- ScreenTextEntry should no longer cause the music to stop playing, breaking the visualizer - [64d23ef](../../../commit/64d23efc643457b5a321d135843744d54be846bd) [5a71afe](../../../commit/5a71afed147b1f7ce0289e6984f86f8089de9e00)
- Tooltip overlay shouldn't produce so many errors when reloading scripts - [4918feb](../../../commit/4918feb1390ff4b40229893f4cab3200f6eb5e37)