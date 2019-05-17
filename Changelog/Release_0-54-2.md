# Release Changelog


## [0.54.2] - 2017-03-19 - Unreleased Iteration

Source only unofficial release.

### Added 
- Early support for Chord Cohesion Off - [599f08a](../../../commit/599f08a38cc9c2a0eaf7d1c08a4e3bbf02e8b977) [7de6738](../../../commit/7de6738db267fb34839f20da12ffa558ddfcbd57) [0b45e1f](../../../commit/0b45e1f4473fb39e87554c9b03c5381be394bc05) [e6024e2](../../../commit/e6024e2bb44cbdb97da0394ab6565d9eb535ea6c) [f5d6712](../../../commit/f5d6712b62b676a48f10aa8f86a835df1dff788a) [5759c88](../../../commit/5759c88ef1150f5b0724348fc4c2067a5a98ac2d) [0d2688c](../../../commit/0d2688c2dac45632c284b779a3454d17f84b422b) [569bf34](../../../commit/569bf34fed30ed4e24ba9cb49e45a65bf88b8571) [98302d3](../../../commit/98302d32081834ce02d22f118913bd1fd2399e7f) [64a7df1](../../../commit/64a7df183aa6d16ab0275bb5e11337aadbbafd81)
- Initial Customize Gameplay work. Movable Gameplay elements - [b77b548](../../../commit/b77b548333f3656583db22a19f757a0ec288df40) [0a42a98](../../../commit/0a42a980466ffde54aa5b147082d4280783c12a4) [d253366](../../../commit/d2533665a65b9d4ce3a7ac6751cf33270ff95627) [a6e4f83](../../../commit/a6e4f831c5481478fbbf3c879f04388d6b152703) [e977220](../../../commit/e9772202c45700d93c05dd026bc82224c02bd837) [d31d1ef](../../../commit/d31d1ef204b5c9b6dea2b447942635caa85ee051)
- Level system for Multiplayer only - [4086fbf](../../../commit/4086fbf9c2664899ef53a999ed81f2a934117be1) [c7f2221](../../../commit/c7f2221e65e7bca16c69c3a3040be4357729c95c) [801fb81](../../../commit/801fb815436e7ff47ceef48b67154f9e6e5addfb) [bcd0477](../../../commit/bcd04776c6ca28c75e037460b4a04e3968721462) [44aa0d2](../../../commit/44aa0d2588cae4348577ad94ba304cd0526b25d7) [78ff27d](../../../commit/78ff27df9e8435f891890142786e09b8c5650e0d)
### Changed
- CDTitles moved to a better position - [20816d3](../../../commit/20816d38034aabe89dba7977994d3f23a67de6c8)
- Don't clamp judging old Noterows to a certain window in the past, but still clamp the future - [3716264](../../../commit/3716264818878249de087d9ac73f242f60ea3af6)
- Don't garbage collect textures in Gameplay - [b988947](../../../commit/b9889476fc6f0ea84b0d4fe596505953b15d1bb4)
- Font change on installer image - [e83ed4e](../../../commit/e83ed4ee5b9a904c38bda6f30eea50f1a7a3c533)
- Generate Chartkeys slightly faster using fewer function calls - [db13407](../../../commit/db1340700c2cce920eb06b9fd751396898ed1cc5)
- Improve drawing polygon lines by calculating each line before drawing the necessary quads - [d3d2f66](../../../commit/d3d2f6605abf421d4bb78a2533fb77b7328f5587)
- Improve Predictive Frame Rendering by using C++11s std thread sleep since it works better - [3040671](../../../commit/3040671c5560c5caf09cc56a652d79793676eaff)
- Modify Multiplayer visually to support more aspect ratios - [7ccdb63](../../../commit/7ccdb63869c20d97161d624053ef56b435b7cabc) [40d1c9b](../../../commit/40d1c9b7c2c8d28fd30f30c7d1d1f71d82ad576c) [e9c891e](../../../commit/e9c891ee62d6eee08e67798a90f3efdd5442e5f0) [eeb2c4e](../../../commit/eeb2c4e44dc3cdbf9eeb521bc810181561b3c999)
- Moved Filters to a new FilterManager for organization - [758fda7](../../../commit/758fda7c4b8b8a1741ccbb10dd0607b9e635f1ad) [f4074a0](../../../commit/f4074a0a5aab66edca28e00e2450c836f89f89a9)
- MSD Calc short file downscaler is lightened up - [9a633cb](../../../commit/9a633cbcb561dd057b22b06cdd32d597266e0072)
- Only allow D3D multisampling if supported & don't round corners if SmoothLines is off - [df8a577](../../../commit/df8a5779d3d419fe444cddee5e2695c74c4c474f)
- Optimize difficulty calculation by serializing NoteData - [3e2509a](../../../commit/3e2509a801f0d8761282ecbea141b08081dcaaf4) [204c764](../../../commit/204c76449930841147cd969850bad1c2841c1810)
- Optimize bitmap text rendering to use 1 draw call for all characters of a texture page (FPS improvements for all renderers) - [0bf1a6a](../../../commit/0bf1a6ae613fcb8f30b05721ffa6691a14656147)
- Set DivideByZero to resolutions divisible by 16 - [f66406c](../../../commit/f66406c3196c2382a6413912ecfce6e8790ed493)
### Removed
- Game dependencies in the MSD Calc - [395736f](../../../commit/395736f260d2291596a2d159815bf6fb4dc71d68)
- Multiple Toasties - [e377aac](../../../commit/e377aac0f19f2768380d0ad05f724c7dd6c8f465)
- Unused theme files - [afa6375](../../../commit/afa63754015a877165dcbef8c8e57afae9c92a97)
### Fixed
- Crashed when using the mouse wheel after entering a song from Multiplayer lobbies - [17eb440](../../../commit/17eb4407d7d2762cda1c849c4b511ab97e4352aa)
- Mines hit in DP to Wife rescoring didn't get considered - [87e1a59](../../../commit/87e1a5992fbad403fa015903cb56f28448d8e135)
- Unicode Fonts loaded very wrong sometimes - [349f056](../../../commit/349f0561ca016f7840fbd2e3ed87e0d68a951fae)
