# Release Changelog


## [0.01.0] - 2016-08-26 - Early Experimental Release

Binary only release to replace Stepmania.exe. Initial branch off of [Stepmania 5.0.12](../../../tree/v5.0.12)

### Changed
- Replace BeatToNoteRowNotRounded with BeatToNoteRow & typecast instead of round in BeatToNoteRow, because rounding is slow - [eed2f6e](../../../commit/eed2f6e7c2ebb36af7b31b3d1cc4ba5992a88ba0) 
- Set the start of songs to be constant, independent of playback rate - [63c5efe](../../../commit/63c5efe778efc7c853c0636641e9e7d5c1570d2e)
- Try to allow VSync off using Aero in D3D - [d51205b](../../../commit/d51205b174ced006aace4ac9a7d44affa0bfe872)
### Removed
- 1ms sleep in the Frame Limiter on Windows - [e490364](../../../commit/e4903649377257957728e907f313705dc4f18858)
- Groove Radar Calculations (nobody uses them) - [fa53caf](../../../commit/fa53cafb80ee8f450cad4baf6fbcc0d2156d71aa)
- Multithreaded D3D (for FPS) - [e0f4d7c](../../../commit/e0f4d7c43c649f3f83d703f35779a0ff53553ba6)
### Fixed
- FPS dropped in gameplay due to large number of bpm changes throughout the file - [e3e3460](../../../commit/e3e346075f6411b648eed7b8fdf940287333a855)
- FPS dropped when exiting gameplay due to unnecessary looping -  [a7ca8c7](../../../commit/a7ca8c7a5ec955430cd3fa55f056ba408bffa10f)
- Rate System with Pitch was put back together. It was previously removed by the SM5 devs - [b5f7cc7](../../../commit/b5f7cc7707a5735bece3498ea2aac822ec484699)
