## [0.55.1] - 2017-09-18

As always backup your profile. Backup any theme files you have significantly edited as they will be overwritten. You are advised to clear your song cache however this should *technically* not be necessary.

### Fixed
- Song options supports the full compliment of rate mods up to 3x.
- Profiles will no longer crash when attempting to load an empty playlist.
- Non group sort orders now use old code again and should operate as they did in 0.54 or vanilla.
- Mines no longer count towards NPS for the graph or text indicator.
- Scores incorrectly flagged as ccon will be updated when their SSRs are recalculated as triggered by calc updates. This won't apply to fail or incomplete scores since it checks note counts. This will also flag legacy scores obtained on files with no chords as valid, since chord cohesion is meaningless in that context. 
- "Can't sync in course mode" has been fixed for the fkey gameplay sync hotkeys.
- Autosync and manual sync will no longer delete files and crash your game when you try it.
- Calc normalizes song offset now and shouldn't produce differing values for the same file if the offset is changed. No this doesn't affect the hashing and never has stop asking.

### Changed
- Various product id and game identification strings have been updated.
- Some updates to the save format to facilitate parsing by EO.

### Known Issue
- kinda broke cold warriors theme oops dont play it

## [0.55] - 2017-09-07

***This is mostly complete however the changelog spans many months and some changes may be missing***

***Warning:*** A lot of under the hood cleanup will take place when first running 0.55. Your song cache will be rebuilt your profile will be converted and all SSR values for the newly imported scores will be recalculated along with a number of other operations. You are **strongly advised** to avoid interrupting these processes and to **save your profile** when they are complete. Interrupting the process may trick the client into thinking some processes have been completed when they haven't and you'll have to manually delete the song cache and start over. And also apologize.

### Community Site
- The open beta of our community site is now live at https://www.etternaonline.com.

### Added
- Chord cohesion is now disabled by default. If you don't know what that means or why it's important or how it impacts you, it doesn't. Chord cohesion can be re-enabled by setting disablechordcohesion to 0 in the `preferences.ini` file, however if you want your scores to be eligible for community score tracking sites this is not advised. Fake scores!
- Differential reload feature has been added to the game. At music select pressing <kbd>CTRL</kbd>+<kbd>Q</kbd> will reload any and only new songs or packs that have been placed in the songs folder since the game has been started. You are strongly advised to use this in place of the options reload function during the course of normal gameplay.
- An in-game goal tracker has been added.  At music select pressing <kbd>CTRL</kbd>+<kbd>G</kbd> will add a scoregoal for the currently selected chart and rate with a default to 93%. This can be changed in the goal tracker tab interface. Now you can stop using this after a day instead of that spreadsheet!
- The x/y positions and scale of all gameplay screen elements are now stored as player preferences and are now customizable directly via the gameplay screen if the customize gameplay player option is enabled. The default positions for standard display ratios should remain unchanged. Pixel perfection can be achieved by directly editing the values which are now stored in the `playerconfig.lua` file in your save folder. Autoplay must be enabled in order for this to function properly.
- The entire save format has been reorganized to follow the chartkey system and much of the code that interacts with saving and loading of profiles has been written from scratch. An existing profile will be converted automatically and the next time you save an `etterna.xml` file will be generated in the `Save` folder. The old-style save format is treated as read only and you may remove it or leave it in at your discretion. The score import function tends to be *super lenient* so there are certain cases (disregard lookin at u) where scores may be improperly assigned to the wrong chart. Scout's honor on deleting/disabling those scores if they effect an undue influence on your rating.
- Full cross-platform support has been (sort of mostly) re-achieved.
- Rates up to 3x may be set at music select. Other features that interact with rates have had their limits expanded to include up to 3x.
- <kbd>CTRL</kbd>+<kbd>M</kbd> with a song selected on the music wheel will tag it for permamirror. Permamirror charts will always have the mirror mod set for them when you play them. The mirror mod is activated after initiating gameplay and deactivated after exiting gameplay and should not unexpectedly carry over to another song selection.
- Added playlists. Scruffy says they do things. Mhm.

