# Release Changelog


## [0.54.0] - 2017-01-12 - Pre-release: More Specific Skillsets

Windows installer pre-release.

### Added
- Appveyor & Travis - [a73fa11](../../../commit/a73fa1109c0f597d95b1e48d1cb2088d80d0b387)
- DivideByZero Noteskin Variants
  - Bar version of the DivideByZero Noteskin - [44b90bc](../../../commit/44b90bc685cbf7acc9131bb344e2ad40b73b2076)
  - Halved version - [c6bd0cc](../../../commit/c6bd0cc4202666535898088098845b0c18c81dd4)
  - Orb version - [44a37a7](../../../commit/44a37a7a511e637e51cab60ab42bc572285a52bd) [44b90bc](../../../commit/44b90bc685cbf7acc9131bb344e2ad40b73b2076)
  - Semihalved version - [7e9ed57](../../../commit/7e9ed571202008f586ef4f97c99aea3a22d9deda)
  - DivideByInf (192nd coloring) - [f2a7449](../../../commit/f2a744948c056d4d0f80ff67f0664d682c06c280)
- Invalidating scores from ingame via the Profile tab - [0a364c1](../../../commit/0a364c14d52b7863ded223c924a487bbb72d4c2d)
- Preference for Horizontal Scaling of NoteField - [c16d57d](../../../commit/c16d57dc784fcb198cb0f35e13ab0a0e8fc79065)
- Profile Page shows top scores of each Skillset - [75f5d50](../../../commit/75f5d50ad8245abfafa64babf7b41e465550ac67) [7238dc7](../../../commit/7238dc7571144f7feab2ff0a87e553b9b5e816c2) [f0468f5](../../../commit/f0468f599163de5039b6ce6f8bfe2bb90a753c3e) [3ce8baa](../../../commit/3ce8baac632f4b5cbc556b9cc9f9e8d572256bc5)
- Skillset categories are split into more specific ones
  - New Technical Skillset - [1253f09](../../../commit/1253f094c08fc77cd2422b1600c53e737c2e73c5) [58bb603](../../../commit/58bb603b0dde34184f70378acb21a2531147aeec) [f2b8a6c](../../../commit/f2b8a6c99c2574941f92724d89c6c5fd907b4a09) [57390b6](../../../commit/57390b62ab89b901e627467201412e43aabdcefd) [86ebd28](../../../commit/86ebd288474aa49d758159f0043679354d674283)
  - Jack is now JackSpeed & JackStamina - [854dc2a](../../../commit/854dc2a418691b8b10ce773d86d51fed72fff917)
  - Speed is now Stream, Jumpstream & Handstream - [854dc2a](../../../commit/854dc2a418691b8b10ce773d86d51fed72fff917)
