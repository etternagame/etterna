# Release Changelog


## [0.56.2] - 2017-11-05 - Pre-release: Calculator Updates required for EtternaOnline

Windows, Mac, and Linux pre-release. The MSD Calculator is receiving heavy chiseling.

### Added
- TopScore flag introduced for HighScores for identifying Top Scores properly for EtternaOnline parsing - [d518aed](../../../commit/d518aed9f83ae3a59710d1cb9605feb18a1a9171) [5df2fc0](../../../commit/5df2fc054545ec13a16a78fd25c2349cfd543e78)
### Changed
- The Calculator has been updated - [dd1ed41](../../../commit/dd1ed41ebda7617e2f354c2d45c8b1310853645b) [7e0bcc7](../../../commit/7e0bcc772ce9290247ca47fd9d58bf48a408d2da) [0848477](../../../commit/08484777b7277805959f9d58ce210ff638dfd3d3)
  - The SSR cap is now 97% instead of 96%.
  - The distribution of all Stamina files is a bit more even.
  - Stamina associated with Stream or JS/HS is better rated.
- Calculating the Player Rating also sets Top Scores - [ff9bee9](../../../commit/ff9bee9518cfae946407fff97fe756e3f84d522f)
- Only TopScores are considered in Player Rating Calculation - [3cd733a](../../../commit/3cd733aa59351d0da323a24e9312f2c99055c313)
- Player Rating Calculation ignores CC On scores, is a flat average of skillsets, and had the internal algorithm slightly changed - [c7cc786](../../../commit/c7cc7865c0bdcad3ba82bff66840ca84f0c984d9) [bb6dfc5](../../../commit/bb6dfc5cc1d8a48814c083862c649a4f6798f521)
- TopScores should not be read from the XML, and always calculated - [f2216ac](../../../commit/f2216ac716d2911432c59df0253515df52a21759)
