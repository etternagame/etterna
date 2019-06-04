# Release Changelog


## [0.54.0] - 2017-01-24 - Official v0.54 Release

Windows installer release.

### Added
- Early unreleased Etterna Multiplayer Protocol - [d54fb47](../../../commit/d54fb4798ac78b106ba0315215912fdd8e338c3b) [3dcd1bd](../../../commit/3dcd1bdd7fec749af3736d7f96ffd721c5c01d54)
- Judge Conversion Button on Evaluation screen - [7544862](../../../commit/7544862422db1585a1685ebfcc1ff8e8863d8ee2) [6d817de](../../../commit/6d817ded4cf54163417e39e9d1f385b9c08f0958) [c2f6311](../../../commit/c2f631136befdfa9c910aeb1ae4679631273f7d6)
- New Etterna installer icons [35de48f](../../../commit/35de48f9a9ed34c45c64d7cae1264f0386ef3b97)
- Save Profile button in the Profile tab - [9f8c9a7](../../../commit/9f8c9a7caf1fc763bcee630fc75f21373da4b8ee)
### Changed
- Calculating SSRs is specific instead of calculating every single SSR for no reason - [de405af](../../../commit/de405afb373bbe9f77d7937d8eea560e6c37cb46)
- Optimize offset plot loading - [fce2e49](../../../commit/fce2e494f57ca960382ab0ca2e9aa313d3d57fd1) [a83ab16](../../../commit/a83ab166e484e72a079e20c7b72305d7222c56bd)
- Profile Tab properly shows nothing for Steps of invalid entries - [253d009](../../../commit/253d009608db207c708e34351272dc98686d0ef1)
- Rates up to 3x are supported by TopSSRs - [f26235e](../../../commit/f26235e7f6d1988ea2c59f67101efc4d14232ce8) 
- Use std functionality instead of OpenMP for calculating Non Empty Row Vectors - [5a1a217](../../../commit/5a1a21725cf0163b38de70e3c2f20ac8c0cb9e0a)
### Removed
- Old SM5 installer icons [b921cc3](../../../commit/b921cc302deb58d8884d7bd6fa68c0e50a1bc427) [1c408a3](../../../commit/1c408a375c99b1552325f7437184d11dcb4f5601)
- OpenMP - [5a1a217](../../../commit/5a1a21725cf0163b38de70e3c2f20ac8c0cb9e0a)
- Theme Version Display - [35c8889](../../../commit/35c8889a5707d0e11f409be2b2bf7867646256c0)
### Fixed
- Filter tab elements could be clicked in impossible ways - [7747210](../../../commit/7747210461cee54d424441d13e9150f9d983541a)
- Scrolling while opening the Search Tab was sticky - [8b45309](../../../commit/8b453098175fae6ffac1365a771b8a414c7e2d67)