- Song rates of 0.05x are allowed - [71ada86](../../../commit/71ada86739b8cf215fac46c744d279664b9a8b5c)
- Tangent Tectonics by Sinewave as a new "official" Etterna song file - [df8b6dd](../../../commit/df8b6dde151e05f2cdcca43db7c317f0255b31bf) 
### Changed
- Basic cleanup of MusicWheel messages - [04afbb7](../../../commit/04afbb766886e3c7342d6c5f1c91acce0ae10141)
- Heavily improved Multiplayer visuals and playability, to be brought more in line with offline play - [fe2bd47](../../../commit/fe2bd473784f50e32389d6b17d1deaf76b1146d8) [0bc44b5](../../../commit/0bc44b5d06dd8d41fabfa1677c25e143a4b19642) [aa10d39](../../../commit/aa10d39c792254989a868c3a95bbb3e2ab490a09) [8d12f9c](../../../commit/8d12f9ccfc8d530542556b649c553bff0e192594) [9360c18](../../../commit/9360c188c8974c10dfc4a3a2e60eddd83e3aed1e) [11e4ff0](../../../commit/11e4ff0eabde24a97249151642bef3452b5e58a0) [a0b2355](../../../commit/a0b23551889de73d11e0e3b183022f0dde0a0e33) [eaca47a](../../../commit/eaca47a2220882246f562027c0472405dfc848cf) [aa8383e](../../../commit/aa8383e170a4419262306613b42f0a2fb9501394) [3c12c85](../../../commit/3c12c855b16482f92ff80d308d781bed3741174f) [431ef6b](../../../commit/431ef6b4028a6e5a106c35a8b5cc3cf6868bf8db) [ffe1856](../../../commit/ffe18561815b2a8201a7711778ce7b51e3077fd0) [545f29a](../../../commit/545f29a550503d9a185cb98a4db44aa498b700bb) [753b2bd](../../../commit/753b2bd50591e37c58151d61e6c3af6611fab80b)
- Improve cache space usage by the calc - [a7e0be4](../../../commit/a7e0be41b1667e49d8e99817ade92519323d5cbd)
- Lua has access to Wife & Wife is more hardcoded - [5ee64fa](../../../commit/5ee64fa06b08393f24bb9623f7f6c4b0618d991b) [45f497e](../../../commit/45f497ea176dfe3717bac7b1c197475e67631ede)
- Many MSD calc updates - [c8a7c79](../../../commit/c8a7c79563a526805b81ab6df52d22b6db80aca2) [d3b6c5d](../../../commit/d3b6c5db3933975b78e65728a0ea41a30f943e83) [e0bbddd](../../../commit/e0bbddd3c0ca0ecd8e7f8792eb1c9a27a5e51325)
- MSD interpolates for non-standard rates (non 0.1x intervals) - [44a80fe](../../../commit/44a80fe662160af2724f9dbce68a3afb82548fe6)
- Note Per Second vector generator re-enabled - [fc51138](../../../commit/fc5113849066669dca22a64268300c58b10656e8)
- Noteskin Player Option row is always 1 option at a time to prevent confusion - [a2e5646](../../../commit/a2e5646fd7cf5d8b814f2e98e0c66777151aff05)
- Player Options slightly reorganized to be chronologically ordered - [f0fe666](../../../commit/f0fe666e249758f332e93eea47647e9551acc283)
- Skillsets are more generalized internally - [a625318](../../../commit/a625318b6b61a805482ba57379abb9e7a10505a7) [df526ed](../../../commit/df526ed9399504fbacfb841df0e037fea80203c3)
- SSRs are not calculated for scores under 0.1% - [6f83788](../../../commit/6f837887594bead76392018dc864922b4056f3c3)
### Removed
- IRC Notifications from Travis - [d9bd3f3](../../../commit/d9bd3f3b01f76da959381011fbc7df83c6453d7e)
- IRC Notifications from Appveyor - [7c11e2b](../../../commit/7c11e2b4588a486e076b26c14bbd2f62c85448a3)
### Fixed
- Crashed when moving to a negative location on the Profile page - [4722f33](../../../commit/4722f33e95eb4fe2d6a2ead4996909964d3aa87b)
- GCC had a compile error due to non-const rvalue - [ba1e734](../../../commit/ba1e734f628e97f080f6b8b2bfc23496225d1473)
- Lifts were not considered at all by the calc - [0785474](../../../commit/0785474a34a784492dc9bff39eed14c0f3068684)
- Non-dance-single scores sometimes gave weird SSR values - [587b55e](../../../commit/587b55e9a55cec51efa09698ea17fa1431612c6a)
- Parentheses placement for 1x and 2x rates on Scores was bad - [15c4faa](../../../commit/15c4faa2bdab741279ec52cece45e4cec56bf3d1)
- Selecting a song from Lua didn't select a song - [ac955d7](../../../commit/ac955d7acc1b49bfd6bfa68d6964fbd8df476df4)
- Window icon was giant - [dead101](../../../commit/dead101d49beba0158d6b3740191585a4b55bd3f)