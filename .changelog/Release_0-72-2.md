# Release Changelog


## [0.72.2] - 2023-03-01 - Hotfix

### Changes
- Network Options is hidden from the Rebirth menus because it isn't functional right now - [f52f4b4](../../../commit/f52f4b4d34f2474da367a05636c8250855d6569b)

### Fixed
- Rebirth Song Search would appear to do nothing if a search returned a chart that appears on multiple songs, but one of the song results was filtered out - [5fb7850](../../../commit/5fb7850c0facc856a3e07687ddec2055aa31a47f)
- Replays with ghost taps before the beginning or ghost taps in large breaks, but close to incoming notes, would cause the rest of the replay to be offset and broken - [3f8f572](../../../commit/3f8f5728c82cd328ceec70b89dcee5e958fbf24a)