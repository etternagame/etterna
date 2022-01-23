
local ratios = {
    InfoTopGap = 25 / 1080, -- top edge screen to top edge box
    InfoLeftGap = 60 / 1920, -- left edge screen to left edge box
    InfoWidth = 1799 / 1920, -- small box width
    InfoHeight = 198 / 1080, -- small box height
    InfoHorizontalBuffer = 40 / 1920, -- from side of box to side of text
    InfoVerticalBuffer = 28 / 1080, -- from top/bottom edge of box to top/bottom edge of text

    MainDisplayTopGap = 250 / 1080, -- top edge screen to top edge box
    MainDisplayLeftGap = 60 / 1920, -- left edge screen to left edge box
    MainDisplayWidth = 1799 / 1920, -- big box width
    MainDisplayHeight = 800 / 1080, -- big box height

    ScrollerWidth = 15 / 1920, -- width of the scroll bar and its area (height dependent on items)
    ListWidth = 405 / 1920, -- from right edge of scroll bar to left edge of separation gap
    SeparationGapWidth = 82 / 1920, -- width of the separation area between selection list and the info area

    TopBuffer = 45 / 1080, -- buffer from the top of any section to any item within the section
    TopBuffer2 = 119 / 1080, -- from top edge of section to the subtitle text
    TopBuffer3 = 200 / 1080, -- from top edge of section to the description text
    EdgeBuffer = 25 / 1920, -- buffer from the edge of any section to any item within the section

    IconExitWidth = 47 / 1920,
    IconExitHeight = 36 / 1080,
}

local actuals = {
    InfoTopGap = ratios.InfoTopGap * SCREEN_HEIGHT,
    InfoLeftGap = ratios.InfoLeftGap * SCREEN_WIDTH,
    InfoWidth = ratios.InfoWidth * SCREEN_WIDTH,
    InfoHeight = ratios.InfoHeight * SCREEN_HEIGHT,
    InfoHorizontalBuffer = ratios.InfoHorizontalBuffer * SCREEN_WIDTH,
    InfoVerticalBuffer = ratios.InfoVerticalBuffer * SCREEN_HEIGHT,
    MainDisplayTopGap = ratios.MainDisplayTopGap * SCREEN_HEIGHT,
    MainDisplayLeftGap = ratios.MainDisplayLeftGap * SCREEN_WIDTH,
    MainDisplayWidth = ratios.MainDisplayWidth * SCREEN_WIDTH,
    MainDisplayHeight = ratios.MainDisplayHeight * SCREEN_HEIGHT,
    ScrollerWidth = ratios.ScrollerWidth * SCREEN_WIDTH,
    ListWidth = ratios.ListWidth * SCREEN_WIDTH,
    SeparationGapWidth = ratios.SeparationGapWidth * SCREEN_WIDTH,
    TopBuffer = ratios.TopBuffer * SCREEN_HEIGHT,
    TopBuffer2 = ratios.TopBuffer2 * SCREEN_HEIGHT,
    TopBuffer3 = ratios.TopBuffer3 * SCREEN_HEIGHT,
    EdgeBuffer = ratios.EdgeBuffer * SCREEN_WIDTH,
    IconExitWidth = ratios.IconExitWidth * SCREEN_WIDTH,
    IconExitHeight = ratios.IconExitHeight * SCREEN_HEIGHT,
}

local infoTextSize = 0.65
local listTextSize = 0.4
local titleTextSize = 0.95
local subtitleTextSize = 0.55
local descTextSize = 0.4

local textZoomFudge = 5
local buttonHoverAlpha = 0.3
local cursorAlpha = 0.3
local cursorAnimationSeconds = 0.1
local animationSeconds = 0.1

-- special handling to make sure our beautiful icon doesnt get tarnished
local logosourceHeight = 133
local logosourceWidth = 102
local logoratio = math.min(1920 / SCREEN_WIDTH, 1080 / SCREEN_HEIGHT)
local logoH, logoW = getHWKeepAspectRatio(logosourceHeight, logosourceWidth, logosourceWidth / logosourceWidth)