### Changed
- Song caching speed has been improved by 30%.
- Text is now rendered in one draw call for each segment of a single language, rather than up to only up to 8 characters. Should see on average 35-50% increased fps on screens with lots of long text lines.
- Text performance optimizations are now compatible with OpenGL.
- Enabling SmoothLines now enables antialiasing on D3D. This may also decrease FPS by up to 20%. If you are consistently hitting late during gameplay compared to 0.54.1 and you don't have the NPS graph enabled/don't utilize this in any way, you should turn this off.
- D3D will now render LineStrips in one draw call if SmoothLines is enabled. This mixed with the improvement above means that rendering the NPS graph no longer has a massive FPS hit, while looking pretty good.
- Evaluation screen's judgment plot now uses one MultiVertex actor instead of one per judgment. This removes essentially all of the frame rate hit of rendering the judgment plot.
- PredictiveRendering has been changed to sleep until the correct time.
- The lua call `GetPercentDP` now actually gets the percent of total dance points, rather than MIGS.
- Lots of theme changes.
- Rather than mindlessly and infinitely iterating gameplay stats in a profile when a score is achieved or a profile is merged those stats are calculated directly from the existent scores in the profile. The exceptions to this are those stats for which this cannot be done, such as toasty count. Which are stupid and useless anyway.
- The replay storage directory has been moved up in the folder hierarchy to be independent of profiles. Replays are currently not automatically transferred from the old storage folders within profile folders to the new directory, however this is functionally irrelevant as at the moment the ability to load the data for use is disabled pending resolution of crash issues.
- You can save your profile directly at the music select screen after manipulating it (or I guess even if you don't change it) by using the shortcut crtl+s (surprise!).
- The shortcut to unfavorite is no longer <kbd>CTRL</kbd>+<kbd>U</kbd>, <kbd>CTRL</kbd>+<kbd>F</kbd> now simply toggles favorited status.
- `MinPercentageForMachineSongHighScore` has been renamed to `MinPercentToSaveScores`. Setting this value to 0.1 will not save any score below 10%. Setting this value to 1 will not save any score. The default value is -1. This doesn't mean everything will be saved, it means anything under -100% will not be saved.
- The printscreen and shift printscreen functionalities have been swapped. Printscreen now takes full resolution screenshots while shift printscreen will take locked 640x480 screenshots. You know, the way it should have been all along.
- The MAD MP3 decoder was replaced by an FFmpeg MP3 decoder. This was done because FFmpeg is licensed under LGPL which is more permissive than GPL.


### Balance Changes (snicker)
- SSR values are now capped at 96% instead of 97%.
- Various lots of calc changes that can't be shown to be an improvement or detriment on an overall scale in any way.

### Removed
A few dozen thousand lines of code have been removed that pertain to elements of the game that are being phased out or replaced- namely arcade cabinet and pad oriented 'features'. Also support. That is, arcade cabinets and pad stuff is now entirely unsupported. There's enough here to warrant its own section. Most of the code pertaining to the following have been removed or rendered inert:
- Arcade lights drivers.
- USB stick drivers and code relating to saving/loading profiles to/from them.
- Courses. Courses are being replaced with playlists which will serve the same functionality and more without requiring 20k lines of code. The implementation is not finalized yet. The interface for constructing them is admittedly clunky at best and playing a playlist as a course will not properly aggregate at the evaluation screen. However the 'run' of the course will be properly documented and one of the goals is to provide a feature that reconstructs an eval screen for an existing score if the appropriate information is available. Thus you will be able to play through a playlist as a course and reconstruct the proper evaluation screen at a later time.
- Several gamemodes, including the lights gamemode, the kickbox gamemode, the spinning teacup gamemode, and most unfortunately the perpetual worship of cthulu gamemode.
- Anything involving calories.
- Attacks.
- "Edits". This doesn't mean edit difficulty. It means "edits". Because you know, why would anyone want to be clear on that sort of thing. Not that you need any clarity because nobody used the other thing anyway.
- The editor.
- Preferences related to the above subjects (and below if I think of more things that were gutted and don't want to move this line).
- Nonstop, endless, whatever all stage mode stuff. There's only event mode now. To appropriate a comment from the codebase "this is meant for home use, who cares how many songs you get to play before the game kicks you out".
- The lifebar types that nobody uses or cares about. So, battery and whatever the other one was, if there was another one, I don't remember.
- The unlock manager. Also by association, the capacity to lock content and the need to check whether or not it is locked. Also the locked sort order, if it existed. Let's say it did. You can't prove otherwise.
- Roulette sort order. Also a bunch of others. Not that you'll miss them. If you do I'd advise seeking counsel on your ability to form emotional attachments to things you don't need in video games.
- Almost everything to do with coins. This isn't stepmario. It's not even stepmania. (haha just kidding all we did was change some compiler flags xD)
- Anything to do with items, also battle mode. And Rave mode. Also mothra mode.
- The screen saver. What screen saver you say? Yeah.
- Symlinks.
- **Monkeyinput.**
- Cancer. I mean autogen.
- Traditional "radar values". Chaos/freeze etc etc.
- Old themes are no longer fully supported for a variety of reasons. This doesn't necessarily mean they'll vomit errors or blow up in your face, however behavior involving profile save and load and a few other things will be erratic if not entirely ineffectual. Effort is being made to minimize the amount of effort required to port older themes.
- Multiple toasties are gone, good riddance. *you hear the clink of glasses*.
- The ability to rescore at the evaluation screen to other judge values has been temporarily disabled pending an overhaul of the replay system.
- The option to flag the music wheel to use the old behavior when displaying grades by difficulty has been removed.
- While player 2 has not been supported since the origination of the project, it now like, probably doesn't work at all. Totally. Tubular. Far out hyper-torus.
- Having multiple profiles in the save folder is currently not supported, or at least, they don't operate in the way that you would expect, and it's that operational capacity that isn't supported. You can still try it *at your own peril*. Re-implementing support for separate profiles in the way most people imagine it should operate is planned.
- The advisory level on trying to install Etterna on cab has been elevated from "strongly against" to "if you try it don't tell me because I'll just laugh at you".
- Shadow rendering has been disabled. According to comments in the codebase it's entirely useless because it's non functional in the first place. 
- Haste mod.
- A number of music select codes have been again removed from fallback. It should be quite a bit harder to accidentally reset your player options by mashing buttons (but if you try really hard you probably still can). 

### Fixed
- Notes missed during long lag spikes will be correctly counted and scored.

### Known Issues
- The song cache has failed to rebuild for some people which tends to cause a lot of odd issues. If spooky things are afoot, manually clear the cache.
- Sometimes grades don't display correctly on the eval screen or on the music wheel (rare).
- Getting frisky enough spamming tabs at music select while manipulating the music wheel with the mouse will eventually cause a crash. Treat her like the delicate classy and hilariously antiquated lady she is.
- DWI (and possibly other?) format support has temporarily been removed while all the code related to the song load process is being reviewed and refactored for optimization and general sanity.
- Scores on edit difficulties of charts may not properly import to the new save format.
- Some things are buggy, sometimes.
- Music wheel sort orders that are not group sort are definitely super broken.
- Using the song reload from the options menu is not advised. Use the differential reload from music select if you want to load a few new songs or a new pack into the game during a session. Restart your game otherwise.
- Under certain conditions entering gameplay from a playlist group on the music wheel will return you to the pack it originated in after gameplay and not the playlist you started from. Homeward bound, you.
- Songs that end in long freezes may exit gameplay before the freezes end. This will not affect your score, however there will be a discrepancy at the evaluation screen involving the proportion of freezes held. It is a display error.
- The gameplay sync feature is temporarily inaccessible. This will be fixed in what may be fairly referred to as a tepidfix.
- The song options menu doesn't properly reflect the ability to set rates up to 3x.
- Some people (those using fullscreen) may have issues using printscreen to take screenshots, Use shift+printscreen instead.
- Setting MinTNSToHideNotes to "none" while chord cohesion is disabled will seriously screw up timings and judgments. Returning it to the default of 'w3' is advised.
- The ability to rescore at the evaluation screen to other judge values has been temporarily disabled pending an overhaul of the replay system. Yes I know this is duplicated but should probably be listed here for the people that don't read anything else.
- Jack stamina has been shifted to a placeholder pending updates to the calc. It is no longer used when calculating final player ratings or in any other aggregate calculations.
- Saving an empty playlist will essentially break your profile. If you do this you'll need to edit your etterna.xml and remove the playlist section or you'll crash on start up.
- Certain MP3 files may have offset issues when played in-game.