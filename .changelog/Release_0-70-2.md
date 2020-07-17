# Release Changelog


## [0.70.2] - 2020-07-13 - MSD Update

### Changed
- MinaCalc updates (to Jacks & JS primarily) - [74eed96](../../../commit/74eed96cb0f4f0575c5973bf2b263d0e138fc70d) [0fa0117](../../../commit/0fa0117fdee473a5ca93438581d82e3927ee68b2) [b3f25ad](../../../commit/b3f25adcaa3749add7cbd12c24015fd535e4f2d7) [92518e5](../../../commit/92518e5fa00832ec8afb9eae929ab441dc68b753) [82b67dd](../../../commit/82b67ddb8ccec93f526020bffbd59bff1b46b24d) [6a5a06d](../../../commit/6a5a06d2bc7680580eae6ea9792c6467e64cf409) [79797de](../../../commit/79797dea45b853ebc7ebdd4ba995e821bc98addc)
- Profile will now save after score upload but before reaching Evaluation - [c62833f](../../../commit/c62833ff2a695594d57ece1e976465e234bd05e4) [be84d66](../../../commit/be84d669bbfcbbdf140dead164082c3c357f1d1c)

### Removed
- PlayMode - [3b966eb](../../../commit/3b966ebe82f8576f7c2bc0a6e0f6303efed26d2e)

### Fixed
- Crash on sortorder iteration via metrics - [79defd0](../../../commit/79defd01be938862de6afc140186dcf48a4c671a)
- Filtering will actually remove all charts that don't match - [e6b1a44](../../../commit/e6b1a44af626c0578d06c303dc122b3971e74b81) [adeb392](../../../commit/adeb392fcff5c598a186b8c82875dbfe76633b61) [c52443b](../../../commit/c52443b3b674069729495eee740431a4710f7be5)
- Highest-Only filtering will properly filter strictly to the Highest-Only charts - [#833](../../../pull/833) [e6b1a44](../../../commit/e6b1a44af626c0578d06c303dc122b3971e74b81)
- Linux crashing due to muFFT visualizer usage - [#837](../../../pull/837)
- Profile Tab scores set rates even if the song was not present - [8119385](../../../commit/811938555cc166ccbd077d7d072da75799d9737b)
- Sortmode buttons suddenly centered themselves - [fa81da6](../../../commit/fa81da68f5780253b4aaa9bb22d352ea9f199c95)
- Top Grade sort breaks for D and F grades - [337277f](../../../commit/337277fa4f8d57519e4ff3aeb5051f93a35dbf62)