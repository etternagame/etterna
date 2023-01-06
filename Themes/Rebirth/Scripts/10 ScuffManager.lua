--- Rebirth Scuff Manager
-- this is like a singleton
-- i have no regrets making this
-- its just for global state dependent stuff that i didnt want to write into the game c++ or for some other reason related to being lazy
-- like imagine wanting different files to interact with each other but not wanting to message broadcast and manage 1000 booleans in those files
-- this is all of those bools but in one place instead
-- ok now that i wrote out that explanation that sounds really bad
-- who asked you anyways
-- @module Rebirth_10_ScuffManager

SCUFF = {}

SCUFF.generaltab = 1

SCUFF.generaltabcount = 6 -- this gets reset by the general box anyways
SCUFF.generaltabindex = 1
SCUFF.scoretabindex = 2
SCUFF.profiletabindex = 3
SCUFF.goalstabindex = 4
SCUFF.playliststabindex = 5
SCUFF.tagstabindex = 6

SCUFF.preview = {
    active = false,
    resetmusic = false,
}

-- if the user skipped the screen dont force it on them (once per game session)
SCUFF.visitedCoreBundleSelect = false

-- the screen to go to after the help screen
SCUFF.helpmenuBackout = ""

-- for controlling what happens after GameplaySyncMachine
-- change this before entering ScreenGameplaySyncMachine
-- HACK: ALSO USING THIS FOR ScreenTestInput
SCUFF.screenAfterSyncMachine = "ScreenOptionsInputSub"
SCUFF.screenAfterSyncMachine_iter = 0

-- interaction between settings and ???
SCUFF.optionsThatWillOpenTheLeftSideWhenHovered = {
    Noteskin = true,
}
SCUFF.showingNoteskins = false
SCUFF.showingPreview = false
SCUFF.showingColor = false
SCUFF.showingKeybinds = false