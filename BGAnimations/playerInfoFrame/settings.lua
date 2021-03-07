local ratios = {
    RightWidth = 782 / 1920,
    LeftWidth = 783 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    BottomLipHeight = 99 / 1080,

    EdgePadding = 12 / 1920, -- distance from edges for text and items
    
    --
    -- right options
    OptionTextWidth = 275 / 1920, -- left edge of text to edge of area for text
    OptionTextListTopGap = 21 / 1080, -- bottom of right top lip to top of text
    OptionTextBuffer = 7 / 1920, -- distance from end of width to beginning of selection frame
    OptionSelectionFrameWidth = 250 / 1920, -- allowed area for option selection

    -- for this area, this is the allowed height for all options including sub options
    -- when an option opens, it may only show as many sub options as there are lines after subtracting the amount of option categories
    -- so 1 category with 24 sub options has 25 lines
    -- 2 categories can then only have up to 23 sub options each to make 25 lines
    -- etc
    OptionAllottedHeight = 672 / 1080, -- from top of top option to bottom of bottom option
    NoteskinDisplayWidth = 240 / 1920, -- width of the text but lets fit the arrows within this
    NoteskinDisplayRightGap = 17 / 1920, -- distance from right edge of frame to right edge of display
    NoteskinDisplayReceptorTopGap = 29 / 1080, -- bottom of text to top of receptors
    NoteskinDisplayTopGap = 21 / 1080, -- bottom of right top lip to top of text
}

local actuals = {
    LeftWidth = ratios.LeftWidth * SCREEN_WIDTH,
    RightWidth = ratios.RightWidth * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    TopLipHeight = ratios.TopLipHeight * SCREEN_HEIGHT,
    BottomLipHeight = ratios.BottomLipHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
    OptionTextWidth = ratios.OptionTextWidth * SCREEN_WIDTH,
    OptionTextListTopGap = ratios.OptionTextListTopGap * SCREEN_HEIGHT,
    OptionTextBuffer = ratios.OptionTextBuffer * SCREEN_WIDTH,
    OptionSelectionFrameWidth = ratios.OptionSelectionFrameWidth * SCREEN_WIDTH,
    OptionAllottedHeight = ratios.OptionAllottedHeight * SCREEN_HEIGHT,
    NoteskinDisplayWidth = ratios.NoteskinDisplayWidth * SCREEN_WIDTH,
    NoteskinDisplayRightGap = ratios.NoteskinDisplayRightGap * SCREEN_WIDTH,
    NoteskinDisplayReceptorTopGap = ratios.NoteskinDisplayReceptorTopGap * SCREEN_HEIGHT,
    NoteskinDisplayTopGap = ratios.NoteskinDisplayTopGap * SCREEN_HEIGHT,
}

local visibleframeY = SCREEN_HEIGHT - actuals.Height
local animationSeconds = 0.1
local focused = false

local titleTextSize = 0.8
local explanationTextSize = 0.8
local textZoomFudge = 5

local choiceTextSize = 0.8
local buttonHoverAlpha = 0.6
local buttonActiveStrokeColor = color("0.85,0.85,0.85,0.8")

local maxExplanationTextLines = 2

local t = Def.ActorFrame {
    Name = "SettingsFile",
    InitCommand = function(self)
        -- lets just say uh ... despite the fact that this file might want to be portable ...
        -- lets ... just .... assume it always goes in the same place ... and the playerInfoFrame is the same size always
        self:y(visibleframeY)
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if params.tab and params.tab == "Settings" then
            --
            -- movement is delegated to the left and right halves
            -- right half immediately comes out
            -- left half comes out when selecting "Customize Playfield" or "Customize Keybinds"
            --
            self:diffusealpha(1)
            self:finishtweening()
            self:sleep(0.01)
            self:queuecommand("FinishFocusing")
        else
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(0)
            focused = false
        end
    end,
    FinishFocusingCommand = function(self)
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Settings")
    end,
}


