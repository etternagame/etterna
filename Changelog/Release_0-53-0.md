# Release Changelog


## [0.53.0] - 2016-12-24 - Skillset Ratings

Windows installer release.

### Added
- Colors based on MSD - [f712c4f](../../../commit/f712c4f74be55251d960eb676b33fc235d626488)
- Colors based on file length - [e87a356](../../../commit/e87a356304bf8003335f8d6663a5e291eaea69dd)
- Number of Favorites placed in bottom right corner - [630e753](../../../commit/630e753b8724a223ea384f7fd5afd657488b007a)
- Session Timer - [81b47c3](../../../commit/81b47c3bdde437f181c7b4b1a5132629f20231dd) [2b6f8ee](../../../commit/2b6f8eed8cf4b604c2a5b74ea2b93080e3ff03d5)
- Skillset Specific Difficulties & Ratings - [39559e0](../../../commit/39559e0e3a5ddae60efbf7e7af09325fcefca66f) [d4f2b04](../../../commit/d4f2b040890f244f00d0f9a5f61d3880586df27c) [3551116](../../../commit/3551116ac4ab3ba5c11d0041f8215cfb1322cf75) [104edea](../../../commit/104edea8e011512d5800c78374d254ca160d7097) 
- Toasty - [8305648](../../../commit/8305648f26b6c27183e8d3d8852327b4ba969fe2)
- "Worst Case" DP to Wife conversion - [07be19f](../../../commit/07be19fdba343777227d78800a97a1fd3c4cb7b7) [d8520f0](../../../commit/d8520f02a4ce0b497a01d0eb48c31ed5dcba75ce) [5343373](../../../commit/53433736176ac10df6cd9305feae4809addf025b)
### Changed
- Autoplay scores intentionally score bad according to Wife - [a4ef6b6](../../../commit/a4ef6b6eecf8a11c6a7031389040e3fbc03da883)
- BeatToNoteRow rounds again instead of typecasting - [f719a5a](../../../commit/f719a5a4c0e43beee15bf768e843def338699e4f)
- Centered Player 1 preference will move Gameplay elements - [cf1ea4c](../../../commit/cf1ea4c524d2bc5ecc08193abb6f22bd5505968e)
- Color Config allows MenuDirection input - [03f8947](../../../commit/03f8947ebf02ab4bfafb35eafd1ec972ac60bbeb)
- Don't delay sample music playing - [bf8419a](../../../commit/bf8419ab38903064af50a722ed2c1588e263fa68)
- Hold Enter to Give Up displays 0.2 seconds slower - [9f86da0](../../../commit/9f86da0e2a9d71f758f19854a4d05c7e2c2caa60)
- Invalid scores should not be best scores - [8f66d61](../../../commit/8f66d61c7932b9b11e19ff1c93444a474487f4a2)
- New Loading Splash & Window Icon - [2352dac](../../../commit/2352dac6cd31bac18ef84b562f9e5aa4292ec0ec) [9701017](../../../commit/9701017fde218493a918e0a9ca8cdc28326dadd2)
- Target Tracker properly interacts with set percents or personal bests - [9d65ca9](../../../commit/9d65ca98373fcabac76b329f70829cbd708e7d17) [5a8ad7a](../../../commit/5a8ad7a31582edea0cfbd4c4fdf03e83445f6781)
- Sort HighScores by rate forcibly when grabbing them for Lua - [568ba80](../../../commit/568ba8058296164e1d99df023a8dd27fd8b3ebc7)
- SSR recalculation is only done to loaded songs - [bb2d5a5](../../../commit/bb2d5a55879ad085c1e357dc940e1d970a0cac0f)
- SSR recalculation is handled in a better way to allow less hardcoding for recalculating - [d8a337d](../../../commit/d8a337d17df707d9803f942fc1523b0667733dc9)
### Removed
- Text Optimization when not on D3D - [22b940b](../../../commit/22b940b3e42c2531985a9c75ee93e41f4572a7ac)
- Various unused Lua Scripts files - [705612a](../../../commit/705612ab9790961203b5d45bf72858111dca9f10) [afca92e](../../../commit/afca92e9fa05884aff6db962edce941ad7cc1ea9)
### Fixed
- Background reloaded for no reason when switching theme tabs - [321092d](../../../commit/321092d8b024cf3dd800edcc5951b1af3d6ff3d7)
- D3D Render States didn't function - [0394d35](../../../commit/0394d35997365267f0a6055c38ac144706f2a1a6)
- Delayed texture deletion delayed infinitely - [6ac403d](../../../commit/6ac403d642262e49e7a46c8e3877f11116cae05a)
- Display BPMs was not showing proper BPM when on rates - [1dddd9c](../../../commit/1dddd9c1ba8ae851b253dc50d863fb51f861f9fa)
- Edit Mode crashed upon Hold judgments - [294b1cb](../../../commit/294b1cbffcc3353cefad3f2314f70b7e01e7c406)
- OpenGL/D3D text rendering was broken to some extent. Pointers were nonsense, and some logic was broken - [59c2b7b](../../../commit/59c2b7b2100ce68f0ee1524a9f6495257660d827)
- Multiplayer was visually broken. Redid almost everything - [0e3fa51](../../../commit/0e3fa51e142bbc273c56fcebb7b484b8980cf742) [0e3fa51](../../../commit/0e3fa51e142bbc273c56fcebb7b484b8980cf742) [2be527c](../../../commit/2be527cd570f6338fa0724d095964c67d0b54b38) [6d4a71b](../../../commit/6d4a71b12aed7457b78b534c70866f4052f1cfe2) [d21a97d](../../../commit/d21a97daf290b9fc593dd726d3b1dd25a01d81d5) [ee807aa](../../../commit/ee807aa4084e5afdc12bbb718fc92c143ef5bb8f)
- Some Evaluation elements were badly placed in widescreen - [845afd7](../../../commit/845afd7216c3283e835b9c49c2312f50109c06c8)
- Theme options did not properly place the Progress Bar - [b21018c](../../../commit/b21018ca372123466741a69b10c0f2dbae8fc9e1)
- Wife & DP rescorer didn't function on Score Tab - [2d0174e](../../../commit/2d0174e54626145ea01ae8ee463f7571727fa7ba)
