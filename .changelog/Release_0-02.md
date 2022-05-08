# Release Changelog


## [0.02.0] - 2016-08-31 - Early Experimental FPS Improvements

Binary only release to replace Stepmania.exe.

### Added
- Usable but hidden Jumpstream generators - [c5b3d03](../../../commit/c5b3d031c021d8de0015dc51d9156f0cfe63f8b4)
### Changed
- For Windows, use D3DX instead of RageMath for some calculations - [2d4c053](../../../commit/2d4c0538a683da87c0a09ac68304f953b04542fa)
- FPS with Holds on screen improved by 37%
  - Don't draw the Tap part of a hold within DrawHoldPart - [3ba8bd5](../../../commit/3ba8bd503a207e285880b199d30887d9b74ade01)
  - Don't glow the Hold until it should glow - [3ba8bd5](../../../commit/3ba8bd503a207e285880b199d30887d9b74ade01)
  - Don't draw the Hold body head if the head is past the tail - [434eaa8](../../../commit/434eaa846f5e8b43b139589132ec26a37d335cf0) 
- Reduce the internally bloated nature of the LifeMeterBar & StreamDisplay (for FPS) - [6b05c2b](../../../commit/6b05c2b89ee2bcbb6ec6fabce09a3e953a0c09e7) [a2129a3](../../../commit/a2129a39e9747e2bcf5d2a8b5f5e0a2017693352) [bbd0be0](../../../commit/bbd0be03edffc47e7a926bbe12f134c6e82ec535)
- Replace nearly all lrintf calls with lround - [a559386](../../../commit/a5593868c478a1b6c66b628f28805190818f8bb3)
- Truncate instead of round in a critical "float to char" function - [2faa10b](../../../commit/2faa10b023a66745fe23aa0a8aed96fd0341264a)
### Removed
- The need for Windows Aero for VSync Off in D3D - [8e0a94f](../../../commit/8e0a94f0c7806a850d84df65750189a8d5e95ef7)
- Unnecessary check for judgeable NoteRows - [d412cbf](../../../commit/d412cbf23aebe6f464a48aec7b9b6f5bf1795524)