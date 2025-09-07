# Release Changelog


## [0.75.0] - 2025-00-00 - WIP

### Added
- Lua can use `TEXTUREMAN:RageTextureFromBase64(str)` to try to generate a texture to put on a sprite with a base64 data string - [b16b4c2](../../../commit/b16b4c2ee1fb13fc569eba5dc5590562f430a0fe)
- Lua hook for `SCREENMAN:GetOverlayScreens()` to have some sort of access to the overlay screens instead of none at all - [f9694c0](../../../commit/f9694c0b5e5d2a2942452face340b30ef65a2252)
- Pack banners loaded from online API can be shown ingame - [78bf78c](../../../commit/78bf78cb64149e3d32d5e77dcc2ff23ae3cd503e)
- SetPage Lua hook added for pack download paginations - [647659d](../../../commit/647659d49aff971899b9d2cda632935d17a2f0d0)
- Sprite Lua hooks related to CustomTexCoords and CustomPosCoords - [081a681](../../../commit/081a681ca387ef5ef0b01df07e7a2f9d18253e9a) [29fb334](../../../commit/29fb334ee6a88a8797b58e497bfb0789f30f510f) [1c012f3](../../../commit/1c012f35f941bc36bd1acac0d2b68965596c994a)

### Changes
- Banners on the preinstalled themes now support spritesheet animation - [8d9d8e9](../../../commit/8d9d8e9a27fc91adcf1d0e4c042c0637deae5967)
- Mac might properly prompt for input permissions on some platforms like 10.15 - [c45d547](../../../commit/c45d547f5f6e03254d8e82507069fbace9e53b68)
- Moved to OpenSSL 3.5.x - [d3dd7de](../../../commit/d3dd7de8c5d3cf50d6c4c1018a2e38c955ecbcfc) [0414f3c](../../../commit/0414f3c2908aebcb8395e1e3309c47bbd1a9671d)
- Libcurl updated to 8.13.0 - [6a10dd4](../../../commit/6a10dd4b5e5d7017df7a7ea99dc9ceac9b7f2997)
- stb_image error messages improved - [ac8bc5f](../../../commit/ac8bc5f0ec38967634a91b5dadc8d8fa36692aa0)
- Sync changes of any kind in Gameplay will prevent saving the score - [e11c1c3](../../../commit/e11c1c30639d686b146532bd79bd525d90555bbc)

### Removed
- Unused GLES2 implementation - [6b99cf4](../../../commit/6b99cf4134491266b4e037ccd571136e3bcad926)

### Fixed
- Calc debug assert - [164bd55](../../../commit/164bd55187fd7e29c978432bd4982c39dfe1c795)
- Linux input devices (obscure) might load now - [2f1b598](../../../commit/2f1b5984c0be2f109e0c9b23cd958b4abb971338)
- Til' Death Lua errors in SelectMusic - [ceb88aa](../../../commit/ceb88aac4866f9bfed17a5f2fac008d9077a48ee)
- TotalHolds in the Etterna.xml now functions and represents all Holds and Rolls hit for all scores. To find the amount of Holds, subtract the TotalRolls - [e330b32](../../../commit/e330b32bd565fe48a7129202fa3d78ec1dbf4b1d)