local function leftFrame()

    local t = Def.ActorFrame {
        Name = "LeftFrame",
        InitCommand = function(self)
            self:x(0) -- yea
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:zoomto(actuals.LeftWidth, actuals.Height)
                self:diffuse(color("#111111"))
                self:diffusealpha(0.6)
            end
        },
        Def.Quad {
            Name = "TopLip",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:zoomto(actuals.LeftWidth, actuals.TopLipHeight)
                self:diffuse(color("#111111"))
                self:diffusealpha(0.6)
            end
        },
        LoadFont("Common Normal") .. {
            Name = "HeaderText",
            InitCommand = function(self)
                self:halign(0)
                self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
                self:zoom(titleTextSize)
                self:maxwidth((actuals.LeftWidth - actuals.EdgePadding*2) / titleTextSize - textZoomFudge)
                self:settext("Customize {x}")
            end
        }
    }


    return t
end

local function rightFrame()

    local t = Def.ActorFrame {
        Name = "RightFrame",
        InitCommand = function(self)
            self:x(SCREEN_WIDTH - actuals.RightWidth)
        end,

        Def.Quad {
            Name = "BG",
            InitCommand = function(self)
                self:valign(0):halign(0)
                self:zoomto(actuals.RightWidth, actuals.Height)
                self:diffuse(color("#111111"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Quad {
            Name = "TopLip",
            InitCommand = function(self)
                -- height is double normal top lip
                self:valign(0):halign(0)
                self:zoomto(actuals.RightWidth, actuals.TopLipHeight * 2)
                self:diffuse(color("#111111"))
                self:diffusealpha(0.6)
            end,
        },
        Def.Quad {
            Name = "BottomLip",
            InitCommand = function(self)
                -- height is double normal top lip
                self:valign(1):halign(0)
                self:y(actuals.Height)
                self:zoomto(actuals.RightWidth, actuals.BottomLipHeight)
                self:diffuse(color("#111111"))
                self:diffusealpha(0.6)
            end,
        },
        LoadFont("Common Normal") .. {
            Name = "HeaderText",
            InitCommand = function(self)
                self:halign(0)
                self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
                self:zoom(titleTextSize)
                self:maxwidth((actuals.RightWidth - actuals.EdgePadding*2) / titleTextSize - textZoomFudge)
                self:settext("Options")
            end
        },
        LoadFont("Common Normal") .. {
            Name = "ExplanationText",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:xy(actuals.EdgePadding, actuals.Height - actuals.BottomLipHeight + actuals.EdgePadding)
                self:zoom(explanationTextSize)
                --self:maxwidth((actuals.RightWidth - actuals.EdgePadding*2) / explanationTextSize - textZoomFudge)
                self:wrapwidthpixels((actuals.RightWidth - actuals.EdgePadding * 2) / explanationTextSize)
                self:maxheight((actuals.BottomLipHeight - actuals.EdgePadding * 2) / explanationTextSize)
                self:settext("Explanations about optoins go here and im writing a long sentence so that the demonstration of automatic line breaks is completed.")
            end
        }
    }

    -- -----
    -- Utility functions for options not necessarily needed for global use in /Scripts (could easily be put there instead though)
    
    -- set any mod as part of PlayerOptions at all levels in one easy function
    local function setPlayerOptionsModValueAllLevels(funcname, ...)
        -- you give a funcname like MMod, XMod, CMod and it just works
        local poptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
        local stoptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Stage")
        local soptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Song")
        local coptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Current")
        poptions[funcname](poptions, ...)
        stoptions[funcname](stoptions, ...)
        soptions[funcname](soptions, ...)
        coptions[funcname](coptions, ...)
    end
    -- set any mod as part of SongOptions at all levels in one easy function
    local function setSongOptionsModValueAllLevels(funcname, ...)
        -- you give a funcname like MusicRate and it just works
        local poptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred")
        local stoptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Stage")
        local soptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Song")
        local coptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Current")
        poptions[funcname](poptions, ...)
        stoptions[funcname](stoptions, ...)
        soptions[funcname](soptions, ...)
        coptions[funcname](coptions, ...)
    end
    -- alias for getting "current" PlayerOptions
    local function getPlayerOptions()
        return GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
    end
    -- alias for getting "current" SongOptions
    local function getSongOptions()
        return GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred")
    end

    --- for Speed Mods -- this has been adapted from the fallback script which does speed and mode at once
    local function getSpeedModeFromPlayerOptions()
        local poptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
        if poptions:MaxScrollBPM() > 0 then
            return "M"
        elseif poptions:TimeSpacing() > 0 then
            return "C"
        else
            return "X"
        end
    end
    local function getSpeedValueFromPlayerOptions()
        local poptions = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
        if poptions:MaxScrollBPM() > 0 then
            return math.round(poptions:MaxScrollBPM())
        elseif poptions:TimeSpacing() > 0 then
            return math.round(poptions:ScrollBPM())
        else
            return math.round(poptions:ScrollSpeed() * 100)
        end
    end

    --
    -- -----

    -- -----
    -- Extra data for cross-option interaction
    --
    local optionData = {
        speedMod = {
            speed = GetSpeedValueFromPlayerOptions(),
            mode = GetSpeedModeFromPlayerOptions(),
        },
        noteSkins = {
            names = NOTESKIN:GetNoteSkinNames(),
        }
    }
    --
    -- -----

    -- -----
    -- Extra utility functions that require optionData to be initialized first
    local function setSpeedValueFromOptionData()
        local mode = optionData.speedMod.mode
        local speed = optionData.speedMod.speed
        if mode == "X" then
            -- the way we store stuff, xmod must divide by 100
            -- theres no quirk to it, thats just because we store the number as an int (not necessarily an int but yeah)
            -- so 0.01x XMod would be a CMod of 1 -- this makes even more sense if you just think about it
            setPlayerOptionsModValueAllLevels("XMod", speed/100)
        elseif mode == "C" then
            setPlayerOptionsModValueAllLevels("CMod", speed)
        elseif mode == "M" then
            setPlayerOptionsModValueAllLevels("MMod", speed)
        end
    end
    --
    -- -----

    -- the names and order of the option pages
    -- these values must correspond to the keys of optionPageCategoryLists
    local pageNames = {
        "Player",
        "Graphics",
        "Sound",
        "Input",
        "Profiles",
    }

    -- mappings of option page names to lists of categories
    -- the keys in this table are option pages
    -- the values are tables -- the categories of each page in that order
    -- each category corresponds to a key in optionDefs
    -- the options of each category are in the order of the value tables in optionDefs
    local optionPageCategoryLists = {
        Player = {
            "Essential Options",
            "Appearance Options",
            "Invalidating Options",
        },
        Graphics = {
            "Global Options",
            "Appearance Options",
        },
        Sound = {
            "Sound Options",
        },
        Input = {
            "Input Options",
        },
        Profiles = {
            "Profile Options",
        },
    }

    -- the mother of all tables.
    -- this is each option definition for every single option present in the right frame
    -- mapping option categories to lists of options
    -- LIMITATIONS: A category cannot have more sub options than the max number of lines minus the number of categories.
    --  example: 25 lines? 2 categories? up to 23 options per category.
    -- OPTION DEFINITION EXAMPLE: 
    --[[
        {
            Name = "option name" -- display name for the option
            Type = "" -- determines how to generate the actor to display the choices
            AssociatedOption = "other option name" -- runs the index getter for this option when a choice is selected
            Choices = { -- option choice definitions -- each entry is another table -- if no choices are defined, visible choice comes from ChoiceIndexGetter
                {
                    Name = "choice1" -- display name for the choice
                    ChosenFunction = function() end -- what happens when choice is PICKED (not hovered)
                },
                {
                    Name = "choice2"
                    ...
                },
                ...
            },
            Directions = {
                -- table of direction functions -- these define what happens for each pressed direction button
                -- most options have only Left and Right
                -- if these functions are undefined and required by the option type, a default function moves the index of the choice rotationally
                -- some option types may allow for more directions or direction multipliers
                -- if Toggle is defined, this function is used for all direction presses
                Left = function() end,
                Right = function() end,
                Toggle = function() end, --- OPTIONAL -- WILL REPLACE ALL DIRECTION FUNCTIONALITY IF PRESENT
                ...
            },
            ChoiceIndexGetter = function() end -- a function to run to get the choice index or text
            ChoiceGenerator = function() end -- an OPTIONAL function for generating the choices table if too long to write out (return a table)
        }
    ]]
    local optionDefs = {
        -----
        -- PLAYER OPTIONS
        ["Essential Options"] = {
            {
                Name = "Scroll Type",
                Type = "",
                AssociatedOption = "Scroll Speed",
                Choices = {
                    {
                        Name = "XMod",
                    },
                    {
                        Name = "CMod",
                    },
                    {
                        Name = "MMod",
                    },
                },
                Directions = {
                    Left = function()
                        -- traverse list left, set the speed mod again
                        -- order:
                        -- XMOD - CMOD - MMOD
                        local mode = optionData.speedMod.mode
                        if mode == "C" then
                            mode = "X"
                        elseif mode == "M" then
                            mode = "C"
                        elseif mode == "X" then
                            mode = "M"
                        end
                        optionData.speedMod.mode = mode
                        setSpeedValueFromOptionData()
                    end,
                    Right = function()
                        -- traverse list right, set the speed mod again
                        -- order:
                        -- XMOD - CMOD - MMOD
                        local mode = optionData.speedMod.mode
                        if mode == "C" then
                            mode = "M"
                        elseif mode == "M" then
                            mode = "X"
                        elseif mode == "X" then
                            mode = "C"
                        end
                        optionData.speedMod.mode = mode
                        setSpeedValueFromOptionData()
                    end,
                },
                ChoiceIndexGetter = function()
                    local mode = optionData.speedMod.mode
                    if mode == "X" then return 1
                    elseif mode == "C" then return 2
                    elseif mode == "M" then return 3 end
                end,
            },
            {
                Name = "Scroll Speed",
                Type = "",
                Directions = {
                    Left = function(multiplier)
                        local increment = -1
                        if multiplier then increment = -50 end
                        optionData.speedMod.speed = optionData.speedMod.speed + increment
                        setSpeedValueFromOptionData()
                    end,
                    Right = function(multiplier)
                        local increment = 1
                        if multiplier then increment = 50 end
                        optionData.speedMod.speed = optionData.speedMod.speed + increment
                        setSpeedValueFromOptionData()
                    end,
                },
                ChoiceIndexGetter = function()
                    local mode = optionData.speedMod.mode
                    local speed = optionData.speedMod.speed
                    if mode == "X" then
                        return mode .. notShit.round((speed/100), 2)
                    else
                        return mode .. speed
                    end
                end,
            },
            {
                Name = "Scroll Direction",
                Type = "",
                Choices = {
                    {
                        Name = "Upscroll",
                    },
                    {
                        Name = "Downscroll",
                    },
                },
                Directions = {
                    Toggle = function()
                        if getPlayerOptions():UsingReverse() then
                            -- 1 is 100% reverse which means on
                            setPlayerOptionsModValueAllLevels("Reverse", 1)
                        else
                            -- 0 is 0% reverse which means off
                            setPlayerOptionsModValueAllLevels("Reverse", 0)
                        end
                    end,
                },
                ChoiceIndexGetter = function()
                    if getPlayerOptions:UsingReverse() then
                        return 2
                    else
                        return 1
                    end
                end,
            },
            {
                Name = "Noteskin",
                Type = "",
                ChoiceIndexGetter = function()
                    local currentSkinName = getPlayerOptions():NoteSkin()
                    for i, name in ipairs(optionData.noteSkins.names) do
                        if name == currentSkinName then
                            return i
                        end
                    end
                    -- if function gets this far, look for the default skin
                    currentSkinName = THEME:GetMetric("Common", "DefaultNoteSkinName")
                    for i, name in ipairs(optionData.noteSkins.names) do
                        if name == currentSkinName then
                            return i
                        end
                    end
                    -- if function gets this far, cant find anything so just return the first skin
                    return 1
                end,
                ChoiceGenerator = function()
                    local o = {}
                    local skinNames = NOTESKIN:GetNoteSkinNames()
                    for i, name in ipairs(skinNames) do
                        o[#o+1] = {
                            Name = name,
                        }
                    end
                    table.sort(
                        o,
                        function(a, b)
                            return a.Name:lower() < b.Name:lower()
                        end)

                    return o
                end,
            },
            {
                Name = "Receptor Size",
                Type = "",
                ChoiceIndexGetter = function()
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i = 1, 200 do
                        o[#o+1] = {
                            Name = tostring(i) .. "%",
                            ChosenFunction = function()
                                -- set mini?
                            end,
                        }
                    end
                    return o
                end,
            },
            {
                Name = "Judge Difficulty",
                Type = "",
                ChoiceIndexGetter = function()
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i = 4, 8 do
                        o[#o+1] = {
                            Name = tostring(i),
                            ChosenFunction = function()
                                -- set judge
                            end,
                        }
                    end
                    o[#o+1] = {
                        Name = "Justice",
                        ChosenFunction = function()
                            -- sets j9
                        end,
                    }
                    return o
                end,
            },
            {
                Name = "Global Offset",
                Type = "",
                ChoiceIndexGetter = function()
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i = -100, 100 do
                        local r = i * .01
                        o[#o+1] = {
                            Name = tostring(r),
                            ChosenFunction = function()
                                -- set global offset to r
                            end,
                        }
                    end
                    return o
                end,
            },
            {
                Name = "Visual Delay",
                Type = "",
                ChoiceIndexGetter = function()
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i = -100, 100 do
                        local r = i * .01
                        o[#o+1] = {
                            Name = tostring(r),
                            ChosenFunction = function()
                                -- set visual delay to r
                            end,
                        }
                    end
                    return o
                end,
            },
            {
                Name = "Game Mode",
                Type = "",
                ChoiceIndexGetter = function()
                end,
                ChoiceGenerator = function()
                    local o = {}
                    -- get a list of game modes that are playable
                    return o
                end,
            },
            {
                Name = "Fail Type",
                Type = "",
                ChoiceIndexGetter = function()
                end,
                ChoiceGenerator = function()
                    -- get the list of fail types
                end,
            },
            {
                Name = "Customize Playfield",
                Type = "",
                ChoiceIndexGetter = function() return 1 end,
                Choices = {
                    {
                        Name = "Customize Playfield",
                        ChosenFunction = function()
                            -- activate customize gameplay screen
                        end,
                    }
                }
            },
            {
                Name = "Customize Keybinds",
                Type = "",
                ChoiceIndexGetter = function() return 1 end,
                Choices = {
                    {
                        Name = "Customize Keybinds",
                        ChosenFunction = function()
                            -- activate keybind screen
                        end,
                    }
                }
            },
        },
        ["Appearance Options"] = {
            {
                Name = "Appearance",
                Type = "",
            },
            {
                Name = "Perspective",
                Type = "",
            },
            {
                Name = "Turn",
                Type = "",
            },
            {
                Name = "Hidenote Judgment",
                Type = "",
            },
            {
                Name = "Default Centered NoteField",
                Type = "",
            },
            {
                Name = "NoteField BG Opacity",
                Type = "",
            },
            {
                Name = "Background Brightness",
                Type = "",
            },
            {
                Name = "Background Type",
                Type = "",
            },
            {
                Name = "Replay Mod Emulation",
                Type = "",
            },
        },
        ["Invalidating Options"] = {
            {
                Name = "Mines",
                Type = "",
            },
            {
                Name = "more",
                Type = "",
            },
        },
        --
        -----
        -- GRAPHICS OPTIONS
        ["Global Options"] = {
            {
                Name = "Language",
                Type = "",
            },
            {
                Name = "Display Mode",
                Type = "",
            },
            {
                Name = "Aspect Ratio",
                Type = "",
            },
            {
                Name = "Display Resolution",
                Type = "",
            },
            {
                Name = "Refresh Rate",
                Type = "",
            },
            {
                Name = "Fullscreen Type",
                Type = "",
            },
            {
                Name = "Display Color",
                Type = "",
            },
            {
                Name = "Force High Resolution Textures",
                Type = "",
            },
            {
                Name = "Texture Resolution",
                Type = "",
            },
            {
                Name = "Texture Color",
                Type = "",
            },
            {
                Name = "Movie Color",
                Type = "",
            },
            {
                Name = "VSync",
                Type = "",
            },
            {
                Name = "Instant Search",
                Type = "",
            },
            {
                Name = "Fast Note Rendering",
                Type = "",
            },
            {
                Name = "Show Stats",
                Type = "",
            },
        },
        ["Appearance Options"] = {
            {
                Name = "Theme",
                Type = "",
            },
            {
                Name = "Music Wheel Position",
                Type = "",
            },
            {
                Name = "Show Backgrounds",
                Type = "",
            },
            {
                Name = "Toasties",
                Type = "",
            },
            {
                Name = "Music Visualizer",
                Type = "",
            },
            {
                Name = "Mid Grades",
                Type = "",
            },
            {
                Name = "SSRNorm Sort",
                Type = "",
            },
            {
                Name = "Show Lyrics",
                Type = "",
            },
            {
                Name = "Transliteration",
                Type = "",
            },
            {
                Name = "Tip Type",
                Type = "",
            },
            {
                Name = "Set BG Fit Mode",
                Type = "",
            },
            {
                Name = "Color Config",
                Type = "",
            },
            {
                Name = "Overscan Correction",
                Type = "",
            },
        },
        --
        -----
        -- SOUND OPTIONS
        ["Sound Options"] = {
            {
                Name = "Volume",
                Type = "",
            },
            {
                Name = "Menu Sounds",
                Type = "",
            },
            {
                Name = "Mine Sounds",
                Type = "",
            },
            {
                Name = "Pitch on Rates",
                Type = "",
            },
            {
                Name = "Calibrate Audio Sync",
                Type = "",
            },
        },
        --
        -----
        -- INPUT OPTIONS
        ["Input Options"] = {
            {
                Name = "everthin in advance input optns",
                Type = "",
            },
            {
                Name = "Customize Keybinds",
                Type = "",
            },
            {
                Name = "Test Input",
                Type = "",
            },
        },
        --
        -----
        -- PROFILE OPTIONS
        ["Profile Options"] = {
            {
                Name = "Create Profile",
                Type = "",
            },
            {
                Name = "Rename Profile",
                Type = "",
            },
        },
    }
    -- check for choice generators on any option definitions and execute them
    for categoryName, categoryDefinition in pairs(optionDefs) do
        for i, optionDef in ipairs(categoryDefinition) do
            if optionDef.Choices == nil and optionDef.ChoiceGenerator ~= nil then
                optionDefs[categoryName][i].Choices = optionDef.ChoiceGenerator()
            end
        end
    end

    local function createOptionPageChoices()
        local selectedIndex = 1
    
        local function createChoice(i)
            return UIElements.TextButton(1, 1, "Common Normal") .. {
                Name = "ButtonTab_"..pageNames[i],
                InitCommand = function(self)
                    local txt = self:GetChild("Text")
                    local bg = self:GetChild("BG")
    
                    -- this position is the center of the text
                    -- divides the space into slots for the choices then places them half way into them
                    -- should work for any count of choices
                    -- and the maxwidth will make sure they stay nonoverlapping
                    self:x((actuals.RightWidth / #pageNames) * (i-1) + (actuals.RightWidth / #pageNames / 2))
                    txt:zoom(choiceTextSize)
                    txt:maxwidth(actuals.RightWidth / #pageNames / choiceTextSize - textZoomFudge)
                    txt:settext(pageNames[i])
                    bg:zoomto(actuals.RightWidth / #pageNames, actuals.TopLipHeight)
                end,
                UpdateSelectedIndexCommand = function(self)
                    local txt = self:GetChild("Text")
                    if selectedIndex == i then
                        txt:strokecolor(buttonActiveStrokeColor)
                    else
                        txt:strokecolor(color("0,0,0,0"))
                    end
                end,
                ClickCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "OnMouseDown" then
                        selectedIndex = i
                        MESSAGEMAN:Broadcast("GeneralTabSet", {tab = i})
                        self:GetParent():playcommand("UpdateSelectedIndex")
                    end
                end,
                RolloverUpdateCommand = function(self, params)
                    if self:IsInvisible() then return end
                    if params.update == "in" then
                        self:diffusealpha(buttonHoverAlpha)
                    else
                        self:diffusealpha(1)
                    end
                end
            }
        end
        local t = Def.ActorFrame {
            Name = "Choices",
            InitCommand = function(self)
                self:y(actuals.TopLipHeight * 1.5)
                self:playcommand("UpdateSelectedIndex")
            end,
            BeginCommand = function(self)
                local snm = SCREENMAN:GetTopScreen():GetName()
                local anm = self:GetName()

                CONTEXTMAN:RegisterToContextSet(snm, "Settings", anm)
                CONTEXTMAN:ToggleContextSet(snm, "Settings", false)
    
                -- enable the possibility to press the keyboard to switch tabs
                SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                    -- if locked out, dont allow
                    if not CONTEXTMAN:CheckContextSet(snm, "Settings") then return end
                    if event.type == "InputEventType_FirstPress" then
                        -- nothing
                        -- will use arrows to place cursor
                        -- dummy complex probably not going to be in this spot
                    end
                end)
            end
        }
        for i = 1, #pageNames do
            t[#t+1] = createChoice(i)
        end
        return t
    end

    t[#t+1] = createOptionPageChoices()

    return t
end

t[#t+1] = leftFrame()
t[#t+1] = rightFrame()

return t