local function helpMenu()

    -- describe each category
    -- this appears in the scroll list and large above the main screen
    -- the point of this table is solely to maintain the order of option categories that show up
    local categoryDefs = {
        "Common Pattern Terminology",
        "Hotkeys",
        "How-To",
        "Information",
        "Troubleshooting",
    }

    -- describe each option in each category
    -- each option within each category shows up in exactly that order. the categories do not use this order (refer to the above table instead)
    -- the definition is:
    --[[
    ["CategoryDef Entry"] = {
        [1] = {
            Name = "OptionName", -- this appears in the scroll list and large in the main area
            ShortDescription = "1 sentence", -- this appears as a subtext to the large text in the main area
            Description = "a paragraph", -- this appears as regular text in the remaining area
            Image = "path to an image", -- OPTIONAL -- if supplied, this takes up the right half of the main area
        },
        [2] = {}, ....
    }
    ]]
    local optionDefs = {
        ["Common Pattern Terminology"] = {
            {
                Name = "Roll (1234)",
                ShortDescription = "Vaguely a jumptrill",
                Description = "Roll is the common name given to this type of pattern which requires you to press all columns in succession before repeating any columns again. It comes in several forms.\n\nThe specific roll depicted here may be referred to as an ascending roll. If it went in the opposite direction (4321) it would be descending.\n\nThe direction of the roll does not affect its capacity to be jumptrilled. Only the speed of the roll does.",
                Image = THEME:GetPathG("", "Patterns/1234 roll"),
            },
            {
                Name = "Roll (1243)",
                ShortDescription = "Vaguely a jumptrill",
                Description = "Roll is the common name given to this type of pattern which requires you to press all columns in succession before repeating any columns again. It comes in several forms.\n\nThe specific roll depicted here may be referred to as a split roll. If the pattern began with the opposite hand (4312) it would be functionally equivalent.\n\nRegardless, this roll variant can be effectively jumptrilled if its speed is sufficient.",
                Image = THEME:GetPathG("", "Patterns/1243 roll"),
            },
            {
                Name = "Roll (4132)",
                ShortDescription = "Vaguely a split jumptrill",
                Description = "Roll is the common name given to this type of pattern which requires you to press all columns in succession before repeating any columns again. It comes in several forms.\n\nThe specific roll depicted here may be referred to as a split roll, split hand roll, or split trill. It may also begin in reverse order (3241, inside out) but still remains functionally equivalent.\n\nThis roll variant is more difficult to jumptrill than others. When hit quickly, it is physically similar to a split jumptrill.",
                Image = THEME:GetPathG("", "Patterns/1423 roll"),
            },
            {
                Name = "Gluts",
                ShortDescription = "Another word for 'in excess'",
                Description = "Gluts is a broad term which captures jumpgluts and handgluts. It's frequently debated what defines a glut or even if a handglut exists.\n\nThe most accepted definition of a glut is literal: jumpgluts are many continuous jumps (jumpjacks) which typically form minijacks as they change column pairs over the course of the glut run. Depicted in the image is a run of jumpjacks referred to as gluts because of the minijacks on column 1 and 4.\n\nGluts are a subset of chordjacks with more focus on jack speed than chords.",
                Image = THEME:GetPathG("", "Patterns/gluts"),
            },
            {
                Name = "Chordjack",
                ShortDescription = "Chords which form jacks",
                Description = "Chordjack is the blanket term for patterns made up entirely of n-chords which form jacks.\n\nThe chords contained in the overall pattern do not have to be the same. Jumps, hands, or quads are valid. Depicted to the right is a very generic medium density chordjack pattern containing only jumps and hands.",
                Image = THEME:GetPathG("", "Patterns/chordjacks"),
            },
            {
                Name = "Dense Chordjack",
                ShortDescription = "Don't break your keyboard",
                Description = "Dense chordjacks are a specialization of chordjacks which are biased toward hands and quads. At high enough density, this may be referred to as holedodge because when reading, there are more notes than empty spaces.\n\nDense chordjacks require a lot of stamina and tend to have embedded longjacks due to the pattern being almost entirely hands and quads.",
                Image = THEME:GetPathG("", "Patterns/dense chordjack"),
            },
            {
                Name = "Stream",
                ShortDescription = "Continuous single taps",
                Description = "Just like the name implies, a stream is a continuous stream of notes. More specifically, these continuous notes are mostly on separate columns, not forming jacks. There can be any kind of variation to the patterning as long as it doesn't deviate too much from the pure definition.\n\nMinijacks, chords, or other patterns that can be embedded may be found within a stream, but only serve to make it more difficult unless they dominate the overall pattern.\n\nTo the right is a stream which is slightly rolly.",
                Image = THEME:GetPathG("", "Patterns/streams"),
            },
            {
                Name = "Jumpstream",
                ShortDescription = "Stream with jumps",
                Description = "Jumpstream expands the definition of a stream by requiring jumps at a certain frequency within the pattern. Other than that, it is still a stream.\n\nDepicted to the right is a simple jumpstream pattern with an anchor on column 1. A more observant player may also recognize that this pattern in isolation can be jumptrilled.",
                Image = THEME:GetPathG("", "Patterns/jumpstream"),
            },
            {
                Name = "Handstream",
                ShortDescription = "Stream with hands",
                Description = "Handstream expands the definition of a stream by requiring hands at a certain frequency within the pattern. It is also not uncommon to find jumps embedded within a handstream. This may be referred to as dense handstream, although the most dense handstream is purely hands and single taps.\n\nAnchors are more common in a handstream since the charter only has 4 ways to fit a hand into 4 columns. In the depiction to the right, there is an anchor on column 4 and, depending who you ask, also column 3.",
                Image = THEME:GetPathG("", "Patterns/handstream"),
            },
            {
                Name = "Quadstream",
                ShortDescription = "Stream with quads",
                Description = "Quadstream takes the definition of stream so far that it begins to look like chordjacks.\n\nIt forms a minijack with a quad using a single tap (usually) and still flows like a stream as opposed to pure chordjacks.\n\nIncreasing the density of quads will lose this characteristic due to the limited column space.",
                Image = THEME:GetPathG("", "Patterns/quadstream"),
            },
            {
                Name = "Trill",
                ShortDescription = "Like musical theory",
                Description = "A trill is a sequence of two continuously alternating notes. In the context of this game, they can be either on one hand or split between both hands.\n\nWhen a trill is one handed, it is called a one hand trill. Otherwise, it is a two hand trill. Trills are not restricted to two adjacent columns, and can be on columns 1 and 3 for example.\n\nDepicted to the right is a simple two hand trill.",
                Image = THEME:GetPathG("", "Patterns/trill"),
            },
            {
                Name = "Jumptrill",
                ShortDescription = "Can be played blind",
                Description = "Jumptrill is a very simple expansion of a two hand trill. Jump on one hand and then jump on the other. Most often this pattern makes up the highest NPS section of a chart unless it is dense chordjacks.\n\nOne special thing about the ordinary jumptrill is that many other patterns can be broken down into a jumptrill, which allows cheese-oriented gameplay. We don't recommend doing this too frequently for the sake of your scores and habit forming.",
                Image = THEME:GetPathG("", "Patterns/jumptrill"),
            },
            {
                Name = "Split Jumptrill [13][24]",
                ShortDescription = "Jumptrill but dangerous",
                Description = "Split jumptrill is a shuffled jumptrill. It forms two one hand trills instead of one pure jumptrill.\n\nSplit jumptrills are very annoying due to many players' inability to hit them fluently. This specific variant of split jumptrills can be more difficult than the other variant of split jumptrills because a player tends to be more inclined to jumptrill or roll when simultaneously doing a same-direction rolling motion with both hands. Luckily they can be jumptrilled if hit just right.\n\nTo the right is a long split jumptrill.",
                Image = THEME:GetPathG("", "Patterns/13 split jt"),
            },
            {
                Name = "Split Jumptrill [14][23]",
                ShortDescription = "Jumptrill but dangerous",
                Description = "Split jumptrill is a shuffled jumptrill. It forms two one hand trills instead of one pure jumptrill.\n\nSplit jumptrills are very annoying due to many players' inability to hit them fluently. Compared to the other variant of split jumptrills, this one is usually easier because it can feel more natural. Anyways, they can be jumptrilled if hit just right.\n\nTo the right is a long split jumptrill.",
                Image = THEME:GetPathG("", "Patterns/14 split jt"),
            },
            {
                Name = "Minijacks",
                ShortDescription = "Instant combo breaker",
                Description = "Minijacks are pairs of jacks. They can be continuous like the image to the right or embedded in some other pattern like a stream.\nMinijacks can be difficult to hit accurately as they get closer together because of the nature of the hit window. Imagine hitting one note within 180ms. Now hit two notes in that same window. If the minijack is fast enough or the player is slow enough, it's guaranteed points lost.\nMinijacks can be embedded within broader jack oriented patterns as a jack burst. Most commonly they are in isolation or in stream transitions that jack instead of trill.",
                Image = THEME:GetPathG("", "Patterns/minijacks"),
            },
            {
                Name = "Longjack",
                ShortDescription = "Continuous taps",
                Description = "A jack, or jackhammer, is a set of continuous taps in the same column. A longjack is the same thing, but a little longer than usual.\n\nThe length of a jack that is considered a longjack is debated, but generally it is around 5-6 notes. The longjack can continue into infinity.\n\nLongjacks are the base pattern which make up continuous jumpjacks, handjacks, or quadjacks. This base pattern also makes up much of the structure for files oriented towards the vibro playstyle.",
                Image = THEME:GetPathG("", "Patterns/longjack"),
            },
            {
                Name = "Anchor",
                ShortDescription = "Basically embedded longjacks",
                Description = "An anchor is a common continuous set of columns being utilized relative to the other columns contained in a pattern. Most commonly, anchors are only one column at a time.\n\nAn anchor may be on a snap alternating from the rest of the pattern or not, or a bit of both. Anchors that last a while break down to be longjacks, which will cause an overall pattern to be more stamina draining.\n\nThe image to the right depicts an anchor on column 4.",
                Image = THEME:GetPathG("", "Patterns/anchor"),
            },
            {
                Name = "Minedodge",
                ShortDescription = "Spicy notes",
                Description = "Minedodge is the term for a type of chart or general pattern which contains notes that are intentionally placed near mines to increase difficulty.\n\nMinedodge does not actually change the MSD of a chart, because the calculator measures physical difficulty of the taps.\n\nThe difficulty of minedodge comes from the necessity of more precisely timing the press and release of notes and increased difficulty to read the notes depending on the noteskin used.",
                Image = THEME:GetPathG("", "Patterns/minedodge"),
            },
            {
                Name = "Hold",
                ShortDescription = "Don't let go",
                Description = "Holds are a note type which require the player to hold the button for the entire duration of the note. They may also be referred to as freezes or long notes.\n\nPatterns made up of more holds are sometimes referred to as a holdstream or, at the extreme end of the spectrum, full inverse (all empty space is a hold).\n\nIn this game, a hold can be released for a short period of time dependent on the judge difficulty. On judge 4, the time is 250ms. Holds can then be regrabbed. Holds do not have release timing, but release timing can be emulated with a lift or mine.",
                Image = THEME:GetPathG("", "Patterns/hold"),
            },
            {
                Name = "Roll / Rolld",
                ShortDescription = "Keep tapping",
                Description = "Roll note types, not to be confused with the roll pattern, are a hold type which require the player to continuously tap the button for the duration of the note. They may also be referred to as a rolld.\n\nRolls cannot be held and must be continuously tapped. The speed of the tap has a threshold the player receives no judgment for, but it gets smaller at higher judge difficulties. On judge 4, the player has up to 500ms between taps.",
                Image = THEME:GetPathG("", "Patterns/rolld"),
            },
            {
                Name = "Burst",
                ShortDescription = "Explosive speed",
                Description = "A burst is a pattern specialization which means exactly what it says. Relative to its pattern context, a burst is much quicker.\n\nBursts can come in any form which matches that definition, not only the scenario depicted to the right. Jacks and jumpstream can also burst. The point is that it is a quicker collection of notes, almost like a compressed pattern.\n\nOften, a burst is patterned in such a way that it isn't difficult to full combo. But that isn't always the case!",
                Image = THEME:GetPathG("", "Patterns/burst"),
            },
            {
                Name = "Polyrhythm",
                ShortDescription = "Brain melting patterns",
                Description = "Polyrhythms are a pattern specialization which indicates that multiple rhythms are being charted simultaneously, leading to alternating snaps being utilized. Sometimes the result of this is a very awkward, technically difficult to execute pattern.\n\nTo the right is a depiction of a simpler polyrhythm of 16ths and 12ths.",
                Image = THEME:GetPathG("", "Patterns/polyrhythms"),
            },
            {
                Name = "Graces / Flams",
                ShortDescription = "A little extra flare",
                Description = "Grace notes are slightly offset notes, exceptionally rarely forming minijacks, which represent a kind of grace or extra flare to an initial note.\n\nIn musical theory, these are defined as not so necessary, but typically when these are charted it means that a grace note was present in the music.\n\nFlams are made up of graces. Within this game, both usually mean the same. Graces usually break down to be a single chord, and can rarely contain chords themselves.",
                Image = THEME:GetPathG("", "Patterns/graces"),
            },
            {
                Name = "Runningman",
                ShortDescription = "Anchored stream",
                Description = "Classically, runningman is a term referring to a stream that is anchored. We expand on that definition by allowing chords to be mixed in very lightly. An anchored jumpstream can technically contain a runningman, but is more likely to just be referred to as anchored jumpstream.\n\nIt is required that the anchor in a runningman be offset from the rest of the pattern so that it doesn't form chords with the rest of the pattern.\n\nTo the right is a runningman anchored on column 1. The off-taps may be on any column, but it is important that not too many taps be on the same hand as the anchor.",
                Image = THEME:GetPathG("", "Patterns/runningman"),
            },
        },
        ["Hotkeys"] = {
            {
                Name = "Global Hotkeys",
                ShortDescription = "Hotkeys available anywhere",
                Description = "F2 -- Reload Textures & Metrics\nF2 + Shift -- Reload Metrics\nF2 + Ctrl -- Reload Scripts\nF2 + Shift + Ctrl -- Reload Overlay Screens\nF3 -- Debug Menu (many shortcut options within)\nF9 -- Toggle author-defined metadata transliteration\nAlt + Enter -- Fullscreen Toggle\nTab -- Speed up animations x4\n~ -- Slow down animations x4\nPause|Break -- Toggle menu sounds\nPrintScreen -- Screenshot",
                Image = nil,
            },
            {
                Name = "SelectMusic Hotkeys",
                ShortDescription = "Hotkeys only in song selection",
                Description = "F1 -- Song Search\nCtrl + Q -- Load new song folders\nShift + Ctrl + R -- Reload current song folder\nShift + Ctrl + P -- Reload current pack folder\nCtrl + F -- Favorite chart toggle\nCtrl + M -- Permanent mirror chart toggle\nCtrl + G -- Create goal on current chart\nCtrl + O -- Toggle practice mode\nCtrl + S -- Save profile\nCtrl + P -- Create playlist\nCtrl + A -- Add current chart to selected playlist\nCtrl + Number -- If main box active, access the top right buttons. If not, access main box tabs.\nNumber -- Switch tabs in main box/settings\nEscape/Back -- In side box context, exit to main box\nCtrl + L -- Login/Logout\nSpace -- General Tab/Settings Chart Preview toggle\nRight Click -- Pause music toggle if functionality not overridden",
                Image = nil,
            },
            {
                Name = "Gameplay Hotkeys",
                ShortDescription = "Hotkeys only in regular gameplay",
                Description = "Hold Enter -- Force Fail\nF4 -- Undo sync changes before saving them\nF6 -- Toggle through Autosync Song, Autosync Machine\nF7 -- Toggle claps\nLeftShift + F7 -- Toggle metronome\nF8 -- Toggle autoplay\nF11 -- Move song offset earlier\nAlt + F11 -- Move song offset earlier (small increment)\nF12 -- Move song offset later\nAlt + F12 -- Move song offset later (small increment)\nShift + F11 -- Move global offset earlier\nAlt + Shift + F11 -- Move global offset earlier (small increment)\nShift + F12 -- Move global offset later\nAlt + Shift + F12 -- Move global offset later (small increment)",
                Image = nil,
            },
            {
                Name = "Customize Gameplay Hotkeys",
                ShortDescription = "Hotkeys only in gameplay customization",
                Description = "Enter -- Select element or Deselect element\nDelete/Backspace/RestartGameplay -- Undo only the most recent change\nCtrl + Undo -- Reset selected element to default\nRight Click -- Deselect element\nDirectional -- Menu movement or selected element movement\nShift + Directional -- Smaller increment movement\nSpace -- Scroll through movement types of selected element",
                Image = nil,
            },
            {
                Name = "Practice Hotkeys",
                ShortDescription = "Hotkeys only in practice mode",
                Description = "Backspace -- Jump to loop region start or bookmarked position\nEffectUp -- Increase rate 0.05x\nEffectDown -- Decrease rate 0.05x\nInsertCoin -- Set bookmark position or loop region boundaries\nInsertCoin twice while paused -- Reset loop region to a bookmark position\nMousewheel Scrolling -- If paused, move song position in fine increments\nRight Click -- Toggle pause\nLeft Click Graph -- Jump to position\nRight Click Graph -- Set bookmark or loop region boundaries",
                Image = nil,
            },
            {
                Name = "Replay Hotkeys",
                ShortDescription = "Hotkeys only in replays",
                Description = "InsertCoin -- Toggle pause\n\nMust be paused before using any of the following:\nAlt + EffectUp -- Set bookmark position\nAlt + EffectDown -- Go to bookmark position\nShift + EffectUp -- Increase rate 0.05x\nShift + EffectDown -- Decrease rate 0.05x\nEffectUp -- Move song position 5 seconds\nCtrl + EffectUp -- Move song position 0.1 seconds\nEffectDown -- Move song position -5 seconds\nCtrl + EffectDown -- Move song position -0.01 seconds",
                Image = nil,
            },
            {
                Name = "Evaluation Hotkeys",
                ShortDescription = "Hotkeys only in the evaluation screen",
                Description = "Ctrl + L -- Login/Logout\nUp/Down -- Scroll through column highlight settings\nEffectUp -- Increase display judge\nEffectDown -- Decrease display judge\nSelect -- Screenshot\nLeft + Right -- Screenshot",
                Image = nil,
            },
            {
                Name = "Multiplayer Hotkeys",
                ShortDescription = "Hotkeys only in multiplayer",
                Description = "Multiplayer isn't finished!",
                Image = nil,
            }
        },
        ["How-To"] = {
            {
                Name = "Create Charts",
                ShortDescription = "Notes to noises",
                Description = "You need an editor. Etterna doesn't currently come with an editor. The popular choices are ArrowVortex and DDReam. Others also choose to use the osu editor or older SM3/SM5 editors.\n\nWhat matters is you have a way to place notes and get them to a valid format like .sm.\n\nA chart only requires audio and the metadata file (.sm for example) to load. Be careful not to attempt to load the chart before both of these are present in your Etterna Songs folder.\n\nMore extensive tutorials on chart creation exist online, and most people are willing to help if you ask around.",
                Image = nil,
            },
            {
                Name = "Manual Song Install",
                ShortDescription = "When downloading in client isn't your thing",
                Description = "Here's the scenario: you've created or downloaded a single song or a pack of songs. Now you need to make them load in the game.\n\nIdeally, the folders are structured in this pattern - /Packname/Songname/stuff.sm\n\nIf this applies to a pack you just downloaded, all you must do is extract the pack folder and place it in your Songs folder. The structure is then /Songs/Packname/Songname/stuff.sm.\n\nIf you have a single song, you should create a new pack folder for it in the Songs folder. It can have any name. After all of this is completed, either reopen the game or press Ctrl + Q in SelectMusic.\n\nIf for some reason you want to keep your install separate from your songs, open Preferences.ini and add a direct path to a new Songs folder in the AdditionalSongFolders field.",
                Image = nil,
            },
            {
                Name = "Download Packs",
                ShortDescription = "How to press a button",
                Description = "Ingame, downloading packs is very simple. If you have no packs installed, you may be led first to the core bundle select screen which shows a few sets of packs of varying difficulty to get you started.\n\nIf you skipped that, don't want the bundles, or already have something installed, you can use the downloader screen in SelectMusic. In the top right, in the Ctrl + 3 menu, you should find every pack you could download from ingame. If this list is blank, you may not have an internet connection.\n\nThere are other sources of packs if these downloads ever fail which you must ask around for.\n\nDownloaded packs will extract and install automatically when finished. If it appears that they finish but nothing happens, the download may have failed instead.",
                Image = nil,
            },
            {
                Name = "Song Search",
                ShortDescription = "Finding the song",
                Description = "You may have been expecting to be able to search for a song that you don't have installed. Let's get that out of the way first - it isn't currently possible from within the client.\n\nYou can search for songs that are installed using the F1 menu (the leftmost button in the top right of SelectMusic). The interface should be simple to use. Just fill out the fields you care for and hit enter, and the results come to you.\n\nOtherwise, places that host packs may offer search capabilities.",
                Image = nil,
            },
            {
                Name = "Song Filter",
                ShortDescription = "Filtering the songs",
                Description = "The song filter is integrated with song search. It can be found in the F1 menu (the leftmost button in the top right of SelectMusic).\n\nThere is not currently any keyboard compatibility with this menu, but the sliders should be sufficient to let you set up a filter with your mouse. A slider placed at an extreme end is considered effectively 0 or infinite, a disabled bound.\n\nFilters stick if you happen to enter a song and come back, unlike the song search.",
                Image = nil,
            },
            {
                Name = "Sortmodes",
                ShortDescription = "Sorting the songs",
                Description = "The song wheel can be resorted in different predefined ways.\n\nTry pressing up-down-up-down in the main area, and it changes your wheel into a menu to select a sortmode. Some people will find some sortmodes more useful than others.\n\nMost of the time, Group sort is used because it is the natural order everyone expects. Some sorts are for more informational purposes.\n\nIf you would like to modify the behavior of these sortmodes, check out the implementation in Scripts/WheelDataManager.",
                Image = nil,
            },
            {
                Name = "Customize Gameplay",
                ShortDescription = "Power at your fingertips",
                Description = "Customize Gameplay is the gameplay screen which allows you to modify every element on the screen in almost any way to make it fit to your standard.\n\nOn the right side is the navigation and information panel which can be dragged around for visibility. The arrow keys can be used if the mouse is not preferred.\n\nEnter to select a highlighted element, or click an element to select it. Use space to change which movement type you are modifying (coordinates vs sizing). Some elements do not offer more than one type. The mouse can be used to drag around any element.\n\nHold shift to move with smaller increments. Use RestartGameplay to undo or hold Ctrl with it to reset to default.\n\nAll changes are saved to disk as soon as you exit the screen.",
                Image = nil,
            },
            {
                Name = "Update Etterna",
                ShortDescription = "Backup your user data",
                Description = "Updating Etterna is typically a painless process on all platforms. In all cases, we would recommend you take a backup of any content you directly added to the game - Noteskins, Save, Assets. Songs do not need to be backed up.\n\nOn Windows, the installer may ask for you to uninstall the old version, and the uninstaller can fail. It's safe to ignore that. As long as you are not directly overlaying the latest version on top of the old one, your new install will run fine.\n\nOn Mac, updating is simply reinstalling the game again, but with less configuration.\n\nOn Linux, updating can be moving to the latest binary or a git pull and rebuild.\n\nThe most important folder you never want to lose is the entire Save folder. Nearly every file in this folder is related to user scores or settings in some way.",
                Image = nil,
            },
            {
                Name = "Set Sync/Offset",
                ShortDescription = "Notes follow music, you know",
                Description = "Every player will experience an issue at some point which manifests in their tap offsets being generally late or generally early. This translates to a late or early mean. In any scenario, a negative offset or mean indicates 'early.'\n\nIf you suffer from an unplayable offset or a consistently bad mean (> 5-10ms +/-) here's a few things to try.\nEtterna offers a visual offset (also called the judge offset), a global (machine) offset, and a song offset. The song offset is set by each chart author, and if it ever ends up wrong, that tends to be their fault and not yours.\nMoving the visual offset moves the visual position where a perfect hit is relative to your receptors. To change it, find it in the F3 menu or in the SelectMusic settings.\nGlobal offset applies to all songs in addition to the song offset. It changes the position of the audio relative to the notes. Ideally, it is 0ms, but can be nonzero if you have some audio/monitor/input related discrepancies.\nGlobal and song offsets can be set automatically by going into a song and pressing F6, then playing to the best of your ability. Try not to let autosync run more than 2 iterations.\nYou may also try moving them manually with F11/F12.",
                Image = nil,
            },
            {
                Name = "Change Theme",
                ShortDescription = "It's not called a skin",
                Description = "The general look of the game is called a Theme. Most of it is written in Lua.\n\nTo install a theme, you just extract its folder into the Themes folder of your install. You shouldn't ever have to backup this folder.\n\nTo change your theme, you can find the option in SelectMusic settings, or in Display Options. It won't look the same as this theme though.",
                Image = nil,
            },
            {
                Name = "Change Language",
                ShortDescription = "We need translators",
                Description = "Etterna comes with a partial translation into a few popular languages, but overall none of them are nearly as sufficient as we would like them to be.\n\nTranslation support is always on our mind, but not a very high priority.\n\nTo change language and see what things look like, find it in Display Options or in SelectMusic settings.",
                Image = nil,
            },
            {
                Name = "Toggle Menu Sounds",
                ShortDescription = "Beep",
                Description = "Sometimes menu sounds are annoying.\n\nTo turn them off, press Pause on your keyboard, or find the option in the F3 menu F5 page or in SelectMusic settings.",
                Image = nil,
            },
            {
                Name = "Toggle Pitch Rates",
                ShortDescription = "Nightcore Remix",
                Description = "Some players enjoy high pitch noises more than others, and some players are better when the song is just faster but not high pitched.\n\nTo toggle pitch usage on rates, find it in the F3 menu F8 page, Advanced Options, or SelectMusic settings.",
                Image = nil,
            },
            {
                Name = "Swap Wheel Side",
                ShortDescription = "Change is bad",
                Description = "Understandably, some people are bound to not like the fact that the wheel defaults to the left side in Rebirth.\n\nTo swap sides, find the wheel position option in SelectMusic settings > Graphics > Theme Options.",
                Image = nil,
            },
            {
                Name = "Login",
                ShortDescription = "Using Online Functionality",
                Description = "Login is required if you ever want to upload scores or view leaderboards.\n\nTo begin the login process, you click the broken link in the top left or press Ctrl + L while in SelectMusic. Doing the same again will log you out.\n\nLogging in this way is not required to play multiplayer. Instead, you are separately asked to log in to that, if it is required.",
                Image = nil,
            },
            {
                Name = "Upload Scores",
                ShortDescription = "Flex your 89% on Game Time",
                Description = "You must first be logged in to upload any score.\n\nEvery time you log in (and if you automatically log in at the start of a session) Etterna will try to upload any scores you have set that have not yet been uploaded. Scores will also attempt to upload immediately after setting a score, even before your profile saves.\n\nOnly scores on ranked charts will be uploaded. You can see if a chart is ranked by checking its leaderboards in the Scores tab.\n\nTo try force uploading, find the upload buttons either in the Profile tab or the Scores tab.\n\nEtterna uploads things in the background, so there is no clear progress indicator. Just trust.\n\nScores worth 0.00 or that are otherwise invalid will not upload.",
                Image = nil,
            },
            {
                Name = "Favorite Songs",
                ShortDescription = "Ear worms",
                Description = "Your song library might become huge and Favoriting is a way to keep track of the files you really like. Just press Ctrl + F on a chart and it becomes a Favorite. Do the same again to remove it.\n\nFavorites can be viewed as a playlist or in the Favorites sortmode if you press Up-Down-Up-Down.",
                Image = nil,
            },
            {
                Name = "Permanently Mirror Charts",
                ShortDescription = "Hand bias bad",
                Description = "Some charts out there are very biased toward one hand, and some players have particularly bad hands. Because Etterna calculates based on physical difficulty and mirroring a chart is still equivalent because only the hands are swapped, mirroring is a core feature of the game and is ranked.\n\nSome charts might need to always be mirrored for some players, but the players might not want to leave mirror on for everything. Permamirror is the solution.\n\nTo permanently toggle mirror on a specific chart, hover it and press Ctrl + M. Every time you open it, mirror will be on.",
                Image = nil,
            },
        },
        ["Information"] = {
            {
                Name = "About Keymodes",
                ShortDescription = "Styles, StyleTypes, StepsTypes, Games ...",
                Description = "At its root, this game is a simulator for several other real games. This is why keymodes are separated by Game, and then further by Style.\n\nDance - The core game. 4k. Dance-double can be found here, and is 8k. Also contains a 3k type.\nSolo - Separated from the core game, 6k.\nBeat - The BMS game. 5k+1, 7k+1, and doubles for both.\nKb7 - A gamemode that didn't take off, 7k.\nPump - 5k, 6k, and 10k in single, halfdouble, and doubles.\nPopn - Yes, popn. 5k and 9k.\n\nWithin a game, each style is visible in the difficulty displays. You can switch game in SelectMusic settings or the Select Game screen in Options.",
                Image = nil,
            },
            {
                Name = "Old Key Config",
                ShortDescription = "You shouldn't need this",
                Description = "Old key config can still be accessed through the main menu options. The left half of the screen is where the relevant controls are - it's the Player 1 side. The right half is for Player 2, but now those controls are dead. The default columns cannot be modified directly.\n\nTo unbind a default control, rebind that key to a different button on the right side that is infrequently used.\n\nIf you only use the key rebinding screen in Rebirth, you never have to use this screen.\n\nBe aware - just because this screen allows you to bind the same button to multiple keys to play with does not mean that setting scores using that kind of setup is allowed. The feature is meant more for people with a particular playstyle or restriction.",
                Image = nil,
            },
            {
                Name = "Replays",
                ShortDescription = "Stuck in the past",
                Description = "Local and online replays can be viewed for scores where valid replay data is present.\n\nLocal replays can be viewed if the replay is present in Replays or ReplaysV2. You have to find the score in the scores tab, and then click the button to view the replay.\n\nOnline replays can be viewed straight from the chart leaderboards.\n\nWhile in a replay, you can pause and change rates or seek around. The large progress bar can be used as a seeking bar.\n\nEvaluation will show a reconstruction of the actual score's evaluation screen.",
                Image = nil,
            },
            {
                Name = "SelectMusic Tips",
                ShortDescription = "Speedy menu navigation",
                Description = "SelectMusic contains a long list of hotkeys that make navigation quick. Memorizing many of them will be very helpful. Find the page on SelectMusic hotkeys to learn more.\n\nCertain parts of the UI have extra mouse functionality. Clicking the header of the wheel will randomly pick a group or a song in the current group. Clicking certain text in the Overall page of the profile tab toggles the displayed information. Clicking the banner of a song opens chart preview.\n\nThe music wheel scroll bar can be clicked or dragged to quickly navigate through songs. Right clicking scores in the profile tab invalidates them.\n\nEscape is a very useful button to exit the side menus that come up when using the buttons at the top of the screen.",
                Image = nil,
            },
            {
                Name = "Profile Tab Usage",
                ShortDescription = "Your Profile and You",
                Description = "The Profile tab contains most relevant information about your game usage and scores. The Overall section gives a glance at online and offline ratings as well as general stats. Here, you can upload all scores (that haven't been uploaded) or revalidate all scores locally. You can also click the Player Ratings text to switch it to Player Stats for more information.\n\nIn the View Recent Scores button, or any of the other tabs, you can view several hundred of your scores organized by whichever condition the tab specializes in. Stream for example sorts everything by stream. Recent scores sorts them by date.",
                Image = nil,
            },
            {
                Name = "Goals Tab Usage",
                ShortDescription = "Strive to be better eventually",
                Description = "The Goals tab contains a listing of goals you created on charts which you intend to eventually reach. That's the nature of a goal. By default, all goals start at priority 1, rate 1x, and 93%. These attributes can be clicked to increase or decrease them.\n\nTo create a goal, there's a button for it in the tab or you can use Ctrl + G.\n\nThe top buttons of the tab are for sorting the list of goals. The rightmost button filters the list by complete status.\n\nA vacuous goal simply means that the goal is pointless because you have already set a score that beats it. Remove it or make a higher goal.\n\nGoals save to your XML.",
                Image = nil,
            },
            {
                Name = "Playlists Tab Usage",
                ShortDescription = "'Courses'",
                Description = "The Playlists tab lets you create multiple lists of charts to play or refer back to. These don't actually have to be used to play in succession like a course, but can serve to be lists of files.\n\nTo create a playlist, Ctrl + P can be used or you can use the New Playlist button. Then click a playlist name to enter it.\n\nTo add charts to the selected playlist, use Ctrl + A or click the Add Current Chart button. The rate a chart gets played on in a course playback can be changed if you click the rate in the detail list.\n\nA playlist cannot be reordered.\n\nPlaylists save to your XML.",
                Image = nil,
            },
            {
                Name = "Tags Tab Usage",
                ShortDescription = "Describe your charts",
                Description = "The Tags tab lets you put description tags on charts so you can either describe them in your own way or filter based on the tags.\n\nAny number of tags can be assigned to a chart. Assigned tags (up to a certain amount) will show up in the General tab, and also as a different color in the Tags tab.\n\nThe Require Tag button sets a filter on all songs that requires the tag is assigned. The Hide Tag button does the opposite - any song with that tag is not visible. You must click Apply to set these filters.\n\nTo assign tags to a chart, just click the Assign button then click some tags.\n\nDeleting tags is a 2 step process started by clicking Delete.\n\nThe Reset button resets all filters. It does not delete any tags.\n\nTags are saved in a tags.lua file which can be shared with other people.",
                Image = nil,
            },
            {
                Name = "Getting Support",
                ShortDescription = "Not for spoonfeeding",
                Description = "Sometimes getting Etterna set up or getting things fixed is not a simple process. Well ... it is to a lot of us, but not everyone.\n\nSince many people know the solution to a lot of common issues, if you ever need help, you are free to ask around in the Etterna communities for help. Github issues (can be found on the Title Screen) is a place to report bugs, not ask for help.\n\nDiscord servers that have answers to your questions can be found on the front page of the Github repo, or any website associated with this game.",
                Image = nil,
            },
        },
        ["Troubleshooting"] = {
            {
                Name = "Softlocked",
                ShortDescription = "Locked in",
                Description = "In some rare conditions, not just in Rebirth, the game can lock but not hard crash or freeze. You can tell that this is what is happening if some buttons might work, but you can't advance or move or go back.\n\nThe instant solution to a softlock is Ctrl + Operator. By default, Operator is set to Scroll Lock. It can be rebound in the old key config. Pressing this combo will send you to the options menu.\n\nF3 + F8 + 9 will also put you in old key config.\n\nIf you ever end up in a softlock condition, it is helpful to be able to consistently reproduce and then describe how you did it in detail in a Github issue so that we can patch it.",
                Image = nil,
            },
            {
                Name = "All Input Broke",
                ShortDescription = "The menu just laughs at you",
                Description = "Rebirth has some input quirks which can one way or another cause your menu navigation to completely stop working or work incorrectly.\n\nTo solve this, you can first try to escape back to the main menu. If not possible or you are already there, try reloading scripts and then overlays - Ctrl + F2 and then Ctrl + Shift F2. Then reload the screen by pressing F3 + F6 + 2.\n\nIf you ever end up in this scenario, it is very helpful to report it.",
                Image = nil,
            },
            {
                Name = "General Tips",
                ShortDescription = "Blanket fixes",
                Description = "Lots of random things happen in this game. Here's some things to try before crying about it.\n\nRebirth locks, input issues, and error spam can be temporarily solved usually by reloading scripts, overlays, and the current screen. Ctrl + F2 > Ctrl + Shift F2 > F3 + F6 + 2. When developing on this theme especially, you will want to know this combo.\n\nCtrl + Operator or F3 + F8 + 9 will get you out of any softlock.\n\nIf input or the graphics don't work for some reason, rebooting is usually a fix.\n\nConstant crashing on startup is usually caused by putting random garbage in pack folders or 0 BPM related issues.",
                Image = nil,
            },
            {
                Name = "Song Doesn't Load",
                ShortDescription = "Mysterious",
                Description = "Sometimes when creating charts, authors find that the chart they are working on never appears. There's a reason for this (it's mostly a bug)\n\nIf you put music into a song folder without a .sm and restart or Ctrl + Q at any point, the folder is now forever ignored as not a song directory. The same applies if you put a .sm in that folder but it contains no charts.\n\nTo be safe, you should put audio with the .sm and a valid chart in before restarting or using Ctrl + Q. This will load the new song fine.\n\nIf this situation ever comes up, to solve it you must either delete the Cache folder or rename the song folder.",
                Image = nil,
            },
            {
                Name = "Song Doesn't Update",
                ShortDescription = "When you don't know features",
                Description = "When editing charts, authors may run into an issue where the notedata they set is updated when they go to playtest, but the song ends early or late. Or none of the metadata updates to what they saved it as.\n\nThe reason for this is that the game has a cache which it typically blindly trusts. It loads new notedata from disk when going into gameplay, but will not modify chart properties based on that notedata.\n\nTo fix this, hover the song and press Ctrl + Shift + R. That will update the song from disk.",
                Image = nil,
            },
            {
                Name = "Scores Don't Save",
                ShortDescription = "Rare bugs or game misuse",
                Description = "There are a couple of scenarios you may find yourself in if scores stop saving.\n\nThe usual one is that you've accidentally turned off Save Scores. This is an option that is found somewhere in the menus. It normally shouldn't be turned off.\n\nAnother is a rare bug called 0x where in the evaluation somehow your music rate is set to 0x, and now the likelihood that the rest of the session doesn't save is very high. To fix, just restart.\n\nThe last is a faulty install. You installed in a location where you don't have write permissions, such as Program Files. Or you ran out of disk space.",
                Image = nil,
            },
            {
                Name = "Scores Worth 0.00",
                ShortDescription = "Invalid scores",
                Description = "Scores become worth 0 instantly when the score is invalid upon creation.\n\nConditions for invalid scores:\nNegative BPMs or Warps\nInvalid modifiers (it is obvious which ones are being used)\nFile has less than 200 notes\nFailed\nChord Cohesion On\nAutoplay or Practice\nCertain lua mods active",
                Image = nil,
            },
            {
                Name = "Random Crashing",
                ShortDescription = "That's life",
                Description = "Sometimes the game crashes. Crashdumps should be generated in a folder within your Program folder.\n\nIf you opt in to crash uploading, you should just let us know in an issue or a report on Discord that you are having crash issues. Otherwise, still let us know but be ready to supply us with the associated .dmp file and logs.\n\nIf you keep crashing for no clear reason, it's always good to submit an issue or ask the community what is happening.",
                Image = nil,
            },
        },
    }

    -- generated table
    -- basically the data representation of the scroller thing
    -- categories are top level items
    -- options are slightly shifted over
    -- pagination and indexing is based on this table
    local items = {}
    for _, cat in ipairs(categoryDefs) do
        items[#items+1] = {
            isCategory = true,
            Name = cat,
        }
        for __, optionDef in ipairs(optionDefs[cat]) do
            items[#items+1] = {
                isCategory = false,
                Parent = cat,
                Name = optionDef.Name,
                Def = optionDef,
            }
        end
    end

    local itemsVisible = 20
    local cursorIndex = 1
    local page = 1
    local maxPage = math.ceil(#items / itemsVisible)
    local function getPageFromIndex(i)
        return math.ceil((i) / itemsVisible)
    end
    local function cursorHoversItem(i)
        if getPageFromIndex(cursorIndex) ~= page then return false end
        return ((cursorIndex-1) % itemsVisible == (i-1))
    end
    local function movePage(n)
        local newpage = page + n
        if newpage > maxPage then newpage = 1 end
        if newpage < 1 then newpage = maxPage end
        page = newpage
        MESSAGEMAN:Broadcast("UpdatePage")
    end
    local function moveCursor(n)
        local newpos = cursorIndex + n
        if newpos > #items then newpos = 1 end
        if newpos < 1 then newpos = #items end
        cursorIndex = newpos
        local newpage = getPageFromIndex(cursorIndex)
        if newpage ~= page then
            page = newpage
            MESSAGEMAN:Broadcast("UpdatePage")
        end
        MESSAGEMAN:Broadcast("UpdateCursor")
    end

    local function menuItem(i)
        local yIncrement = (actuals.MainDisplayHeight) / itemsVisible
        local index = i
        local item = items[index]
        return Def.ActorFrame {
            Name = "MenuItem_"..i,
            InitCommand = function(self)
                self:x(actuals.ScrollerWidth + actuals.EdgeBuffer/2)
                -- center y
                self:y(yIncrement * (i-1) + yIncrement / 2)
                self:playcommand("UpdateItem")
            end,
            SelectCurrentCommand = function(self)
                if cursorHoversItem(i) then
                    -- do something
                    MESSAGEMAN:Broadcast("SelectedItem", {def = item.Def, category = item.Parent})
                end
            end,
            UpdateItemCommand = function(self)
                index = (page-1) * itemsVisible + i
                item = items[index]
                if item ~= nil then
                    self:finishtweening()
                    self:diffusealpha(0)
                    self:smooth(animationSeconds)
                    self:diffusealpha(1)
                else
                    self:finishtweening()
                    self:smooth(animationSeconds)
                    self:diffusealpha(0)
                end
            end,
            UpdatePageMessageCommand = function(self)
                self:playcommand("UpdateItem")
            end,

            UIElements.QuadButton(1, 1) .. {
                Name = "ItemBG", -- also the "cursor" position
                InitCommand = function(self)
                    self:halign(0)
                    -- 97% full size to allow a gap for mouse hover logic reasons
                    self:zoomto(actuals.ListWidth - actuals.EdgeBuffer, yIncrement * 0.97)
                    self:diffusealpha(0)
                    self.alphaDeterminingFunction = function(self)
                        local alpha = 1
                        if isOver(self) then
                            alpha = buttonHoverAlpha
                            if cursorHoversItem(i) then
                                alpha = (buttonHoverAlpha + 1) / 2
                            end
                        else
                            alpha = 0
                            if cursorHoversItem(i) then
                                alpha = cursorAlpha
                            end
                        end

                        self:diffusealpha(alpha)
                    end
                end,
                CursorShowCommand = function(self)
                    self:smooth(cursorAnimationSeconds)
                    self:alphaDeterminingFunction()
                end,
                CursorHideCommand = function(self)
                    self:smooth(cursorAnimationSeconds)
                    self:alphaDeterminingFunction()
                end,
                MouseOverCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                MouseOutCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
                UpdateCursorMessageCommand = function(self)
                    self:alphaDeterminingFunction()
                end,
                UpdateItemCommand = function(self)
                    self:alphaDeterminingFunction()
                end,
                MouseDownCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    cursorIndex = index
                    self:GetParent():playcommand("SelectCurrent")
                end,
                SelectedItemMessageCommand = function(self)
                    if self:GetParent():IsInvisible() then return end
                    self:alphaDeterminingFunction()
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "Text",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoom(listTextSize)
                    self:maxwidth((actuals.ListWidth - actuals.EdgeBuffer * 2) / listTextSize)
                end,
                UpdateItemCommand = function(self)
                    if item ~= nil then
                        if not item.isCategory then
                            self:x(actuals.EdgeBuffer)
                        else
                            self:x(0)
                        end
                        self:settext(item.Name)
                    end
                end,
            }
        }
    end

    local rightAreaWidth = actuals.MainDisplayWidth - (actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
    local t = Def.ActorFrame {
        Name = "MenuContainer",
        BeginCommand = function(self)
            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                if event.type == "InputEventType_Release" then return end

                local gameButton = event.button
                local key = event.DeviceInput.button
                local up = gameButton == "Up" or gameButton == "MenuUp"
                local down = gameButton == "Down" or gameButton == "MenuDown"
                local right = gameButton == "MenuRight" or gameButton == "Right"
                local left = gameButton == "MenuLeft" or gameButton == "Left"
                local enter = gameButton == "Start"
                local back = key == "DeviceButton_escape"

                if up or left then
                    moveCursor(-1)
                    self:playcommand("SelectCurrent")
                elseif down or right then
                    moveCursor(1)
                    self:playcommand("SelectCurrent")
                elseif enter then
                    self:playcommand("SelectCurrent")
                elseif back then
                    SCREENMAN:GetTopScreen():Cancel()
                end
            end)
            self:playcommand("UpdateCursor")
        end,
        Def.Quad {
            Name = "ScrollBar",
            InitCommand = function(self)
                self:zoomto(actuals.ScrollerWidth, actuals.MainDisplayHeight / maxPage)
                self:halign(0):valign(0)
                self:diffusealpha(0.6)
            end,
            UpdatePageMessageCommand = function(self)
                self:finishtweening()
                self:smooth(animationSeconds)
                self:y(actuals.MainDisplayHeight / maxPage * (page-1))
            end,
        },
        Def.Quad {
            Name = "MouseScrollArea",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:diffusealpha(0)
                self:zoomto(actuals.ScrollerWidth + actuals.ListWidth, actuals.MainDisplayHeight)
            end,
            MouseScrollMessageCommand = function(self, params)
                if isOver(self) then
                    if params.direction == "Up" then
                        movePage(-1)
                    else
                        movePage(1)
                    end
                end
            end,
        },
        Def.ActorFrame {
            Name = "SelectedItemContainer",
            InitCommand = function(self)
                self:x(actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
                -- make empty defaults load
                self:playcommand("UpdateSelectedItem")
            end,
            SelectedItemMessageCommand = function(self, params)
                self:playcommand("UpdateSelectedItem", params)
            end,

            LoadFont("Menu Normal") .. {
                Name = "Name",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer)
                    self:zoom(titleTextSize)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.Name)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:maxwidth(((rightAreaWidth / 2) - actuals.EdgeBuffer) / titleTextSize)
                        else
                            self:maxwidth((rightAreaWidth - actuals.EdgeBuffer) / titleTextSize)
                        end
                    else
                        self:settext("")
                    end
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "ShortDescription",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer2)
                    self:skewx(-0.15)
                    self:zoom(subtitleTextSize)
                    self:maxheight((actuals.TopBuffer3 - actuals.TopBuffer2) / subtitleTextSize - textZoomFudge * 5)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.ShortDescription)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:wrapwidthpixels(((rightAreaWidth / 2) - actuals.EdgeBuffer) / subtitleTextSize)
                        else
                            self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer * 2) / subtitleTextSize)
                        end
                    else
                        self:settext("")
                    end
                end,
            },
            LoadFont("Menu Normal") .. {
                Name = "Paragraph",
                InitCommand = function(self)
                    self:halign(0):valign(0)
                    self:xy(actuals.EdgeBuffer, actuals.TopBuffer3)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        self:settext(def.Description)
                        self:zoom(descTextSize)
                        self:maxheight((actuals.MainDisplayHeight - actuals.TopBuffer3 - actuals.EdgeBuffer) / descTextSize)

                        if def.Image ~= nil and def.Image ~= "" then
                            self:wrapwidthpixels(((rightAreaWidth / 2) - actuals.EdgeBuffer) / descTextSize)
                        else
                            self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer * 2) / descTextSize)
                        end
                    else
                        self:zoom(subtitleTextSize)
                        self:settext("Select an item on the left to get info.\nScroll through the list for more categories.")
                        self:wrapwidthpixels((rightAreaWidth - actuals.EdgeBuffer) / subtitleTextSize)
                    end
                end,
            },
            UIElements.SpriteButton(1, 1, nil) .. {
                Name = "Image",
                InitCommand = function(self)
                    self:valign(0)
                    self:xy(rightAreaWidth / 4 * 3, actuals.TopBuffer)
                end,
                UpdateSelectedItemCommand = function(self, params)
                    if params and params.def ~= nil then
                        local def = params.def
                        if def.Image ~= nil and def.Image ~= "" then
                            self:diffusealpha(1)
                            self:Load(def.Image)
                            local h = self:GetHeight()
                            local w = self:GetWidth()
                            local allowedHeight = actuals.MainDisplayHeight - (actuals.TopBuffer * 2)
                            local allowedWidth = rightAreaWidth - (actuals.EdgeBuffer + actuals.IconExitWidth)
                            if h >= allowedHeight and w >= allowedWidth then
                                if h * (allowedWidth / allowedHeight) >= w then
                                    self:zoom(allowedHeight / h)
                                else
                                    self:zoom(allowedWidth / w)
                                end
                            elseif h >= allowedHeight then
                                self:zoom(allowedHeight / h)
                            elseif w >= allowedWidth then
                                self:zoom(allowedWidth / w)
                            else
                                self:zoom(1)
                            end
                        else
                            self:diffusealpha(0)
                        end
                    else
                        self:diffusealpha(0)
                    end
                end,
            },
        }
    }

    for i = 1, itemsVisible do
        t[#t+1] = menuItem(i)
    end
    return t
end


local t = Def.ActorFrame {
    Name = "HelpDisplayFile",

    Def.ActorFrame {
        Name = "InfoBoxFrame",
        InitCommand = function(self)
            self:xy(actuals.InfoLeftGap, actuals.InfoTopGap)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.InfoWidth, actuals.InfoHeight)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Sprite {
            Name = "Logo",
            Texture = THEME:GetPathG("", "Logo"),
            InitCommand = function(self)
                self:xy(actuals.ScrollerWidth + (actuals.ListWidth / 2), actuals.InfoHeight / 2)
                self:zoomto(logoW, logoH)
            end
        },
        LoadColorFont("Menu Bold") .. {
            Name = "Text",
            InitCommand = function(self)
                local textw = actuals.InfoWidth - (actuals.ScrollerWidth + actuals.ListWidth + actuals.SeparationGapWidth)
                local textx = actuals.InfoWidth - textw / 2
                self:xy(textx, actuals.InfoHeight/2)
                self:zoom(infoTextSize)
                self:maxheight((actuals.InfoHeight - (actuals.InfoVerticalBuffer*2)) / infoTextSize)
                self:wrapwidthpixels(textw / infoTextSize)
                self:settext("Help")
            end,
            SelectedItemMessageCommand = function(self, params)
                if params and params.category ~= nil then
                    self:settext(params.category)
                else
                    self:settext("Help")
                end
            end
        },
    },
    Def.ActorFrame {
        Name = "MainDisplayFrame",
        InitCommand = function(self)
            self:xy(actuals.MainDisplayLeftGap, actuals.MainDisplayTopGap)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:zoomto(actuals.MainDisplayWidth, actuals.MainDisplayHeight)
                self:diffuse(color("0,0,0"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Quad {
            Name = "Separator",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:x(actuals.ScrollerWidth + actuals.ListWidth)
                self:zoomto(actuals.SeparationGapWidth, actuals.MainDisplayHeight)
                self:diffuse(color("1,1,1"))
                self:diffusealpha(0.2)
            end,
        },
        helpMenu() .. {
            InitCommand = function(self)
                self:xy(0, 0)
            end,
        },
        UIElements.SpriteButton(1, 1, THEME:GetPathG("", "exit")) .. {
            Name = "Exit",
            InitCommand = function(self)
                self:valign(0):halign(1)
                self:xy(actuals.MainDisplayWidth - actuals.InfoVerticalBuffer/4, actuals.InfoVerticalBuffer/4)
                self:zoomto(actuals.IconExitWidth, actuals.IconExitHeight)
            end,
            MouseDownCommand = function(self, params)
                SCREENMAN:GetTopScreen():Cancel()
                TOOLTIP:Hide()
            end,
            MouseOverCommand = function(self, params)
                self:diffusealpha(buttonHoverAlpha)
                TOOLTIP:SetText("Exit")
                TOOLTIP:Show()
            end,
            MouseOutCommand = function(self, params)
                self:diffusealpha(1)
                TOOLTIP:Hide()
            end,
        },
    },
    
}

return t
