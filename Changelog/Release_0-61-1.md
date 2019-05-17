# Release Changelog


## [0.61.1] - 2018-10-17 - 64bit D3D Crash Hotfix

Windows x64 installer release only.

### Fixed
- CMake does not load the correct version of d3d9 - [396d856](../../../commit/396d85601c6844b17dca5d0ccf91202397248a5c)
- Saving a pointer as an unsigned integer in fails due to increased pointer size on 64 bit - [b984fe4](../../../commit/b984fe493d9f7ac84a35af3e6f80f16607aceb09)
