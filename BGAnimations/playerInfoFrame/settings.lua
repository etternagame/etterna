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
    OptionBigTriangleHeight = 19 / 1080, -- visually the width most of the time because the triangles are usually turned
    OptionBigTriangleWidth = 20 / 1920,
    OptionSmallTriangleHeight = 12 / 1080,
    OptionSmallTriangleWidth = 13 / 1920,
    OptionSmallTriangleGap = 2 / 1920,
    OptionChoiceDirectionGap = 7 / 1920, -- gap between direction arrow pairs and between direction arrows and choices

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
    OptionBigTriangleHeight = ratios.OptionBigTriangleHeight * SCREEN_HEIGHT,
    OptionBigTriangleWidth = ratios.OptionBigTriangleWidth * SCREEN_WIDTH,
    OptionSmallTriangleHeight = ratios.OptionSmallTriangleHeight * SCREEN_HEIGHT,
    OptionSmallTriangleWidth = ratios.OptionSmallTriangleWidth * SCREEN_WIDTH,
    OptionSmallTriangleGap = ratios.OptionSmallTriangleGap * SCREEN_WIDTH,
    OptionChoiceDirectionGap = ratios.OptionChoiceDirectionGap * SCREEN_WIDTH,
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

local optionTitleTextSize = 0.7
local optionChoiceTextSize = 0.7
-- basically our font is bad and not on the baseline or equivalent to what a BitMapText:isOver says it is, so this is a modifier to the invisible text button size
-- could also be moved even further for whatever accessibility concerns
local textButtonHeightFudgeScalarMultiplier = 1.6
local optionRowAnimationSeconds = 0.15

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

    -- for convenience to generate a choice table for a float interface setting
    local function floatSettingChoice(visibleName, funcName, enabledValue, offValue)
        return {
            Name = visibleName,
            CheckFunction = function()
                return getPlayerOptions()[funcName] ~= offValue
            end,
            OnFunction = function()
                setPlayerOptionsModValueAllLevels(funcName, enabledValue)
            end,
            OffFunction = function()
                setPlayerOptionsModValueAllLevels(funcName, offValue)
            end,
        }
    end

    -- for convenience to generate a choice table for a boolean interface setting
    local function booleanSettingChoice(visibleName, funcName)
        return {
            Name = visibleName,
            CheckFunction = function()
                return getPlayerOptions()[funcName] == true
            end,
            OnFunction = function()
                setPlayerOptionsModValueAllLevels(funcName, true)
            end,
            OffFunction = function()
                setPlayerOptionsModValueAllLevels(funcName, false)
            end,
        }
    end

    -- for convenience to generate a direction table for a setting which goes in either direction and wraps via PREFSMAN
    -- if the max value is reached, the min value is the next one
    local function preferenceIncrementDecrementDirections(preferenceName, minValue, maxValue, increment)
        return {
            Left = function()
                local x = clamp(PREFSMAN:GetPreference(preferenceName), minValue, maxValue)
                x = x - increment
                if x < minValue then x = maxValue end
                PREFSMAN:SetPreference(preferenceName, notShit.round(x, 3))
            end,
            Right = function()
                local x = clamp(PREFSMAN:GetPreference(preferenceName), minValue, maxValue)
                x = x + increment
                if x > maxValue then x = minValue end
                PREFSMAN:SetPreference(preferenceName, notShit.round(x, 3))
            end,
        }
    end

    local function basicNamedPreferenceChoice(preferenceName, displayName, chosenValue)
        return {
            Name = displayName,
            ChosenFunction = function()
                PREFSMAN:SetPreference(preferenceName, chosenValue)
            end,
        }
    end
    local function preferenceToggleDirections(preferenceName, trueValue, falseValue)
        return {
            Toggle = function()
                if PREFSMAN:GetPreference(preferenceName) == trueValue then
                    PREFSMAN:SetPreference(preferenceName, falseValue)
                else
                    PREFSMAN:SetPreference(preferenceName, trueValue)
                end
            end,
        }
    end
    local function preferenceToggleIndexGetter(preferenceName, oneValue)
        -- oneValue is what we expect for choice index 1 (the first one)
        return function()
            if PREFSMAN:GetPreference(preferenceName) == oneValue then
                return 1
            else
                return 2
            end
        end
    end
    -- convenience to create a Choices table for a variable number of choice names
    local function choiceSkeleton(...)
        local o = {}
        for _, name in ipairs({...}) do
            o[#o+1] = {
                Name = name,
            }
        end
        return o
    end

    local function initDisplayResolutions()
        local resolutions = {}
        local displaySpecs = GetDisplaySpecs()
        for _, spec in ipairs(displaySpecs) do
            for __, mode in ipairs(spec:GetSupportedModes()) do
                resolutions[#resolutions+1] = {
                    w = mode:GetWidth(),
                    h = mode:GetHeight(),
                }
            end
        end
        return resolutions
    end

    --
    -- -----

    -- -----
    -- Extra data for option temporary storage or cross option interaction
    --
    local playerConfigData = playerConfig:get_data()
    local themeConfigData = themeConfig:get_data()
    local displaySpecs = GetDisplaySpecs()
    local optionData = {
        speedMod = {
            speed = getSpeedValueFromPlayerOptions(),
            mode = getSpeedModeFromPlayerOptions(),
        },
        noteSkins = {
            names = NOTESKIN:GetNoteSkinNames(),
        },
        receptorSize = playerConfigData.ReceptorSize,
        gameMode = {
            modes = GAMEMAN:GetEnabledGames(),
            current = GAMESTATE:GetCurrentGame():GetName(),
        },
        screenFilter = playerConfigData.ScreenFilter,
        language = {
            list = THEME:GetLanguages(),
            current = THEME:GetCurLanguage(),
        },
        instantSearch = themeConfigData.global.InstantSearch,
        wheelPosition = themeConfigData.global.WheelPosition,
        showBackgrounds = themeConfigData.global.ShowBackgrounds,
        showVisualizer = themeConfigData.global.ShowVisualizer,
        tipType = themeConfigData.global.TipType,
        display = {
            ratios = { -- hardcoded aspect ratio list
                {n = 3, d = 4},
                {n = 1, d = 1},
                {n = 5, d = 4},
                {n = 4, d = 3},
                {n = 16, d = 10},
                {n = 16, d = 9},
                {n = 8, d = 3},
                {n = 21, d = 9}
            },
            refreshRates = { -- hardcoded refresh rate list (i dont know what im doing)
                -- if you put floats here it is your fault if something breaks but go nuts if you want
                --REFRESH_DEFAULT, -- skip this, it is hardcoded as the first value
                59,
                60,
                70,
                72,
                75,
                80,
                85,
                90,
                100,
                120,
                144,
                150,
                240,
            },
            -- displayspec generated "compatible" ratios
            dRatios = GetDisplayAspectRatios(displaySpecs),
            wRatios = GetWindowAspectRatios(),
            -- displayspec generated "compatible" resolutions
            resolutions = initDisplayResolutions(),
            loadedAspectRatio = PREFSMAN:GetPreference("DisplayAspectRatio")
        },
        pickedTheme = THEME:GetCurThemeName(),
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
    local function basicNamedOptionDataChoice(optionDataPropertyName, displayName, chosenValue)
        return {
            Name = displayName,
            ChosenFunction = function()
                optionData[optionDataPropertyName] = chosenValue
            end,
        }
    end
    local function optionDataToggleDirections(optionDataPropertyName, trueValue, falseValue)
        return {
            Toggle = function()
                if optionData[optionDataPropertyName] == trueValue then
                    optionData[optionDataPropertyName] = falseValue
                else
                    optionData[optionDataPropertyName] = trueValue
                end
            end,
        }
    end
    local function optionDataToggleIndexGetter(optionDataPropertyName, oneValue)
        -- oneValue is what we expect for choice index 1 (the first one)
        return function()
            if optionData[optionDataPropertyName] == oneValue then
                return 1
            else
                return 2
            end
        end
    end
    --
    -- -----

    local optionRowCount = 18
    local maxChoicesVisibleMultiChoice = 4

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
    -- each category corresponds to a key in optionDefs (must be unique keys -- values of these tables have to be globally unique)
    -- the options of each category are in the order of the value tables in optionDefs
    local optionPageCategoryLists = {
        Player = {
            "Essential Options",
            "Appearance Options",
            "Invalidating Options",
        },
        Graphics = {
            "Global Options",
            "Theme Options",
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
    -- TYPE LIST:
    --  SingleChoice            -- scrolls through choices
    --  SingleChoiceModifier    -- scrolls through choices, shows 2 sets of arrows for each direction, allowing multiplier
    --  MultiChoice             -- shows all options at once, selecting any amount of them
    --  Button                  -- it's a button. you press enter on it.
    --
    -- OPTION DEFINITION EXAMPLE: 
    --[[
        {
            Name = "option name" -- display name for the option
            Type = "type name" -- determines how to generate the actor to display the choices
            AssociatedOptions = {"other option name"} -- runs the index getter for these options when a choice is selected
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
            ChoiceIndexGetter = function() end -- a function to run to get the choice index or text, or return a table for multi selection options
            ChoiceGenerator = function() end -- an OPTIONAL function for generating the choices table if too long to write out (return a table)
            Explanation = "" -- an explanation that appears at the bottom of the screen
        }
    ]]
    local optionDefs = {
        -----
        -- PLAYER OPTIONS
        ["Essential Options"] = {
            {
                Name = "Scroll Type",
                Type = "SingleChoice",
                AssociatedOptions = {
                    "Scroll Speed",
                },
                Choices = choiceSkeleton("XMod", "CMod", "MMod"),
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
                Type = "SingleChoiceModifier",
                Directions = {
                    Left = function(multiplier)
                        local increment = -1
                        if multiplier then increment = -50 end
                        optionData.speedMod.speed = optionData.speedMod.speed + increment
                        if optionData.speedMod.speed <= 0 then optionData.speedMod.speed = 1 end
                        setSpeedValueFromOptionData()
                    end,
                    Right = function(multiplier)
                        local increment = 1
                        if multiplier then increment = 50 end
                        optionData.speedMod.speed = optionData.speedMod.speed + increment
                        if optionData.speedMod.speed <= 0 then optionData.speedMod.speed = 1 end
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
                Type = "SingleChoice",
                Choices = choiceSkeleton("Upscroll", "Downscroll"),
                Directions = {
                    Toggle = function()
                        if not getPlayerOptions():UsingReverse() then
                            -- 1 is 100% reverse which means on
                            setPlayerOptionsModValueAllLevels("Reverse", 1)
                        else
                            -- 0 is 0% reverse which means off
                            setPlayerOptionsModValueAllLevels("Reverse", 0)
                        end
                    end,
                },
                ChoiceIndexGetter = function()
                    if getPlayerOptions():UsingReverse() then
                        return 2
                    else
                        return 1
                    end
                end,
            },
            {
                Name = "Noteskin",
                Type = "SingleChoice",
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
                            ChosenFunction = function()
                                setPlayerOptionsModValueAllLevels("NoteSkin", name)
                            end,
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
                Type = "SingleChoice",
                Directions = {
                    Left = function()
                        local sz = optionData.receptorSize
                        sz = sz - 1
                        if sz < -200 then sz = 200 end
                        optionData.receptorSize = sz
                    end,
                    Right = function()
                        local sz = optionData.receptorSize
                        sz = sz + 1
                        if sz > 200 then sz = -200 end
                        optionData.receptorSize = sz
                    end,
                },
                ChoiceIndexGetter = function()
                    return optionData.receptorSize
                end,
            },
            {
                Name = "Judge Difficulty",
                Type = "SingleChoice",
                ChoiceIndexGetter = function()
                    local lowestJudgeDifficulty = 4
                    return GetTimingDifficulty() - (lowestJudgeDifficulty-1)
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i = 4, 8 do
                        o[#o+1] = {
                            Name = tostring(i),
                            ChosenFunction = function()
                                -- set judge
                                SetTimingDifficulty(i)
                            end,
                        }
                    end
                    o[#o+1] = {
                        Name = "Justice",
                        ChosenFunction = function()
                            -- sets j9
                            SetTimingDifficulty(9)
                        end,
                    }
                    return o
                end,
            },
            {
                Name = "Global Offset",
                Type = "SingleChoice",
                Directions = preferenceIncrementDecrementDirections("GlobalOffsetSeconds", -5, 5, 0.001),
                ChoiceIndexGetter = function()
                    return notShit.round(PREFSMAN:GetPreference("GlobalOffsetSeconds"), 3) .. "s"
                end,
            },
            {
                Name = "Visual Delay",
                Type = "SingleChoice",
                Directions = preferenceIncrementDecrementDirections("VisualDelaySeconds", -5, 5, 0.001),
                ChoiceIndexGetter = function()
                    return notShit.round(PREFSMAN:GetPreference("VisualDelaySeconds"), 3) .. "s"
                end,
            },
            {
                Name = "Game Mode",
                Type = "SingleChoice",
                ChoiceIndexGetter = function()
                    for i = 1, #optionData.gameMode.modes do
                        if optionData.gameMode.modes[i] == optionData.gameMode.current then
                            return i
                        end
                    end
                    return 1
                end,
                ChoiceGenerator = function()
                    local o = {}
                    for i, name in ipairs(optionData.gameMode.modes) do
                        o[#o+1] = {
                            Name = strCapitalize(name),
                            ChosenFunction = function()
                                --GAMEMAN:SetGame(name)
                                optionData.gameMode.current = name
                            end,
                        }
                    end
                    return o
                end,
            },
            {
                Name = "Fail Type",
                Type = "SingleChoice",
                ChoiceIndexGetter = function()
                    local failtypes = FailType
                    local failtype = getPlayerOptions():FailSetting()
                    for i, name in ipairs(failtypes) do
                        if name == failtype then return i end
                    end
                    return 1
                end,
                ChoiceGenerator = function()
                    -- get the list of fail types
                    local failtypes = FailType
                    local o = {}
                    for i, name in ipairs(failtypes) do
                        o[#o+1] = {
                            Name = THEME:GetString("OptionNames", ToEnumShortString(name)),
                            ChosenFunction = function()
                                setPlayerOptionsModValueAllLevels("FailSetting", name)
                            end,
                        }
                    end
                    return o
                end,
            },
            {
                Name = "Customize Playfield",
                Type = "Button",
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
                Type = "Button",
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
        --
        -----
        -- APPEARANCE OPTIONS
        ["Appearance Options"] = {
            {
                Name = "Appearance",
                Type = "MultiChoice",
                Choices = {
                    -- multiple choices allowed
                    floatSettingChoice("Hidden", "Hidden", 1, 0),
                    floatSettingChoice("HiddenOffset", "HiddenOffset", 1, 0),
                    floatSettingChoice("Sudden", "Sudden", 1, 0),
                    floatSettingChoice("SuddenOffset", "SuddenOffset", 1, 0),
                    floatSettingChoice("Stealth", "Stealth", 1, 0),
                    floatSettingChoice("Blink", "Blink", 1, 0)
                },
            },
            {
                Name = "Perspective",
                Type = "MultiChoice",
                Choices = {
                    -- the numbers in these defs are like the percentages you would put in metrics instead
                    -- 1 is 100%
                    -- Overhead does not use percentages.
                    -- adding an additional parameter to these functions does do something (approach rate) but is functionally useless
                    -- you are free to try these untested options for possible weird results:
                    -- setPlayerOptionsModValueAllLevels("Skew", x)
                    -- setPlayerOptionsModValueAllLevels("Tilt", x)
                    {
                        Name = "Overhead",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Overhead", true)
                        end,
                    },
                    {
                        Name = "Incoming",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Incoming", 1)
                        end,
                    },
                    {
                        Name = "Space",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Space", 1)
                        end,
                    },
                    {
                        Name = "Hallway",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Hallway", 1)
                        end,
                    },
                    {
                        Name = "Distant",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("Distant", 1)
                        end,
                    },
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    -- we unfortunately choose to hardcode these options and not allow an additional custom one
                    -- but the above choice definitions allow customizing the specific Perspective to whatever extent you want
                    if po:Overhead() then return 1
                    elseif po:Incoming() ~= nil then return 2
                    elseif po:Space() ~= nil then return 3
                    elseif po:Hallway() ~= nil then return 4
                    elseif po:Distant() ~= nil then return 5
                    end
                    return 1 -- 1 should be Overhead ....
                end,
            },
            {
                Name = "Mirror",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = {
                    Toggle = function()
                        local po = getPlayerOptions()
                        if po:Mirror() then
                            setPlayerOptionsModValueAllLevels("Mirror", false)
                        else
                            setPlayerOptionsModValueAllLevels("Mirror", true)
                        end
                    end,
                },
                ChoiceIndexGetter = function()
                    if getPlayerOptions():Mirror() then
                        return 1
                    else
                        return 2
                    end
                end,
            },
            {
                Name = "Hide Player UI",
                Type = "MultiChoice",
                Choices = {
                    floatSettingChoice("Hide Receptors", "Dark", 1, 0),
                    floatSettingChoice("Hide Judgment & Combo", "Blind", 1, 0),
                },
            },
            {
                Name = "Hidenote Judgment",
                Type = "SingleChoice",
                Choices = {
                    {
                        Name = "Miss",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "TNS_Miss")
                        end,
                    },
                    {
                        Name = "Bad",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "TNS_W5")
                        end,
                    },
                    {
                        Name = "Good",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "TNS_W4")
                        end,
                    },
                    {
                        Name = "Great",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "TNS_W3")
                        end,
                    },
                    {
                        Name = "Perfect",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "TNS_W2")
                        end,
                    },
                    {
                        Name = "Marvelous",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("MinTNSToHideNotes", "TNS_W1")
                        end,
                    },
                },
                ChoiceIndexGetter = function()
                    local opt = PREFSMAN:GetPreference("MinTNSToHideNotes")
                    if opt == "TNS_Miss" then return 1
                    elseif opt == "TNS_W5" then return 2
                    elseif opt == "TNS_W4" then return 3
                    elseif opt == "TNS_W3" then return 4
                    elseif opt == "TNS_W2" then return 5
                    elseif opt == "TNS_W1" then return 6
                    else
                        return 4 -- this is the default option so default to this
                    end
                end,
            },
            {
                Name = "Default Centered NoteField",
                Type = "SingleChoice",
                Choices = choiceSkeleton("Yes", "No"),
                Directions = preferenceToggleDirections("Center1Player", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("Center1Player", true),
            },
            {
                Name = "NoteField BG Opacity",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {}
                    for i = 0, 10 do
                        o[#o+1] = {
                            Name = notShit.round(i*10,0).."%",
                            ChosenFunction = function()
                                optionData.screenFilter = notShit.round(i / 10, 1)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local v = notShit.round(optionData.screenFilter, 1)
                    local ind = notShit.round(v * 10, 0) + 1
                    if ind > 0 and ind < 11 then -- this 11 should match the number of choices above
                        return ind
                    else
                        if ind <= 0 then
                            return 1
                        else
                            return 11
                        end
                    end
                end,
            },
            {
                Name = "Background Brightness",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {}
                    for i = 0, 10 do
                        o[#o+1] = {
                            Name = notShit.round(i*10,0).."%",
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("BGBrightness", notShit.round(i / 10, 1))
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local v = notShit.round(PREFSMAN:GetPreference("BGBrightness"))
                    local ind = notShit.round(v * 10, 0) + 1
                    if ind > 0 and ind < 11 then -- this 11 should match the nubmer of choices above
                        return ind
                    else
                        if ind <= 0 then
                            return 1
                        else
                            return 11
                        end
                    end
                end,
            },
            {
                Name = "Replay Mod Emulation",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("ReplaysUseScoreMods", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("ReplaysUseScoreMods", true),
            },
            {
                Name = "Extra Scroll Mods",
                Type = "MultiChoice",
                Choices = {
                    floatSettingChoice("Split", "Split", 1, 0),
                    floatSettingChoice("Alternate", "Alternate", 1, 0),
                    floatSettingChoice("Cross", "Cross", 1, 0),
                    floatSettingChoice("Centered", "Centered", 1, 0),
                },
            },
            {
                Name = "Fun Effects",
                Type = "MultiChoice",
                Choices = {
                    floatSettingChoice("Drunk", "Drunk", 1, 0),
                    floatSettingChoice("Confusion", "Confusion", 1, 0),
                    floatSettingChoice("Tiny", "Tiny", 1, 0),
                    floatSettingChoice("Flip", "Flip", 1, 0),
                    floatSettingChoice("Invert", "Invert", 1, 0),
                    floatSettingChoice("Tornado", "Tornado", 1, 0),
                    floatSettingChoice("Tipsy", "Tipsy", 1, 0),
                    floatSettingChoice("Bumpy", "Bumpy", 1, 0),
                    floatSettingChoice("Beat", "Beat", 1, 0),
                    -- X-Mode is dead because it relies on player 2!! -- floatSettingChoice("X-Mode"),
                    floatSettingChoice("Twirl", "Twirl", 1, 0),
                    floatSettingChoice("Roll", "Roll", 1, 0),
                },
            },
            {
                Name = "Acceleration",
                Type = "MultiChoice",
                Choices = {
                    floatSettingChoice("Boost", "Boost", 1, 0),
                    floatSettingChoice("Brake", "Brake", 1, 0),
                    floatSettingChoice("Wave", "Wave", 1, 0),
                    floatSettingChoice("Expand", "Expand", 1, 0),
                    floatSettingChoice("Boomerang", "Boomerang", 1, 0),
                },
            }
        },
        --
        -----
        -- INVALIDATING OPTIONS
        ["Invalidating Options"] = {
            {
                Name = "Mines",
                Type = "SingleChoice",
                Choices = {
                    {
                        Name = "On",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("NoMines", false)
                            setPlayerOptionsModValueAllLevels("Mines", false)
                        end,
                    },
                    {
                        Name = "Off",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("NoMines", true)
                            setPlayerOptionsModValueAllLevels("Mines", false)
                        end,
                    },
                    {
                        Name = "Additive",
                        ChosenFunction = function()
                            setPlayerOptionsModValueAllLevels("NoMines", false)
                            setPlayerOptionsModValueAllLevels("Mines", true)
                        end,
                    }
                },
                ChoiceIndexGetter = function()
                    local po = getPlayerOptions()
                    if po:Mines() then
                        -- additive mines, invalidating
                        return 3
                    elseif po:NoMines() then
                        -- nomines, invalidating
                        return 2
                    else
                        -- regular mines, not invalidating
                        return 1
                    end
                end,
            },
            {
                Name = "Turn",
                Type = "MultiChoice",
                Choices = {
                    booleanSettingChoice("Backwards", "Backwards"),
                    booleanSettingChoice("Left", "Left"),
                    booleanSettingChoice("Right", "Right"),
                    booleanSettingChoice("Shuffle", "Shuffle"),
                    booleanSettingChoice("Soft Shuffle", "SoftShuffle"),
                    booleanSettingChoice("Super Shuffle", "SuperShuffle"),
                }
            },
            {
                Name = "Pattern Transform",
                Type = "MultiChoice",
                Choices = {
                    booleanSettingChoice("Echo", "Echo"),
                    booleanSettingChoice("Stomp", "Stomp"),
                    booleanSettingChoice("Jack JS", "JackJS"),
                    booleanSettingChoice("Anchor JS", "AnchorJS"),
                    booleanSettingChoice("IcyWorld", "IcyWorld"),
                },
            },
            {
                Name = "Hold Transform",
                Type = "MultiChoice",
                Choices = {
                    booleanSettingChoice("Planted", "Planted"),
                    booleanSettingChoice("Floored", "Floored"),
                    booleanSettingChoice("Twister", "Twister"),
                    booleanSettingChoice("Holds To Rolls", "HoldRolls"),
                },
            },
            {
                Name = "Remove",
                Type = "MultiChoice",
                Choices = {
                    booleanSettingChoice("No Holds", "NoHolds"),
                    booleanSettingChoice("No Rolls", "NoRolls"),
                    booleanSettingChoice("No Jumps", "NoJumps"),
                    booleanSettingChoice("No Hands", "NoHands"),
                    booleanSettingChoice("No Lifts", "NoLifts"),
                    booleanSettingChoice("No Fakes", "NoFakes"),
                    booleanSettingChoice("No Quads", "NoQuads"),
                    booleanSettingChoice("No Stretch", "NoStretch"),
                    booleanSettingChoice("Little", "Little"),
                },
            },
            {
                Name = "Insert",
                Type = "MultiChoice",
                Choices = {
                    booleanSettingChoice("Wide", "Wide"),
                    booleanSettingChoice("Big", "Big"),
                    booleanSettingChoice("Quick", "Quick"),
                    booleanSettingChoice("BMR-ize", "BMRize"),
                    booleanSettingChoice("Skippy", "Skippy"),
                },
            }
        },
        --
        -----
        -- GLOBAL GRAPHICS OPTIONS
        ["Global Options"] = {
            {
                Name = "Language",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {}
                    for i, l in ipairs(optionData.language.list) do
                        o[#o+1] = {
                            Name = l:upper(),
                            ChosenFunction = function()
                                optionData.language.current = l
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    for i, l in ipairs(optionData.language.list) do
                        if l == optionData.language.current then return i end
                    end
                    return 1
                end,
            },
            {
                Name = "Display Mode",
                Type = "SingleChoice",
                -- the idea behind Display Mode is to also allow selecting a Display to show the game
                -- it is written into the lua side of the c++ options conf but unused everywhere as far as i know except maybe in linux
                -- so here lets just hardcode windowed/fullscreen until that feature becomes a certain reality
                -- and lets add borderless here so that the options are simplified just a bit
                Choices = {
                    {
                        Name = "Windowed",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("Windowed", true)
                            PREFSMAN:SetPreference("FullscreenIsBorderlessWindow", false)
                        end,
                    },
                    {
                        Name = "Fullscreen",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("Windowed", false)
                            PREFSMAN:SetPreference("FullscreenIsBorderlessWindow", false)
                        end,
                    },
                    {
                        -- funny thing about this preference is that it doesnt force fullscreen
                        -- so you have to pick the right resolution for it to work
                        Name = "Borderless",
                        ChosenFunction = function()
                            PREFSMAN:SetPreference("Windowed", false)
                            PREFSMAN:SetPreference("FullscreenIsBorderlessWindow", true)
                        end,
                    }

                },
                ChoiceIndexGetter = function()
                    if PREFSMAN:GetPreference("FullscreenIsBorderlessWindow") then
                        return 3
                    elseif PREFSMAN:GetPreference("Windowed") then
                        return 1
                    else
                        -- fullscreen exclusive
                        return 2
                    end
                end,
            },
            {
                Name = "Aspect Ratio",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {}
                    for _, ratio in ipairs(optionData.display.ratios) do
                        -- ratio is a fraction, d is denominator and n is numerator
                        local v = ratio.n / ratio.d
                        o[#o+1] = {
                            Name = ratio.n .. ":" .. ratio.d,
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("DisplayAspectRatio", v)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local closestdiff = 100
                    local closestindex = 1
                    local curRatio = PREFSMAN:GetPreference("DisplayAspectRatio")
                    for i, ratio in ipairs(optionData.display.ratios) do
                        -- ratio is a fraction, d is denominator and n is numerator
                        local v = ratio.n / ratio.d
                        local diff = math.abs(v - curRatio)
                        if diff < closestdiff then
                            closestdiff = diff
                            closestindex = i
                        end
                    end
                    return closestindex
                end,
            },
            {
                Name = "Display Resolution",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {}

                    -- i trust we didnt generate any duplicates but ....
                    -- ....... hope not
                    for _, resolution in ipairs(optionData.display.resolutions) do
                        -- resolution is a rectangle and contains a width w and a height h
                        o[#o+1] = {
                            Name = resolution.w .. "x" .. resolution.h,
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("DisplayWidth", resolution.w)
                                PREFSMAN:SetPreference("DisplayHeight", resolution.h)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local closestindex = 1
                    local mindist = -1
                    local w = PREFSMAN:GetPreference("DisplayWidth")
            		local h = PREFSMAN:GetPreference("DisplayHeight")
                    for i, resolution in ipairs(optionData.display.resolutions) do
                        -- resolution is a rectangle and contains a width w and a height h
                        local dist = math.sqrt((resolution.w - w)^2 + (resolution.h - h)^2)
                        if mindist == -1 or dist < mindist then
                            mindist = dist
                            closestindex = i
                        end
                    end
                    return closestindex
                end,
            },
            {
                Name = "Refresh Rate",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {
                        {
                            Name = "Default",
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("RefreshRate", REFRESH_DEFAULT)
                            end,
                        }
                    }
                    for _, rate in ipairs(optionData.display.refreshRates) do
                        o[#o+1] = {
                            Name = tostring(rate),
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("RefreshRate", rate)
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local rate = PREFSMAN:GetPreference("RefreshRate")
                    -- first choice element is this
                    if rate == REFRESH_DEFAULT then return 1 end

                    -- add 1 to index if found
                    for i, r in ipairs(optionData.display.refreshRates) do
                        if rate == r then return i+1 end
                    end

                    -- default
                    return 1
                end,
            },
            {
                Name = "Display Color Depth",
                Type = "SingleChoice",
                Choices = {
                    basicNamedPreferenceChoice("DisplayColorDepth", "16bit", 16),
                    basicNamedPreferenceChoice("DisplayColorDepth", "32bit", 32),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("DisplayColorDepth")
                    if v == 16 then return 1
                    elseif v == 32 then return 2
                    end
                    return 1
                end,
            },
            {
                Name = "Force High Resolution Textures",
                Type = "SingleChoice",
                Choices = choiceSkeleton("Yes", "No"),
                Directions = preferenceToggleDirections("HighResolutionTextures", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("HighResolutionTextures", true),
            },
            {
                Name = "Texture Resolution",
                Type = "SingleChoice",
                Choices = {
                    basicNamedPreferenceChoice("MaxTextureResolution", "256", 256),
                    basicNamedPreferenceChoice("MaxTextureResolution", "512", 512),
                    basicNamedPreferenceChoice("MaxTextureResolution", "1024", 1024),
                    basicNamedPreferenceChoice("MaxTextureResolution", "2048", 2048),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("MaxTextureResolution")
                    if v == 256 then return 1
                    elseif v == 512 then return 2
                    elseif v == 1024 then return 3
                    elseif v == 2048 then return 4
                    end
                    return 1
                end,
            },
            {
                Name = "Texture Color Depth",
                Type = "SingleChoice",
                Choices = {
                    basicNamedPreferenceChoice("TextureColorDepth", "16bit", 16),
                    basicNamedPreferenceChoice("TextureColorDepth", "32bit", 32),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("TextureColorDepth")
                    if v == 16 then return 1
                    elseif v == 32 then return 2
                    end
                    return 1
                end,
            },
            {
                Name = "Movie Color Depth",
                Type = "SingleChoice",
                Choices = {
                    basicNamedPreferenceChoice("MovieColorDepth", "16bit", 16),
                    basicNamedPreferenceChoice("MovieColorDepth", "32bit", 32),
                },
                ChoiceIndexGetter = function()
                    local v = PREFSMAN:GetPreference("MovieColorDepth")
                    if v == 16 then return 1
                    elseif v == 32 then return 2
                    end
                    return 1
                end,
            },
            {
                Name = "VSync",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("Vsync", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("Vsync", true),
            },
            {
                Name = "Instant Search",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirections("instantSearch", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetter("instantSearch", true),
            },
            {
                Name = "Fast Note Rendering",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("FastNoteRendering", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("FastNoteRendering", true),
            },
            {
                Name = "Show Stats",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("ShowStats", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("ShowStats", true),
            },
        },
        --
        -----
        -- THEME OPTIONS
        ["Theme Options"] = {
            {
                Name = "Theme",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {}
                    for _, name in ipairs(THEME:GetSelectableThemeNames()) do
                        o[#o+1] = {
                            Name = name,
                            ChosenFunction = function()
                                optionData.pickedTheme = name
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local cur = optionData.pickedTheme
                    for i, name in ipairs(THEME:GetSelectableThemeNames()) do
                        if name == cur then return i end
                    end
                    return 1
                end,
            },
            {
                Name = "Music Wheel Position",
                Type = "SingleChoice",
                Choices = choiceSkeleton("Left", "Right"),
                Directions = optionDataToggleDirections("wheelPosition", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetter("wheelPosition", true),
            },
            {
                Name = "Show Backgrounds",
                Type = "SingleChoice",
                Choices = choiceSkeleton("Yes", "No"),
                Directions = optionDataToggleDirections("showBackgrounds", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetter("showBackgrounds", true),
            },
            {
                Name = "Easter Eggs & Toasties",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("EasterEggs", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("EasterEggs", true),
            },
            {
                Name = "Music Visualizer",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = optionDataToggleDirections("showVisualizer", true, false),
                ChoiceIndexGetter = optionDataToggleIndexGetter("showVisualizer", true),
            },
            {
                Name = "Mid Grades",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("UseMidGrades", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("UseMidGrades", true),
            },
            {
                Name = "SSRNorm Sort",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("SortBySSRNormPercent", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("SortBySSRNormPercent", true),
            },
            {
                Name = "Show Lyrics",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("ShowLyrics", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("ShowLyrics", true),
            },
            {
                Name = "Transliteration",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = {
                    Toggle = function()
                        if PREFSMAN:GetPreference("ShowNativeLanguage") then
                            PREFSMAN:SetPreference("ShowNativeLanguage", false)
                        else
                            PREFSMAN:SetPreference("ShowNativeLanguage", true)
                        end
                        MESSAGEMAN:Broadcast("DisplayLanguageChanged")
                    end,
                },
                ChoiceIndexGetter = preferenceToggleIndexGetter("ShowNativeLanguage", true),
            },
            {
                Name = "Tip Type",
                Type = "SingleChoice",
                Choices = choiceSkeleton("Tips", "Quotes"),
                Directions = optionDataToggleDirections("tipType", 1, 2),
                ChoiceIndexGetter = optionDataToggleIndexGetter("tipType", 1),
            },
            {
                Name = "Set BG Fit Mode",
                Type = "SingleChoice",
                ChoiceGenerator = function()
                    local o = {}
                    for _, fit in ipairs(BackgroundFitMode) do
                        o[#o+1] = {
                            Name = THEME:GetString("ScreenSetBGFit", ToEnumShortString(fit)),
                            ChosenFunction = function()
                                PREFSMAN:SetPreference("BackgroundFitMode", ToEnumShortString(fit))
                            end,
                        }
                    end
                    return o
                end,
                ChoiceIndexGetter = function()
                    local cur = PREFSMAN:GetPreference("BackgroundFitMode")
                    for i, fit in ipairs(BackgroundFitMode) do
                        if ToEnumShortString(fit) == cur then
                            return i
                        end
                    end
                    return 1
                end,
            },
            {
                Name = "Color Config",
                Type = "Button",
                Choices = {
                    {
                        Name = "Color Config",
                        ChosenFunction = function()
                            -- activate color config screen
                        end,
                    },
                }
            },
        },
        --
        -----
        -- SOUND OPTIONS
        ["Sound Options"] = {
            {
                Name = "Volume",
                Type = "SingleChoice",
                Directions = preferenceIncrementDecrementDirections("SoundVolume", 0, 1, 0.01),
                ChoiceIndexGetter = function()
                    return notShit.round(PREFSMAN:GetPreference("SoundVolume") * 100, 0)
                end,
            },
            {
                Name = "Menu Sounds",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("MuteActions", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("MuteActions", true),
            },
            {
                Name = "Mine Sounds",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("EnableMineHitSound", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("EnableMineHitSound", true),
            },
            {
                Name = "Pitch on Rates",
                Type = "SingleChoice",
                Choices = choiceSkeleton("On", "Off"),
                Directions = preferenceToggleDirections("EnablePitchRates", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("EnablePitchRates", true),
            },
            {
                Name = "Calibrate Audio Sync",
                Type = "Button",
                Choices = {
                    {
                        Name = "Calibrate Audio Sync",
                        ChosenFunction = function()
                            -- go to machine sync screen
                        end,
                    },
                },
            },
        },
        --
        -----
        -- INPUT OPTIONS
        ["Input Options"] = {
            {
                Name = "Back Delayed",
                Type = "SingleChoice",
                Choices = choiceSkeleton("Hold", "Instant"),
                Directions = preferenceToggleDirections("DelayedBack", true, false),
                ChoiceIndexGetter = preferenceToggleIndexGetter("DelayedBack", true),
            },
            {
                Name = "Input Debounce Time",
                Type = "SingleChoice",
                Directions = preferenceIncrementDecrementDirections("InputDebounceTime", 0, 1, 0.01),
                ChoiceIndexGetter = function()
                    return PREFSMAN:GetPreference("InputDebounceTime")
                end,
            },
            {
                Name = "Test Input",
                Type = "Button",
                Choices = {
                    {
                        Name = "Test Input",
                        ChosenFunction = function()
                            -- go to test input screen
                        end,
                    }
                }
            },
        },
        --
        -----
        -- PROFILE OPTIONS
        ["Profile Options"] = {
            {
                Name = "Create Profile",
                Type = "Button",
                Choices = {
                    {
                        Name = "Create Profile",
                        ChosenFunction = function()
                            -- make a profile
                        end,
                    }
                }
            },
            {
                Name = "Rename Profile",
                Type = "Button",
                Choices = {
                    {
                        Name = "Rename Profile",
                        ChosenFunction = function()
                            -- rename a profile
                        end,
                    }
                }
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

    -- container function/frame for all option rows
    local function createOptionRows()
        -- Unfortunate design choice:
        -- For every option row, we are going to place every single possible row type.
        -- This means there's a ton of invisible elements.
        -- Is this worth doing? This is better than telling the C++ to let us generate and destroy arbitrary Actors at runtime
        -- (I consider this dangerous and also too complex to implement)
        -- So instead we "carefully" manage all pieces of an option row...
        -- Luckily we can be intelligent about wasting space.
        -- First, we parse all of the optionData to see which choices need what elements.
        -- We pass that information on to the rows (we can precalculate which rows have what choices)
        -- This way we can avoid generating Actor elements which will never be used in a row

        -- Alternative to doing the above and below:
        -- just use ActorFrame.RemoveChild and ActorFrame.AddChildFromPath

        -- table of row index keys to lists of row types
        -- valid row types are in the giant option definition comment block
        local rowTypes = {}
        -- table of row index keys to counts of how many text objects to generate
        -- this should correlate to how many choices are possible in a row on any option page
        local rowChoiceCount = {}
        for _, optionPage in ipairs(pageNames) do
            for i, categoryName in ipairs(optionPageCategoryLists[optionPage]) do
                local categoryDefinition = optionDefs[categoryName]
                
                -- declare certain rows are categories
                -- (current row and the remaining rows after the set of options in this category)
                if rowTypes[i] ~= nil then
                    rowTypes[i]["Category"] = true
                else
                    rowTypes[i] = {Category = true}
                end
                for ii = (i+1), (#optionPageCategoryLists[optionPage]) do
                    local categoryRowIndex = ii + #categoryDefinition
                    if rowTypes[categoryRowIndex] ~= nil then
                        rowTypes[categoryRowIndex]["Category"] = true
                    else
                        rowTypes[categoryRowIndex] = {Category = true}
                    end
                end

                for j, optionDef in ipairs(categoryDefinition) do
                    local rowIndex = j + i -- skip the rows for option category names
                    
                    -- option types for every row
                    if rowTypes[rowIndex] ~= nil then
                        rowTypes[rowIndex][optionDef.Type] = true
                    else
                        rowTypes[rowIndex] = {[optionDef.Type] = true}
                    end

                    -- option choice count for every row
                    local rcc = rowChoiceCount[rowIndex]
                    if rcc == nil then
                        rowChoiceCount[rowIndex] = 0
                        rcc = 0
                    end
                    local defcount = #(optionDef.Choices or {})
                    -- the only case we should show multiple choices is for MultiChoice...
                    if optionDef.Type ~= "MultiChoice" then defcount = 1 end
                    if rcc < defcount then
                        rowChoiceCount[rowIndex] = defcount
                    end
                end
            end
        end

        ----- state variables, dont mess
        -- currently selected options page - from pageNames
        local selectedPageName = pageNames[1]
        local selectedPageDef = optionPageCategoryLists[selectedPageName]
        -- currently opened option category - from optionPageCategoryLists
        local openedCategoryName = selectedPageDef[1]
        local openedCategoryDef = optionDefs[openedCategoryName]
        -- index of the opened option category to know the index of the first valid option row to assign option defs
        local openedCategoryIndex = 1

        -- putting these functions here to save on space below, less copy pasting, etc
        local function onHover(self)
            if self:IsInvisible() then return end
            self:diffusealpha(buttonHoverAlpha)
        end
        local function onUnHover(self)
            if self:IsInvisible() then return end
            self:diffusealpha(1)
        end
        local function onHoverParent(self)
            if self:GetParent():IsInvisible() then return end
            self:GetParent():diffusealpha(buttonHoverAlpha)
        end
        local function onUnHoverParent(self)
            if self:GetParent():IsInvisible() then return end
            self:GetParent():diffusealpha(1)
        end
        --

        local t = Def.ActorFrame {
            Name = "OptionRowContainer",
            InitCommand = function(self)
                self:y(actuals.TopLipHeight * 2 + actuals.OptionTextListTopGap)
                self:playcommand("OpenPage", {page = 1})
            end,
            OptionTabSetMessageCommand = function(self, params)
                self:playcommand("OpenPage", params)
            end,
            OpenPageCommand = function(self, params)
                local pageIndexToOpen = params.page
                selectedPageName = pageNames[pageIndexToOpen]
                selectedPageDef = optionPageCategoryLists[selectedPageName]
                self:playcommand("OpenCategory", {categoryName = selectedPageDef[1]})
            end,
            OpenCategoryCommand = function(self, params)
                local categoryNameToOpen = params.categoryName
                openedCategoryName = categoryNameToOpen
                openedCategoryDef = optionDefs[openedCategoryName]
                self:playcommand("UpdateRows")
            end,
            UpdateRowsCommand = function(self)
                openedCategoryIndex = 1
                for i = 1, #selectedPageDef do
                    if selectedPageDef[i] == openedCategoryName then
                        openedCategoryIndex = i
                    end
                end

                -- update all rows, redraw
                for i = 1, optionRowCount do
                    local row = self:GetChild("OptionRow_"..i)
                    row:playcommand("UpdateRow")
                end
            end,
        }
        local function createOptionRow(i)
            local types = rowTypes[i] or {}
            -- SingleChoice             1 arrow, 1 choice
            -- SingleChoiceModifier     2 arrow, 1 choice
            -- MultiChoice              2 arrow, N choices
            -- Button                   no arrow, 1 choice
            -- generate elements based on how many choices and how many directional arrows are needed
            local arrowCount = (types["SingleChoiceModifier"] or types["MultiChoice"]) and 2 or (types["SingleChoice"] and 1 or 0)
            local choiceCount = rowChoiceCount[i] or 0

            local optionDef = nil
            local categoryDef = nil
            local previousDef = nil -- for tracking def changes to animate things in a slightly more intelligent way
            local previousPage = 1 -- for tracking page changes to animate things in a slightly more intelligent way
            local rowHandle = nil -- for accessing the row frame from other points of reference (within this function) instantly

            -- convenience
            local function redrawChoiceRelatedElements()
                local rightpair = rowHandle:GetChild("RightTrianglePairFrame")
                local right = rowHandle:GetChild("RightBigTriangleFrame")
                local choices = rowHandle:GetChild("ChoiceFrame")
                if choices ~= nil then
                    choices:finishtweening()
                    choices:diffusealpha(0)
                    choices:smooth(optionRowAnimationSeconds)
                    choices:diffusealpha(1)
                    choices:playcommand("DrawElement")
                end
                if right ~= nil then
                    right:playcommand("DrawElement")
                end
                if rightpair ~= nil then
                    rightpair:playcommand("DrawElement")
                end
            end

            -- index of the choice for this option, if no choices then this is useless
            -- this can also be a table of indices for MultiChoice
            -- this can also just be a random number or text for some certain implementations of optionDefs as long as they conform
            local currentChoiceSelection = 1
            -- move SingleChoice selection index (assuming a list of choices is present -- if not, another methodology is used)
            local function moveChoiceSelection(n)
                if optionDef == nil then return end

                -- make selection loop both directions
                local nn = currentChoiceSelection + n
                if nn <= 0 then
                    nn = n > 0 and 1 or #optionDef.Choices
                elseif nn > #optionDef.Choices then
                    nn = 1
                end
                currentChoiceSelection = nn
                if rowHandle ~= nil then
                    redrawChoiceRelatedElements()
                end
            end
            
            -- paginate choices according to maxChoicesVisibleMultiChoice
            local choicePage = 1
            local maxChoicePage = 1
            local function moveChoicePage(n)
                if maxChoicePage <= 1 then
                    return
                end
    
                -- math to make pages loop both directions
                local nn = (choicePage + n) % (maxChoicePage + 1)
                if nn == 0 then
                    nn = n > 0 and 1 or maxChoicePage
                end
                choicePage = nn
                if rowHandle ~= nil then
                    redrawChoiceRelatedElements()
                end
            end

            -- getter for all relevant children of the row
            -- expects that self is OptionRow_i
            local function getRowElements(self)
                -- directional arrows
                local leftpair = self:GetChild("LeftTrianglePairFrame")
                local left = self:GetChild("LeftBigTriangleFrame")
                local rightpair = self:GetChild("RightTrianglePairFrame")
                local right = self:GetChild("RightBigTriangleFrame")
                -- choices
                local choices = self:GetChild("ChoiceFrame")
                local title = self:GetChild("TitleText")
                local categorytriangle = self:GetChild("CategoryTriangle")
                return leftpair, left, rightpair, right, choices, title, categorytriangle
            end

            local t = Def.ActorFrame {
                Name = "OptionRow_"..i,
                InitCommand = function(self)
                    self:x(actuals.EdgePadding)
                    self:y((actuals.OptionAllottedHeight / #rowChoiceCount) * (i-1) + (actuals.OptionAllottedHeight / #rowChoiceCount / 2))
                    rowHandle = self
                end,
                UpdateRowCommand = function(self)
                    -- update row information, draw (this will reset the state of the row according to "global" conditions)
                    local firstOptionRowIndex = openedCategoryIndex + 1
                    local lastOptionRowIndex = firstOptionRowIndex + #openedCategoryDef - 1

                    -- track previous definition
                    previousDef = nil
                    if optionDef ~= nil then previousDef = optionDef end
                    if categoryDef ~= nil then previousDef = categoryDef end
                    previousPage = choicePage

                    -- reset state
                    optionDef = nil
                    categoryDef = nil
                    choicePage = 1
                    maxChoicePage = 1

                    if i >= firstOptionRowIndex and i <= lastOptionRowIndex then
                        -- this is an option and has an optionDef
                        local optionDefIndex = i - firstOptionRowIndex + 1
                        optionDef = openedCategoryDef[optionDefIndex]
                        if optionDef.Choices ~= nil then
                            maxChoicePage = math.ceil(#optionDef.Choices / maxChoicesVisibleMultiChoice)
                        end
                    else
                        -- this is a category or nothing at all
                        -- maybe generate a "categoryDef" which is really just a summary of what to display instead
                        local lastValidPossibleIndex = lastOptionRowIndex + (#selectedPageDef - openedCategoryIndex)
                        if i > lastValidPossibleIndex then
                            -- nothing.
                        else
                            -- this has a categoryDef
                            local adjustedCategoryIndex = i
                            -- subtract the huge list of optionDefs to grab the position of the category in the original list
                            if i > lastOptionRowIndex then
                                adjustedCategoryIndex = (i) - #openedCategoryDef
                            end
                            categoryDef = {
                                Opened = (openedCategoryIndex == i) and true or false,
                                Name = selectedPageDef[adjustedCategoryIndex]
                            }
                        end
                    end

                    self:playcommand("DrawRow")
                end,
                DrawRowCommand = function(self)
                    -- redraw row
                    local leftPairArrows, leftArrow, rightPairArrows, rightArrow, choiceFrame, titleText, categoryTriangle = getRowElements(self)

                    if optionDef ~= nil and optionDef.ChoiceIndexGetter ~= nil then
                        currentChoiceSelection = optionDef.ChoiceIndexGetter()
                    end

                    -- blink the row if it updated
                    self:finishtweening()
                    self:diffusealpha(0)
                    -- if def was just defined, or def just changed, or choice page just changed -- show animation
                    if previousDef == nil or (optionDef ~= nil and optionDef.Name ~= previousDef.Name) or (categoryDef ~= nil and categoryDef.Name ~= previousDef.Name) or previousPage ~= choicePage then
                        self:smooth(optionRowAnimationSeconds)
                    end
                    self:diffusealpha(1)

                    -- this is done so that the redraw can be done in a particular order, left to right
                    -- also, not all of these actors are guaranteed to exist
                    -- and each actor may or may not rely on the previous one to be positioned in order to correctly draw
                    -- the strict ordering is required as a result
                    if categoryTriangle ~= nil then
                        categoryTriangle:playcommand("DrawElement")
                    end

                    if titleText ~= nil then
                        titleText:playcommand("DrawElement")
                    end

                    if leftPairArrows ~= nil then
                        leftPairArrows:playcommand("DrawElement")
                    end

                    if leftArrow ~= nil then
                        leftArrow:playcommand("DrawElement")
                    end

                    if choiceFrame ~= nil then
                        choiceFrame:playcommand("DrawElement")
                    end

                    if rightArrow ~= nil then
                        rightArrow:playcommand("DrawElement")
                    end

                    if rightPairArrows ~= nil then
                        rightPairArrows:playcommand("DrawElement")
                    end
                end,

                -- category title and option name
                UIElements.TextButton(1, 1, "Common Normal") .. {
                    Name = "TitleText",
                    InitCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")
                        txt:halign(0)
                        txt:zoom(optionTitleTextSize)
                        txt:settext(" ")

                        bg:halign(0)
                        -- fudge movement due to font misalign
                        bg:y(1)
                        bg:zoomto(0, txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                    end,
                    DrawElementCommand = function(self)
                        local txt = self:GetChild("Text")
                        local bg = self:GetChild("BG")

                        if optionDef ~= nil then
                            self:x(0)
                            txt:settext(optionDef.Name)
                            txt:maxwidth(actuals.OptionTextWidth / optionTitleTextSize - textZoomFudge)
                        elseif categoryDef ~= nil then
                            local newx = actuals.OptionBigTriangleWidth + actuals.OptionTextBuffer / 2
                            self:x(newx)
                            txt:settext(categoryDef.Name)
                            txt:maxwidth((actuals.OptionTextWidth - newx) / optionTitleTextSize - textZoomFudge)
                        else
                            txt:settext("")
                        end
                        bg:zoomx(txt:GetZoomedWidth())
                    end,
                    InvokeCommand = function(self)
                        -- behavior for interacting with the Option Row Title Text
                        if categoryDef ~= nil then
                            rowHandle:GetParent():playcommand("OpenCategory", {categoryName = categoryDef.Name})
                        elseif optionDef ~= nil then
                            if optionDef.Type == "Button" then
                                -- button
                            else
                                -- ?
                            end
                        end
                    end,
                    RolloverUpdateCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "in" then
                            self:diffusealpha(buttonHoverAlpha)
                        else
                            self:diffusealpha(1)
                        end
                    end,
                    ClickCommand = function(self, params)
                        if self:IsInvisible() then return end
                        if params.update == "OnMouseDown" then
                            if optionDef ~= nil or categoryDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end
                    end,
                },
            }

            -- category arrow
            if types["Category"] then
                t[#t+1] = UIElements.SpriteButton(1, 1, THEME:GetPathG("", "_triangle")) .. {
                    Name = "CategoryTriangle",
                    InitCommand = function(self)
                        self:x(actuals.OptionBigTriangleWidth/2)
                        self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                    end,
                    DrawElementCommand = function(self)
                        if categoryDef ~= nil then
                            if categoryDef.Opened then
                                self:rotationz(180)
                            else
                                self:rotationz(90)
                            end
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,
                    InvokeCommand = function(self)
                        -- behavior for interacting with the Option Row Title Text
                        if categoryDef ~= nil and not categoryDef.Opened then
                            rowHandle:GetParent():playcommand("OpenCategory", {categoryName = categoryDef.Name})
                        end
                    end,
                    MouseOverCommand = onHover,
                    MouseOutCommand = onUnHover,
                    MouseDownCommand = function(self, params)
                        if self:IsInvisible() then return end
                        self:playcommand("Invoke")
                    end,
                }
            end

            -- smaller double arrow, left/right
            if arrowCount == 2 then
                -- copy paste territory
                t[#t+1] = Def.ActorFrame {
                    Name = "LeftTrianglePairFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and optionDef.Type == "SingleChoiceModifier" then
                            -- only visible in this case
                            -- offset by half the triangle size due to center aligning
                            self:x(actuals.OptionTextWidth + actuals.OptionTextBuffer + actuals.OptionSmallTriangleHeight/2)
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "LeftTriangle", -- outermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(-90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                        end,
                    },
                    Def.Sprite {
                        Name = "RightTriangle", -- innermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(-90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                            -- subtract by 25% triangle height because image is 25% invisible
                            self:x(actuals.OptionSmallTriangleHeight + actuals.OptionSmallTriangleGap - actuals.OptionSmallTriangleHeight/4)
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "LeftTrianglePairButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:x(actuals.OptionSmallTriangleHeight/2)
                            self:zoomto(actuals.OptionSmallTriangleHeight * 2 + actuals.OptionSmallTriangleGap, actuals.OptionBigTriangleWidth)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoiceModifier selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoice (multiplier)
                                        optionDef.Directions.Toggle(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Left ~= nil then
                                        -- Move Left (multiplier)
                                        optionDef.Directions.Left(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(-2)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
                t[#t+1] = Def.ActorFrame {
                    Name = "RightTrianglePairFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and optionDef.Type == "SingleChoiceModifier" then
                            -- only visible in this case
                            local optionRowChoiceFrame = rowHandle:GetChild("ChoiceFrame")
                            if choiceCount < 1 then self:diffusealpha(0):z(-1) return end
                            -- offset by the position of the choice text and the size of the big triangles
                            -- the logic/ordering of the positioning is visible in the math
                            -- choice xpos + width + buffer + big triangle size + buffer
                            -- we pick choice 1 because only SingleChoice is allowed to show these arrows
                            -- offset by half triangle size due to center aligning (edit: nvm?)
                            -- okay actually im gonna be honest I DONT KNOW WHAT IS HAPPENING HERE
                            -- but it completely mirrors the behavior of the other side so it works
                            -- help
                            self:x(optionRowChoiceFrame:GetX() + optionRowChoiceFrame:GetChild("Choice_1"):GetChild("Text"):GetZoomedWidth() + actuals.OptionChoiceDirectionGap + actuals.OptionBigTriangleHeight*0.9 + actuals.OptionChoiceDirectionGap)
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "RightTriangle", -- outermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                            -- subtract by 25% triangle height because image is 25% invisible
                            self:x(actuals.OptionSmallTriangleHeight + actuals.OptionSmallTriangleGap - actuals.OptionSmallTriangleHeight/4)
                        end,
                    },
                    Def.Sprite {
                        Name = "LeftTriangle", -- innermost triangle
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(90)
                            self:zoomto(actuals.OptionSmallTriangleWidth, actuals.OptionSmallTriangleHeight)
                            self:x(0)
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "RightTrianglePairButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:x(actuals.OptionSmallTriangleHeight/2)
                            self:zoomto(actuals.OptionSmallTriangleHeight * 2 + actuals.OptionSmallTriangleGap, actuals.OptionBigTriangleWidth)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoiceModifier selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoice (multiplier)
                                        optionDef.Directions.Toggle(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Right ~= nil then
                                        -- Move Right (multiplier)
                                        optionDef.Directions.Right(true)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(2)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
            end

            -- single large arrow, left/right
            if arrowCount >= 1 then
                t[#t+1] = Def.ActorFrame {
                    Name = "LeftBigTriangleFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and (optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" or (optionDef.Type == "MultiChoice" and maxChoicePage > 1)) then
                            -- visible for SingleChoice(Modifier) and MultiChoice
                            -- only visible on MultiChoice if we need to paginate the choices
                            -- offset by half height due to center aligning
                            local minXPos = actuals.OptionTextWidth + actuals.OptionTextBuffer + actuals.OptionBigTriangleHeight/2
                            if optionDef.Type == "SingleChoice" or optionDef.Type == "MultiChoice" then
                                -- SingleChoice/MultiChoice is on the very left
                                self:x(minXPos)
                            else
                                -- SingleChoiceModifier is to the right of the LeftTrianglePairFrame
                                -- subtract by 25% triangle height twice because 25% of the image is invisible
                                self:x(minXPos + actuals.OptionSmallTriangleHeight * 2 - actuals.OptionSmallTriangleHeight/2 + actuals.OptionSmallTriangleGap + actuals.OptionChoiceDirectionGap)
                            end
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "Triangle",
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(-90)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "TriangleButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "MultiChoice" then
                                    -- MultiChoice pagination
                                    moveChoicePage(-1)
                                elseif optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoice selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoices
                                        optionDef.Directions.Toggle()
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Left ~= nil then
                                        -- Move Left (no multiplier)
                                        optionDef.Directions.Left(false)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(-1)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
                t[#t+1] = Def.ActorFrame {
                    Name = "RightBigTriangleFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil and (optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" or (optionDef.Type == "MultiChoice" and maxChoicePage > 1)) then
                            -- visible for SingleChoice(Modifier) and MultiChoice
                            local optionRowChoiceFrame = rowHandle:GetChild("ChoiceFrame")
                            if choiceCount < 1 then self:diffusealpha(0):z(-1) return end
                            -- offset by the position of the choice text and appropriate buffer
                            -- the logic/ordering of the positioning is visible in the math
                            -- choice xpos + width + buffer
                            -- we pick choice 1 because only SingleChoice is allowed to show these arrows
                            -- subtract by 25% triangle height because 25% of the image is invisible
                            -- offset by half height due to center aligning
                            if optionDef.Type == "MultiChoice" then
                                -- offset to the right of the last visible choice (up to the 4th one)
                                local lastChoiceIndex = math.min(maxChoicesVisibleMultiChoice, #optionDef.Choices) -- last choice if not on first or last page
                                if choicePage > 1 and choicePage >= maxChoicePage then
                                    -- last if on last (first) page
                                    lastChoiceIndex = #optionDef.Choices % maxChoicesVisibleMultiChoice
                                    if lastChoiceIndex == 0 then lastChoiceIndex = maxChoicesVisibleMultiChoice end
                                end
                                local lastChoice = optionRowChoiceFrame:GetChild("Choice_"..lastChoiceIndex)
                                local finalX = optionRowChoiceFrame:GetX() + lastChoice:GetX() + lastChoice:GetChild("Text"):GetZoomedWidth() + actuals.OptionChoiceDirectionGap + actuals.OptionBigTriangleHeight/4
                                self:x(finalX)
                            else
                                self:x(optionRowChoiceFrame:GetX() + optionRowChoiceFrame:GetChild("Choice_1"):GetChild("Text"):GetZoomedWidth() + actuals.OptionChoiceDirectionGap + actuals.OptionBigTriangleHeight/4)
                            end
                            self:diffusealpha(1)
                            self:z(1)
                        else
                            -- always invisible
                            self:diffusealpha(0)
                            self:z(-1)
                        end
                    end,

                    Def.Sprite {
                        Name = "Triangle",
                        Texture = THEME:GetPathG("", "_triangle"),
                        InitCommand = function(self)
                            self:rotationz(90)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                        end,
                    },
                    UIElements.QuadButton(1, 1) .. {
                        Name = "TriangleButton",
                        InitCommand = function(self)
                            self:diffusealpha(0)
                            self:zoomto(actuals.OptionBigTriangleWidth, actuals.OptionBigTriangleHeight)
                        end,
                        InvokeCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "MultiChoice" then
                                    -- MultiChoice pagination
                                    moveChoicePage(1)
                                elseif optionDef.Type == "SingleChoice" or optionDef.Type == "SingleChoiceModifier" then
                                    -- SingleChoice selection mover
                                    if optionDef.Directions ~= nil and optionDef.Directions.Toggle ~= nil then
                                        -- Toggle SingleChoices
                                        optionDef.Directions.Toggle()
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    elseif optionDef.Directions ~= nil and optionDef.Directions.Right ~= nil then
                                        -- Move Right (no multiplier)
                                        optionDef.Directions.Right(false)
                                        if optionDef.ChoiceIndexGetter ~= nil then
                                            currentChoiceSelection = optionDef.ChoiceIndexGetter()
                                        end
                                        redrawChoiceRelatedElements()
                                        return
                                    end

                                    if optionDef.Choices ~= nil then
                                        moveChoiceSelection(1)
                                    else
                                        ms.ok("ERROR REPORT TO DEVELOPER")
                                    end
                                end
                            end
                        end,
                        MouseDownCommand = function(self, params)
                            if self:GetParent():IsInvisible() then return end
                            if optionDef ~= nil then
                                self:playcommand("Invoke")
                            end
                        end,
                        MouseOverCommand = onHoverParent,
                        MouseOutCommand = onUnHoverParent,
                    }
                }
            end

            -- choice text
            local function createOptionRowChoices()
                local t = Def.ActorFrame {
                    Name = "ChoiceFrame",
                    DrawElementCommand = function(self)
                        if optionDef ~= nil then
                            self:diffusealpha(1)

                            local minXPos = actuals.OptionTextWidth + actuals.OptionTextBuffer
                            local finalXPos = minXPos
                            if optionDef.Type == "SingleChoice" or (optionDef.Type == "MultiChoice" and maxChoicePage > 1) then
                                -- leftmost xpos + big triangle + gap
                                -- subtract by 25% of the big triangle size because the image is actually 25% invisible
                                finalXPos = finalXPos + actuals.OptionBigTriangleHeight + actuals.OptionChoiceDirectionGap - actuals.OptionBigTriangleHeight/4
                            elseif optionDef.Type == "SingleChoiceModifier" then
                                -- leftmost xpos + big triangle + gap + 2 small triangles + gap between 2 small triangles + last gap
                                -- subtract by 25% of big triangle and 25% of small triangle twice because the image is 25% invisible
                                finalXPos = finalXPos + actuals.OptionBigTriangleHeight + actuals.OptionChoiceDirectionGap - actuals.OptionBigTriangleHeight/4 + actuals.OptionSmallTriangleHeight * 2 - actuals.OptionSmallTriangleHeight/2 + actuals.OptionSmallTriangleGap + actuals.OptionChoiceDirectionGap
                            end
                            self:x(finalXPos)
                            
                            -- to force the choices to update left to right
                            for i = 1, math.min(choiceCount, maxChoicesVisibleMultiChoice) do
                                self:GetChild("Choice_"..i):playcommand("DrawChoice")
                            end
                        else
                            -- missing optionDef means no choices possible
                            self:diffusealpha(0)
                        end
                    end,
                }
                for n = 1, math.min(choiceCount, maxChoicesVisibleMultiChoice) do
                    t[#t+1] = UIElements.TextButton(1, 1, "Common Normal") .. {
                        Name = "Choice_"..n,
                        InitCommand = function(self)
                            local txt = self:GetChild("Text")
                            local bg = self:GetChild("BG")
                            txt:halign(0)
                            txt:zoom(optionChoiceTextSize)
                            txt:settext(" ")
    
                            bg:halign(0)
                            -- fudge movement due to font misalign
                            bg:y(1)
                            bg:zoomto(0, txt:GetZoomedHeight() * textButtonHeightFudgeScalarMultiplier)
                        end,
                        DrawChoiceCommand = function(self)
                            if optionDef ~= nil then
                                if optionDef.Type == "MultiChoice" then
                                    -- for Multi choice mode
                                    local choice = optionDef.Choices[n + (choicePage-1) * maxChoicesVisibleMultiChoice]
                                    if choice ~= nil then
                                        local txt = self:GetChild("Text")
                                        local bg = self:GetChild("BG")

                                        -- get the x position of this element using the position of the element to the left
                                        -- this requires all elements be updated in order, left to right
                                        local xPos = 0
                                        if n > 1 then
                                            local choiceJustToTheLeftOfThisOne = self:GetParent():GetChild("Choice_"..(n-1))
                                            xPos = choiceJustToTheLeftOfThisOne:GetX() + choiceJustToTheLeftOfThisOne:GetChild("Text"):GetZoomedWidth() + actuals.OptionTextBuffer
                                        end
                                        self:x(xPos)
                                        txt:settext(choice.Name)
                                        bg:zoomx(txt:GetZoomedWidth())

                                        self:diffusealpha(1)
                                        self:z(1)
                                    else
                                        -- choice does not exist for this option but does for another
                                        self:diffusealpha(0)
                                        self:z(-1)
                                    end
                                elseif optionDef.Type == "Button" then
                                    -- Button is just one choice but lets use the option title as the choice (hide all choices)
                                    self:diffusealpha(0)
                                    self:z(-1)
                                else
                                    -- for Single choice mode only show first choice
                                    if n == 1 then
                                        local txt = self:GetChild("Text")
                                        local bg = self:GetChild("BG")

                                        -- several cases involving the ChoiceIndexGetter for single choices...
                                        if optionDef.ChoiceIndexGetter ~= nil and optionDef.Choices == nil then
                                            -- getter with no choices means the getter supplies the visible information
                                            txt:settext(currentChoiceSelection)    
                                        elseif optionDef.Choices ~= nil then
                                            -- choices present means the getter supplies the choice index that contains the information
                                            txt:settext(optionDef.Choices[currentChoiceSelection].Name)
                                        else
                                            txt:settext("INVALID CONTACT DEVELOPER")
                                        end
                                        bg:zoomx(txt:GetZoomedWidth())
                                        self:diffusealpha(1)
                                        self:z(1)
                                    else
                                        self:diffusealpha(0)
                                        self:z(-1)
                                    end
                                end
                            end
                        end,
                        InvokeCommand = function(self)
                            
                        end,
                        RolloverUpdateCommand = function(self, params)
                            if self:IsInvisible() then return end
                            if params.update == "in" then
                                self:diffusealpha(buttonHoverAlpha)
                            else
                                self:diffusealpha(1)
                            end
                        end,
                    }
                end
                return t
            end
            t[#t+1] = createOptionRowChoices()

            return t
        end
        for i = 1, optionRowCount do
            t[#t+1] = createOptionRow(i)
        end
        return t
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
                        MESSAGEMAN:Broadcast("OptionTabSet", {page = i})
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

    t[#t+1] = createOptionRows()
    t[#t+1] = createOptionPageChoices()

    return t
end

t[#t+1] = leftFrame()
t[#t+1] = rightFrame()

return t