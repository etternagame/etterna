local filterpreset = require('filterpreset')

local ratios = {
    Width = 782 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything
    DividerThickness = 2 / 1080, -- consistently 2 pixels basically
    SliderThickness = 18 / 1080,
    SliderColumnLeftGap = 196 / 1920, -- from left edge of text to left edge of sliders
    RightColumnLeftGap = 410 / 1920, -- from left edge of frame to left edge of text
    -- using the section height to give equidistant spacing between items with "less" work
    UpperSectionHeight = 440 / 1080, -- from bottom of upperlip to top of upper divider
    LowerSectionHeight = 485 / 1080, -- from bottom of upper divider to bottom of frame
}

local actuals = {
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    TopLipHeight = ratios.TopLipHeight * SCREEN_HEIGHT,
    EdgePadding = ratios.EdgePadding * SCREEN_WIDTH,
    DividerThickness = ratios.DividerThickness * SCREEN_HEIGHT,
    SliderThickness = ratios.SliderThickness * SCREEN_HEIGHT,
    SliderColumnLeftGap = ratios.SliderColumnLeftGap * SCREEN_WIDTH,
    RightColumnLeftGap = ratios.RightColumnLeftGap * SCREEN_WIDTH,
    UpperSectionHeight = ratios.UpperSectionHeight * SCREEN_HEIGHT,
    LowerSectionHeight = ratios.LowerSectionHeight * SCREEN_HEIGHT,
}

local translations = {
    Title = THEME:GetString("SearchFilter", "Title"),
    OmniSearch = THEME:GetString("SearchFilter", "OmniSearch"),
    TitleSearch = THEME:GetString("SearchFilter", "TitleSearch"),
    SubtitleSearch = THEME:GetString("SearchFilter", "SubtitleSearch"),
    ArtistSearch = THEME:GetString("SearchFilter", "ArtistSearch"),
    AuthorSearch = THEME:GetString("SearchFilter", "AuthorSearch"),
    GroupSearch = THEME:GetString("SearchFilter", "GroupSearch"),
    OverallFilter = THEME:GetString("SearchFilter", "OverallFilter"),
    SteamFilter = THEME:GetString("SearchFilter", "SteamFilter"),
    JumpstreamFilter = THEME:GetString("SearchFilter", "JumpstreamFilter"),
    HandstreamFilter = THEME:GetString("SearchFilter", "HandstreamFilter"),
    StaminaFilter = THEME:GetString("SearchFilter", "StaminaFilter"),
    JackSpeedFilter = THEME:GetString("SearchFilter", "JackSpeedFilter"),
    ChordjacksFilter = THEME:GetString("SearchFilter", "ChordjacksFilter"),
    TechnicalFilter = THEME:GetString("SearchFilter", "TechnicalFilter"),
    LengthFilter = THEME:GetString("SearchFilter", "LengthFilter"),
    ClearPercentFilter = THEME:GetString("SearchFilter", "ClearPercentFilter"),
    UpperBoundRate = THEME:GetString("SearchFilter", "UpperBoundRate"),
    LowerBoundRate = THEME:GetString("SearchFilter", "LowerBoundRate"),
    AnyAllMode = THEME:GetString("SearchFilter", "AnyAllMode"),
    Any = THEME:GetString("SearchFilter", "Any"),
    All = THEME:GetString("SearchFilter", "All"),
    HighestSkillsetOnly = THEME:GetString("SearchFilter", "HighestSkillsetOnly"),
    HardestChartOnly = THEME:GetString("SearchFilter", "HardestChartOnly"),
    On = THEME:GetString("SearchFilter", "On"),
    Off = THEME:GetString("SearchFilter", "Off"),
    Results = THEME:GetString("SearchFilter", "Results"),
    Reset = THEME:GetString("SearchFilter", "Reset"),
    Apply = THEME:GetString("SearchFilter", "Apply"),
    SaveToDefaultPreset = THEME:GetString("FilterPreset", "SaveToDefaultPreset"),
    ExportPresetToFile = THEME:GetString("FilterPreset", "ExportPresetToFile"),
    SaveFilterPresetPrompt = THEME:GetString("FilterPreset", "SaveFilterPresetPrompt"),
}

local visibleframeX = SCREEN_WIDTH - actuals.Width
local visibleframeY = SCREEN_HEIGHT - actuals.Height
local hiddenframeX = SCREEN_WIDTH
local animationSeconds = 0.1
local focused = false

local t = Def.ActorFrame {
    Name = "SearchFile",
    InitCommand = function(self)
        self:playcommand("SetPosition")
        self:y(visibleframeY)
        self:diffusealpha(0)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:diffusealpha(0)
        self:x(hiddenframeX)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if params.tab and params.tab == "Search" then
            self:diffusealpha(1)
            self:finishtweening()
            self:sleep(0.01)
            self:queuecommand("FinishFocusing")
            self:smooth(animationSeconds)
            self:x(visibleframeX)
        else
            self:finishtweening()
            self:smooth(animationSeconds)
            self:diffusealpha(0)
            self:x(hiddenframeX)
            focused = false
        end
    end,
    FinishFocusingCommand = function(self)
        -- the purpose of this is to delay the act of focusing the screen
        -- the reason is that we dont want to trigger Ctrl+1 inputting a 1 on the search field immediately
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Search")
    end,
    SetPositionCommand = function(self)
        if getWheelPosition() then
            visibleframeX = SCREEN_WIDTH - actuals.Width
            hiddenframeX = SCREEN_WIDTH
        else
            visibleframeX = 0
            hiddenframeX = -actuals.Width
        end
        if focused then
            self:x(visibleframeX)
        else
            self:x(hiddenframeX)
        end
    end,
    UpdateWheelPositionCommand = function(self)
        self:playcommand("SetPosition")
    end,
}


local textSize = 1
local textZoomFudge = 5
local textinputbuffer = 5 -- gap between "Search:" and input text
local buttonHoverAlpha = 0.6

-- i made a critical mistake in planning and have to put this here for scoping reasons
local searchentry = {}
local function upperSection()

    -- the base text for each line
    local entryTextTable = {
        translations["OmniSearch"],
        translations["TitleSearch"],
        translations["SubtitleSearch"],
        translations["ArtistSearch"],
        translations["AuthorSearch"],
        translations["GroupSearch"],
    }

    -- used to actually search for things in WheelDataManager
    -- get an empty one because we dont want to init with a search entry
    searchentry = getEmptyActiveFilterMetadata()

    -- search on the wheel immediately based on the text entered
    local function searchNow()
        -- Main1 is the name of the Main SelectMusic context group
        -- we exit search context after executing a search and set the general box back up
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Main1")
        local scr = SCREENMAN:GetTopScreen()
        local w = scr:GetChild("WheelFile")
        if w ~= nil then
            WHEELDATA:SetSearch(searchentry)
            w:sleep(0.01):queuecommand("ApplyFilter")
        end
        MESSAGEMAN:Broadcast("GeneralTabSet")
    end

    -- current focused text entry field, 1 is "Any"
    -- based on entryTextTable
    local focusedField = 1

    -- text entry into a field causes the searchentry to be updated accordingly
    -- based on entryTextTable
    local entryFunction = {
        -- "Any Search"
        function(input)
            if input ~= nil then input = input:lower() end
            -- must parse the input for title, subtitle, artist, author, group
            -- same formatting as the old search
            local artistpos = input:find("artist=", 1, true)
            local authorpos = input:find("author=", 1, true)
            local mapperpos = input:find("mapper=", 1, true)
            local charterpos = input:find("charter=", 1, true)
            local stepperpos = input:find("stepper=", 1, true)
            local titlepos = input:find("title=", 1, true)
            local subtitlepos = input:find("subtitle=", 1, true)
            local grouppos = input:find("group=", 1, true)
            local packpos = input:find("pack=", 1, true)

            -- because title is a substring of subtitle we have to check to see if the match is incorrect
            if titlepos ~= nil and subtitlepos ~= nil and titlepos == subtitlepos + 3 then
                titlepos = input:find("title=", titlepos + 1, true)
            end

            local foundartist = ""
            local foundauthor = ""
            local foundtitle = ""
            local foundsubtitle = ""
            local foundgroup = ""

            if artistpos ~= nil or authorpos ~= nil or
                titlepos ~= nil or subtitlepos ~= nil or
                mapperpos ~= nil or charterpos ~= nil or
                stepperpos ~= nil or grouppos ~= nil or
                packpos ~= nil then

                if artistpos ~= nil then
                    local strend = input:find("[;]", artistpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundartist = input:sub(artistpos + 7, strend)
                end
                if authorpos ~= nil then
                    local strend = input:find("[;]", authorpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundauthor = input:sub(authorpos + 7, strend)
                elseif mapperpos ~= nil then
                    local strend = input:find("[;]", mapperpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundauthor = input:sub(mapperpos + 7, strend)
                elseif charterpos ~= nil then
                    local strend = input:find("[;]", charterpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundauthor = input:sub(charterpos + 8, strend)
                elseif stepperpos ~= nil then
                    local strend = input:find("[;]", stepperpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundauthor = input:sub(stepperpos + 8, strend)
                end
                if titlepos ~= nil then
                    local strend = input:find("[;]", titlepos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundtitle = input:sub(titlepos + 6, strend)
                end
                if subtitlepos ~= nil then
                    local strend = input:find("[;]", subtitlepos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundsubtitle = input:sub(subtitlepos + 9, strend)
                end
                if grouppos ~= nil then
                    local strend = input:find("[;]", grouppos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundgroup = input:sub(grouppos + 6, strend)
                elseif packpos ~= nil then
                    local strend = input:find("[;]", packpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundgroup = input:sub(packpos + 5, strend)
                end
                searchentry.Title = foundtitle
                searchentry.Subtitle = foundsubtitle
                searchentry.Artist = foundartist
                searchentry.Author = foundauthor
                searchentry.Group = foundgroup
            else
                searchentry.Title = input
                searchentry.Subtitle = ""
                searchentry.Artist = ""
                searchentry.Author = ""
                searchentry.Group = ""
            end

            -- you know what im just going to update all the other entry fields based on this one

        end,
        -- "Title Search"
        function(input)
            searchentry.Title = input
        end,
        -- "Subtitle Search"
        function(input)
            searchentry.Subtitle = input
        end,
        -- "Artist Search"
        function(input)
            searchentry.Artist = input
        end,
        -- "Author Search"
        function(input)
            searchentry.Author = input
        end,
        -- "Group Search"
        function(input)
            searchentry.Group = input
        end,
    }

    -- move focus of text entry to another line
    local function changeFocus(direction)
        focusedField = focusedField + direction
        if focusedField > #entryTextTable then focusedField = 1 end
        if focusedField < 1 then focusedField = #entryTextTable end
        MESSAGEMAN:Broadcast("UpdateSearchFocus")
    end

    local function textEntryField(i)
        return Def.ActorFrame {
            Name = "RowFrame_"..i,
            InitCommand = function(self)
                local numberForSpacingItsLikeTheChoicesButMore = #entryTextTable+1
                local allowedVerticalSpaceForTheseItems = actuals.UpperSectionHeight
                self:xy(actuals.EdgePadding, (allowedVerticalSpaceForTheseItems / numberForSpacingItsLikeTheChoicesButMore) * (i-1) + (allowedVerticalSpaceForTheseItems / numberForSpacingItsLikeTheChoicesButMore / 2))
            end,

            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "RowText",
                InitCommand = function(self)
                    self:halign(0)
                    self:settextf("%s:", entryTextTable[i])
                    self:maxwidth((actuals.Width - actuals.EdgePadding * 2) / textSize - textZoomFudge)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:GetParent():diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:GetParent():diffusealpha(1)
                end,
                MouseDownCommand = function(self, params)
                    if params.event == "DeviceButton_left mouse button" then
                        focusedField = i
                        MESSAGEMAN:Broadcast("UpdateSearchFocus")
                    end
                end,
                UpdateSearchFocusMessageCommand = function(self)
                    if focusedField == i then
                        self:strokecolor(color("0.6,0.6,0.6,0.75"))
                        self:diffusealpha(1)
                    else
                        self:strokecolor(color("1,1,1,0"))
                        self:diffusealpha(0.8)
                    end
                end
            },
            UIElements.TextToolTip(1, 1, "Common Normal") .. {
                Name = "RowInput",
                BeginCommand = function(self)
                    local rowtext = self:GetParent():GetChild("RowText")
                    self:x(rowtext:GetZoomedWidth() + textinputbuffer)
                    self:halign(0)
                    self:maxwidth((actuals.Width - actuals.EdgePadding * 2 - rowtext:GetZoomedWidth() - textinputbuffer) / textSize - textZoomFudge)
                    self:diffuse(COLORS:getMainColor("SecondaryText"))
                    self:diffusealpha(1)
                end,
                ColorConfigUpdatedMessageCommand = function(self)
                    local ab4 = self:GetDiffuseAlpha()
                    self:diffuse(COLORS:getMainColor("SecondaryText"))
                    self:diffusealpha(ab4)
                    self:playcommand("UpdateSearchFocus")
                end,
                MouseOverCommand = function(self)
                    if self:IsInvisible() then return end
                    self:GetParent():diffusealpha(buttonHoverAlpha)
                end,
                MouseOutCommand = function(self)
                    if self:IsInvisible() then return end
                    self:GetParent():diffusealpha(1)
                end,
                MouseDownCommand = function(self, params)
                    if params.event == "DeviceButton_left mouse button" then
                        focusedField = i
                        MESSAGEMAN:Broadcast("UpdateSearchFocus")
                    end
                end,
                UpdateSearchFocusMessageCommand = function(self)
                    if focusedField == i then
                        self:strokecolor(Brightness(COLORS:getMainColor("SecondaryText"), 0.85))
                        self:diffusealpha(1)
                    else
                        self:strokecolor(color("1,1,1,0"))
                        self:diffusealpha(0.8)
                    end
                end,
                InputCommand = function(self, params)
                    local txt = self:GetText()
                    if params.backspace then
                        txt = txt:sub(1, -2)
                    elseif params.delete then
                        txt = ""
                    elseif params.char then
                        txt = txt .. params.char
                    end
                    self:settext(txt)
                    entryFunction[i](txt)
                end,
                InvokeCommand = function(self)
                    searchNow()
                end
            },
            Def.Quad { -- funny we make this a quad instead of an underscore, right
                Name = "CursorUnderscore",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoomto(10,2)
                    -- disabled for now
                    self:visible(false)
                    registerActorToColorConfigElement(self, "main", "SeparationDivider")
                end,
                UpdateSearchFocusMessageCommand = function(self)
                    if focusedField == i then
                        self:diffusealpha(1)
                    else
                        self:diffusealpha(0)
                    end
                end
            },
        }
    end

    local t = Def.ActorFrame {
        Name = "UpperSectionFrame",
        InitCommand = function(self)
            self:y(actuals.TopLipHeight)
        end,
        BeginCommand = function(self)
            local snm = SCREENMAN:GetTopScreen():GetName()
            local anm = self:GetName()

            local function updateFields()
                -- im just gonna.. update all the fields... for your information....
                -- this is ... the ... worst possible way .... but also the best....
                if focusedField == 1 then
                    self:GetChild("RowFrame_2"):GetChild("RowInput"):settext(searchentry.Title)
                    self:GetChild("RowFrame_3"):GetChild("RowInput"):settext(searchentry.Subtitle)
                    self:GetChild("RowFrame_4"):GetChild("RowInput"):settext(searchentry.Artist)
                    self:GetChild("RowFrame_5"):GetChild("RowInput"):settext(searchentry.Author)
                    self:GetChild("RowFrame_6"):GetChild("RowInput"):settext(searchentry.Group)
                else
                    -- backwards engineering the any search field
                    -- for the kids who have big brains and want bigger brains
                    local finalstr = ""
                    if searchentry.Title ~= "" or searchentry.Subtitle ~= "" or searchentry.Artist ~= "" or searchentry.Author ~= "" or searchentry.Group ~= "" then
                        if searchentry.Title ~= "" then
                            finalstr = finalstr .. "title="..searchentry.Title..";"
                        end
                        if searchentry.Subtitle ~= "" then
                            finalstr = finalstr .. "subtitle="..searchentry.Subtitle..";"
                        end
                        if searchentry.Artist ~= "" then
                            finalstr = finalstr .. "artist="..searchentry.Artist..";"
                        end
                        if searchentry.Author ~= "" then
                            finalstr = finalstr .. "author="..searchentry.Author..";"
                        end
                        if searchentry.Group ~= "" then
                            finalstr = finalstr .. "group="..searchentry.Group..";"
                        end
                    end
                    self:GetChild("RowFrame_1"):GetChild("RowInput"):settext(finalstr)
                end
            end
            -- update all the search fields
            updateFields()
            focusedField = 2
            updateFields()
            focusedField = 1
            -- it works

            -- init the search input context but start it out false
            CONTEXTMAN:RegisterToContextSet(snm, "Search", anm)
            CONTEXTMAN:ToggleContextSet(snm, "Search", false)

            SCREENMAN:GetTopScreen():AddInputCallback(function(event)
                -- if context is set to Search, passthrough unless not holding ctrl and a number
                -- pressing a number alone should lead to the general tab
                if CONTEXTMAN:CheckContextSet(snm, "Search") then
                    if event.type ~= "InputEventType_Release" then
                        local btn = event.DeviceInput.button
                        local shift = INPUTFILTER:IsShiftPressed()
                        local ctrl = INPUTFILTER:IsControlPressed()
                        local focusedChild = self:GetChild("RowFrame_"..focusedField)

                        if btn == "DeviceButton_enter" or event.button == "Start" then
                            focusedChild:playcommand("Invoke")
                        elseif event.type == "InputEventType_FirstPress" and (btn == "DeviceButton_tab" and not shift) or btn == "DeviceButton_down" then
                            changeFocus(1)
                        elseif event.type == "InputEventType_FirstPress" and (btn == "DeviceButton_tab" and shift) or btn == "DeviceButton_up" then
                            changeFocus(-1)
                        elseif btn == "DeviceButton_escape" then
                            -- shortcut to escape out of search without searching
                            MESSAGEMAN:Broadcast("GeneralTabSet")
                        else
                            local del = btn == "DeviceButton_delete"
                            local bs = btn == "DeviceButton_backspace"
                            local copypasta = btn == "DeviceButton_v" and ctrl
                            local char = inputToCharacter(event)

                            -- if ctrl is pressed with a number, let the general tab input handler deal with this
                            if char ~= nil and tonumber(char) and INPUTFILTER:IsControlPressed() then
                                return
                            end

                            -- paste
                            if copypasta then
                                char = Arch.getClipboard()
                            end

                            focusedChild:playcommand("Input", {delete = del, backspace = bs, char = char})

                            updateFields()
                        end
                    end
                end
            end)
            self:playcommand("UpdateSearchFocus")
        end,
        PlayerInfoFrameTabSetMessageCommand = function(self, params)
            if params.tab and params.tab == "Search" then
                if focusedField ~= 1 then
                    focusedField = 1
                    self:playcommand("UpdateSearchFocus")
                end
            end
        end
    }

    for i = 1, #entryTextTable do
        t[#t+1] = textEntryField(i)
    end

    return t
end

local function lowerSection()
    -- putting these functions here to save on space below, less copy pasting, etc
    local function onHover(self)
        if self:IsInvisible() then return end
        self:diffusealpha(buttonHoverAlpha)
    end
    local function onUnHover(self)
        if self:IsInvisible() then return end
        self:diffusealpha(1)
    end

    -- names for each filter line
    local filterCategoryTable = {
        translations["OverallFilter"],
        translations["SteamFilter"],
        translations["JumpstreamFilter"],
        translations["HandstreamFilter"],
        translations["StaminaFilter"],
        translations["JackSpeedFilter"],
        translations["ChordjacksFilter"],
        translations["TechnicalFilter"],
        translations["LengthFilter"],
        translations["ClearPercentFilter"],
    }

    -- defines the bounds for each filter line
    -- if a bound is at either limit, it is considered infinite in that direction
    -- so a Length filter of 400,600 is really >400
    -- or a Length filter of 0,360 is really <360
    -- etc
    local filterCategoryLimits = {
        { 0, 40 },  -- Overall
        { 0, 40 },  -- Stream
        { 0, 40 },  -- Jumpstream
        { 0, 40 },  -- Handstream
        { 0, 40 },  -- Stamina
        { 0, 40 },  -- JackSpeed
        { 0, 40 },  -- Chordjacks
        { 0, 40 },  -- Technical
        { 0, 600 },  -- Length (in seconds)
        { 85, 100 }, -- Percent
    }

    -- convenience to set the upper and lower bound for a skillset
    -- for interacting with the c++ side
    local function setSSFilter(ss, lb, ub)
        FILTERMAN:SetSSFilter(lb, ss, 0)
        FILTERMAN:SetSSFilter(ub, ss, 1)
    end

    -- convenience to get the upper and lower bounds for a skillset
    -- for interacting with the c++ side
    local function getSSFilter(ss)
        return FILTERMAN:GetSSFilter(ss, 0), FILTERMAN:GetSSFilter(ss, 1)
    end

    -- functions for each filter, what they control
    -- each of these filters are range filters, take 2 parameters
    -- the third parameter are the limits as determined by the filterCategoryLimits table
    -- i know this looks bad but this is just in case we decide to make more filters in the future
    -- and possibly separate the behavior for upper/lower bounds in some case
    local filterCategoryFunction = {
        -- Overall range
        function(lb, ub, limits)
            setSSFilter(1, lb, ub)
        end,
        -- Stream range
        function(lb, ub, limits)
            setSSFilter(2, lb, ub)
        end,
        -- Jumpstream range
        function(lb, ub, limits)
            setSSFilter(3, lb, ub)
        end,
        -- Handstream range
        function(lb, ub, limits)
            setSSFilter(4, lb, ub)
        end,
        -- Stamina range
        function(lb, ub, limits)
            setSSFilter(5, lb, ub)
        end,
        -- Jackspeed range
        function(lb, ub, limits)
            setSSFilter(6, lb, ub)
        end,
        -- Chordjacks range
        function(lb, ub, limits)
            setSSFilter(7, lb, ub)
        end,
        -- Tech range
        function(lb, ub, limits)
            setSSFilter(8, lb, ub)
        end,
        -- Length range
        function(lb, ub, limits)
            -- funny enough we put the length filter in the mysterious 9th skillset spot
            setSSFilter(9, lb, ub)
        end,
        -- Clear range
        function(lb, ub, limits)
            -- and we put the clear filter in the mysterious 10th skillset spot
            if lb == limits[1] then
                setSSFilter(10, 0, ub)
            else
                setSSFilter(10, lb, ub)
            end
        end,
    }

    -- functions for each filter, getters for what they control
    -- OH NO I COPY PASTED THE ABOVE TABLE THIS JUST GOT A LOT WORSE
    local filterCategoryGetters = {
        -- Overall range
        function()
            return getSSFilter(1)
        end,
        -- Stream range
        function()
            return getSSFilter(2)
        end,
        -- Jumpstream range
        function()
            return getSSFilter(3)
        end,
        -- Handstream range
        function()
            return getSSFilter(4)
        end,
        -- Stamina range
        function()
            return getSSFilter(5)
        end,
        -- Jackspeed range
        function()
            return getSSFilter(6)
        end,
        -- Chordjacks range
        function()
            return getSSFilter(7)
        end,
        -- Tech range
        function()
            return getSSFilter(8)
        end,
        -- Length range
        function()
            -- funny enough we put the length filter in the mysterious 9th skillset spot
            return getSSFilter(9)
        end,
        -- Clear range
        function()
            -- and we put the clear filter in the mysterious 10th skillset spot
            return getSSFilter(10)
        end,
    }

    local grabbedSlider = nil

    local function filterSlider(i)
        -- convenience on convenience yo
        local theLimits = filterCategoryLimits[i]
        local theSetter = filterCategoryFunction[i]
        local theGetter = filterCategoryGetters[i]
        local theName = filterCategoryTable[i]

        -- repeated use vars here
        local xp = actuals.SliderColumnLeftGap - actuals.EdgePadding
        local width = actuals.RightColumnLeftGap - actuals.EdgePadding * 1.5 - actuals.SliderColumnLeftGap
        local sliderBGSizeBump = 15/1920 * SCREEN_WIDTH

        -- internal vars
        local grabbedDot = nil -- either 0 or 1 for left/right, nil for none

        local function gatherToolTipString()
            local lb, ub = theGetter()
            -- upper bound of 0 is infinite: display an indeterminant upper bound
            if ub == 0 then
                return string.format("%s\n%.2f - %.2f+", theName, lb, theLimits[2])
            else
                return string.format("%s\n%.2f - %.2f", theName, lb, ub)
            end
        end

        -- ended up copy pasting this 3 times and took a look and said no thanks
        -- this is responsible for setting all the stuff related to clicky movy things
        local function draggyEvent(self, params)
            local localX = clamp(params.MouseX - xp, 0, width)
            local localY = clamp(params.MouseY, 0, actuals.SliderThickness)

            local lo, hi = theGetter()
            local lb, ub = theLimits[1], theLimits[2]
            local range = ub - lb
            -- convert upper 0 to 100% (infinite)
            if hi == 0 then hi = ub end
            if lo == lb then lo = 0 end
            local percentX = localX / width
            local leftDotPercent = math.max(0, (lo - lb)) / range
            local rightDotPercent = math.min(ub, (hi - lb)) / range

            -- make sure the dot being dragged is not dragged too close to or beyond the other dot
            if grabbedDot == 0 then
                if percentX > rightDotPercent then
                    percentX = clamp(rightDotPercent - 0.001, 0, 1)
                end
                leftDotPercent = percentX
            elseif grabbedDot == 1 then
                if percentX < leftDotPercent then
                    percentX = clamp(leftDotPercent + 0.001, 0, 1)
                end
                rightDotPercent = percentX
            else
                -- dont know how this could happen, but quit if it does
                return
            end

            local fLower = lb + (range * leftDotPercent)
            local fUpper = lb + (range * rightDotPercent)
            -- an upper limit of 100% is meant to be 0 so it can be interpreted as infinite
            if fUpper >= ub then fUpper = 0 end
            fLower = clamp(fLower, 0, ub)
            fUpper = clamp(fUpper, 0, ub)

            theSetter(fLower, fUpper, theLimits)
            TOOLTIP:SetText(gatherToolTipString())
            self:GetParent():playcommand("UpdateDots")
        end

        return Def.ActorFrame {
            Name = "SliderOwnerFrame_"..i,
            InitCommand = function(self)
                local tblAndOne = #filterCategoryTable + 1
                self:xy(actuals.EdgePadding, (actuals.LowerSectionHeight / tblAndOne) * (i-1) + (actuals.LowerSectionHeight / tblAndOne / 2))
            end,

            LoadFont("Common Normal") .. {
                Name = "SliderTitle",
                InitCommand = function(self)
                    self:halign(0)
                    self:zoom(textSize)
                    self:maxwidth((actuals.SliderColumnLeftGap - actuals.EdgePadding) / textSize - textZoomFudge)
                    self:settext(theName)
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
            },
            Def.ActorFrame {
                Name = "SliderFrame",
                InitCommand = function(self)
                    self:x(xp)
                end,

                Def.Sprite {
                    Name = "SliderBG",
                    Texture = THEME:GetPathG("", "sliderBar"),
                    InitCommand = function(self)
                        self:halign(0)
                        self:diffusealpha(0.6)
                        registerActorToColorConfigElement(self, "generalBox", "SliderBackground")
                        -- we want the bg to actually be just barely larger for reasons
                        -- visually it makes everything look a lot better
                        -- because otherwise the dots end up outside the visible bg at the edges
                        self:zoomto(width + sliderBGSizeBump, actuals.SliderThickness)
                        self:x(-sliderBGSizeBump/2)
                    end
                },
                UIElements.QuadButton(1, 1) .. {
                    Name = "SliderButtonArea",
                    InitCommand = function(self)
                        self:halign(0)
                        self:diffusealpha(0)
                        self:zoomto(width, actuals.SliderThickness)
                    end,
                    MouseOverCommand = function(self)
                        if grabbedSlider == nil then
                            TOOLTIP:SetText(gatherToolTipString())
                            TOOLTIP:Show()
                        end
                    end,
                    MouseOutCommand = function(self)
                        if isOver(self:GetParent():GetChild("LowerBound")) then return end
                        if isOver(self:GetParent():GetChild("UpperBound")) then return end

                        if grabbedSlider == nil then
                            TOOLTIP:Hide()
                        end
                    end,
                    MouseDownCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end

                        if grabbedDot == nil then
                            local localX = clamp(params.MouseX - xp, 0, width)
                            local localY = clamp(params.MouseY, 0, actuals.SliderThickness)

                            local lo, hi = theGetter()
                            local lb, ub = theLimits[1], theLimits[2]
                            local range = ub - lb
                            -- convert upper 0 to 100% (infinite)
                            if hi == 0 then hi = ub end
                            local percentX = localX / width
                            local leftDotPercent = math.max(0, (lo - lb)) / range
                            local rightDotPercent = math.min(ub, (hi - lb)) / range

                            -- set the grabbed dot to the closest dot
                            if math.abs(percentX - rightDotPercent) < math.abs(percentX - leftDotPercent) then
                                -- closer to the right dot
                                grabbedDot = 1
                            elseif math.abs(percentX - rightDotPercent) == math.abs(percentX - leftDotPercent) then
                                -- somehow in the center or the dots are on top of each other
                                -- pick the closest dot by direction
                                if percentX > rightDotPercent then
                                    grabbedDot = 1
                                elseif percentX < leftDotPercent then
                                    grabbedDot = 0
                                end
                            else
                                -- closer to the left dot or in the middle
                                grabbedDot = 0
                            end
                            grabbedSlider = i
                        end
                    end,
                    MouseHoldCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end

                        if grabbedDot ~= nil then
                            draggyEvent(self, params)
                        end
                    end,
                    MouseClickCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end
                        -- for all release events while on this button (having already pressed it)
                        grabbedDot = nil
                        grabbedSlider = nil
                        if not isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end,
                    MouseReleaseCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end
                        -- for all release events while not on this button (having already pressed it)
                        grabbedDot = nil
                        grabbedSlider = nil
                        if not isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end,
                },
                UIElements.SpriteButton(1, 1, THEME:GetPathG("", "Marker")) .. {
                    Name = "LowerBound",
                    InitCommand = function(self)
                        -- we use the hypotenuse of a triangle to find the size of the dot but then make it smaller
                        local hypotenuse = math.sqrt(2 * (actuals.SliderThickness ^ 2)) / 2
                        self:zoomto(hypotenuse, hypotenuse)
                        self:playcommand("UpdateDots")
                        registerActorToColorConfigElement(self, "main", "SeparationDivider")
                    end,
                    UpdateDotsCommand = function(self)
                        local lb, ub = theGetter()
                        local percentX = clamp((lb - theLimits[1]) / (theLimits[2] - theLimits[1]), 0, 1)
                        self:x(percentX * width)
                    end,
                    MouseDownCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end

                        if params.event == "DeviceButton_left mouse button" then
                            grabbedDot = 0
                            grabbedSlider = i
                        end
                    end,
                    MouseHoldCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end

                        if grabbedDot ~= nil then
                            draggyEvent(self, params)
                        end
                    end,
                    MouseOverCommand = function(self)
                        if grabbedSlider == nil then
                            TOOLTIP:SetText(gatherToolTipString())
                            TOOLTIP:Show()
                        end
                    end,
                    MouseOutCommand = function(self)
                        if isOver(self:GetParent():GetChild("SliderButtonArea")) then return end

                        if grabbedSlider == nil then
                            TOOLTIP:Hide()
                        end
                    end,
                    MouseClickCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end
                        -- for all release events while on this button (having already pressed it)
                        grabbedDot = nil
                        grabbedSlider = nil
                        if not isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end,
                    MouseReleaseCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end
                        -- for all release events while not on this button (having already pressed it)
                        grabbedDot = nil
                        grabbedSlider = nil
                        if not isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end,
                },
                UIElements.SpriteButton(1, 1, THEME:GetPathG("", "Marker")) .. {
                    Name = "UpperBound",
                    InitCommand = function(self)
                        -- we use the hypotenuse of a triangle to find the size of the dot but then make it smaller
                        local hypotenuse = math.sqrt(2 * (actuals.SliderThickness ^ 2)) / 2
                        self:zoomto(hypotenuse, hypotenuse)
                        self:playcommand("UpdateDots")
                        registerActorToColorConfigElement(self, "main", "SeparationDivider")
                    end,
                    UpdateDotsCommand = function(self)
                        local lb, ub = theGetter()
                        if ub == 0 then ub = theLimits[2] end
                        local percentX = clamp((ub - theLimits[1]) / (theLimits[2] - theLimits[1]), 0, 1)
                        self:x(percentX * width)
                    end,
                    MouseDownCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end

                        if params.event == "DeviceButton_left mouse button" then
                            grabbedDot = 1
                            grabbedSlider = i
                        end
                    end,
                    MouseHoldCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end

                        if grabbedDot ~= nil then
                            draggyEvent(self, params)
                        end
                    end,
                    MouseOverCommand = function(self)
                        if grabbedSlider == nil then
                            TOOLTIP:SetText(gatherToolTipString())
                            TOOLTIP:Show()
                        end
                    end,
                    MouseOutCommand = function(self)
                        if isOver(self:GetParent():GetChild("SliderButtonArea")) then return end
                        if grabbedSlider == nil then
                            TOOLTIP:Hide()
                        end
                    end,
                    MouseClickCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end
                        -- for all release events while on this button (having already pressed it)
                        grabbedDot = nil
                        grabbedSlider = nil
                        if not isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end,
                    MouseReleaseCommand = function(self, params)
                        if params.event ~= "DeviceButton_left mouse button" then return end
                        -- for all release events while not on this button (having already pressed it)
                        grabbedDot = nil
                        grabbedSlider = nil
                        if not isOver(self) then
                            TOOLTIP:Hide()
                        end
                    end,
                }

            }
        }
    end

    -- use this function to generate a new line for the right column
    -- this column has multiple purposes, so all this function will do is generate the base
    -- the base being: they are all BitmapText on a consistent column with consistent spacing
    local function filterMiscLine(i)
        return UIElements.TextToolTip(1, 1, "Common Normal") .. {
            InitCommand = function(self)
                -- x pos: right column
                -- y pos: a line on the right column based on i (similar math to the Tab system positioning)
                local tblAndOne = #filterCategoryTable + 1
                self:halign(0)
                self:xy(actuals.RightColumnLeftGap, (actuals.LowerSectionHeight / tblAndOne) * (i-1) + (actuals.LowerSectionHeight / tblAndOne / 2))
                self:maxwidth((actuals.Width - actuals.EdgePadding - actuals.RightColumnLeftGap) / textSize - textZoomFudge)
                registerActorToColorConfigElement(self, "main", "PrimaryText")
            end
        }
    end


    local t = Def.ActorFrame {
        Name = "LowerSectionFrame",
        InitCommand = function(self)
            self:y(actuals.TopLipHeight + actuals.UpperSectionHeight + actuals.DividerThickness)
        end,
    }

    for i = 1, #filterCategoryTable do
        t[#t+1] = filterSlider(i)
    end

    -- well ... i tried to reduce the code duplication without going stupid
    t[#t+1] = filterMiscLine(1) .. {
        Name = "MaxRateLine",
        InitCommand = function(self)
            self:playcommand("UpdateText")
        end,
        UpdateTextCommand = function(self)
            local maxrate = FILTERMAN:GetMaxFilterRate()
            self:settextf("%s: %2.1f", translations["UpperBoundRate"], maxrate)
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self, params)
            local maxrate = FILTERMAN:GetMaxFilterRate()
            local increment = 0.1
            if params.event == "DeviceButton_left mouse button" then
                -- it's already set haha
            elseif params.event == "DeviceButton_right mouse button" then
                increment = increment * -1
            else
                return
            end

            maxrate = clamp(clamp(maxrate + increment, FILTERMAN:GetMinFilterRate(), 3), 0.7, 3)
            FILTERMAN:SetMaxFilterRate(maxrate)
            self:playcommand("UpdateText")
        end,
    }

    t[#t+1] = filterMiscLine(2) .. {
        Name = "MinRateLine",
        InitCommand = function(self)
            self:playcommand("UpdateText")
        end,
        UpdateTextCommand = function(self)
            local maxrate = FILTERMAN:GetMinFilterRate()
            self:settextf("%s: %2.1f", translations["LowerBoundRate"], maxrate)
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self, params)
            local minrate = FILTERMAN:GetMinFilterRate()
            local increment = 0.1
            if params.event == "DeviceButton_left mouse button" then
                -- it's already set haha
            elseif params.event == "DeviceButton_right mouse button" then
                increment = increment * -1
            else
                return
            end

            minrate = clamp(clamp(minrate + increment, 0.7, FILTERMAN:GetMaxFilterRate()), 0.7, 3)
            FILTERMAN:SetMinFilterRate(minrate)
            self:playcommand("UpdateText")
        end,
    }

    t[#t+1] = filterMiscLine(3) .. {
        Name = "FilterModeLine",
        InitCommand = function(self)
            self:playcommand("UpdateText")
        end,
        UpdateTextCommand = function(self)
            local txt = FILTERMAN:GetFilterMode() and translations["All"] or translations["Any"]
            self:settextf("%s: %s", translations["AnyAllMode"], txt)
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self)
            FILTERMAN:ToggleFilterMode()
            self:playcommand("UpdateText")
        end
    }

    t[#t+1] = filterMiscLine(4) .. {
        Name = "HighestSkillsetOnlyLine",
        InitCommand = function(self)
            self:playcommand("UpdateText")
        end,
        UpdateTextCommand = function(self)
            local txt = FILTERMAN:GetHighestSkillsetsOnly() and translations["On"] or translations["Off"]
            self:settextf("%s: %s", translations["HighestSkillsetOnly"], txt)
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self)
            FILTERMAN:ToggleHighestSkillsetsOnly()
            self:playcommand("UpdateText")
        end
    }

    t[#t+1] = filterMiscLine(5) .. {
        Name = "HighestDifficultyOnlyLine",
        InitCommand = function(self)
            self:playcommand("UpdateText")
        end,
        UpdateTextCommand = function(self)
            local txt = FILTERMAN:GetHighestDifficultyOnly() and translations["On"] or translations["Off"]
            self:settextf("%s: %s", translations["HardestChartOnly"], txt)
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self)
            FILTERMAN:ToggleHighestDifficultyOnly()
            self:playcommand("UpdateText")
        end
    }

    t[#t+1] = filterMiscLine(6) .. {
        Name = "MatchCountLine",
        UpdateTextCommand = function(self)
            local count1 = WHEELDATA:GetFilteredSongCount()
            local count2 = WHEELDATA:GetSongCount()
            self:settextf("%s: %d/%d", translations["Results"], count1, count2)
        end,
        FinishedSortMessageCommand = function(self)
            self:playcommand("UpdateText")
        end
    }

    t[#t+1] = filterMiscLine(7) .. {
        Name = "ResetLine",
        InitCommand = function(self)
            self:settext(translations["Reset"])
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self)
            FILTERMAN:ResetAllFilters()
            self:GetParent():playcommand("UpdateText")
            self:GetParent():playcommand("UpdateDots")
        end
    }

    t[#t+1] = filterMiscLine(8) .. {
        Name = "ApplyLine",
        InitCommand = function(self)
            self:settext(translations["Apply"])
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self)
            -- really all this does is trigger a search
            -- since the filter is always set to what you visually see, you just have to reload the wheel
            local scr = SCREENMAN:GetTopScreen()
            local w = scr:GetChild("WheelFile")
            if w ~= nil then
                WHEELDATA:SetSearch(searchentry)
                w:sleep(0.01):queuecommand("ApplyFilter")
            end
            -- but we dont change the input context to keep it from being too jarring
        end
    }

    t[#t+1] = filterMiscLine(9) .. {
        Name = "SaveLine",
        InitCommand = function(self)
            self:settext(translations["SaveToDefaultPreset"])
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self)
            filterpreset.save_preset("default", PLAYER_1)
        end
    }


    t[#t+1] = filterMiscLine(10) .. {
        Name = "ExportLine",
        InitCommand = function(self)
            self:settext(translations["ExportPresetToFile"])
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
        MouseDownCommand = function(self)
            local redir = SCREENMAN:get_input_redirected(PLAYER_1)
            local function off()
                if redir then
                    SCREENMAN:set_input_redirected(PLAYER_1, false)
                end
            end
            local function on()
                if redir then
                    SCREENMAN:set_input_redirected(PLAYER_1, true)
                end
            end
            off()

            askForInputStringWithFunction(
                translations["SaveFilterPresetPrompt"],
                32,
                false,
                function(answer)
                    -- success if the answer isnt blank
                    if answer:gsub("^%s*(.-)%s*$", "%1") ~= "" then
                        filterpreset.save_preset(answer)
                    else
                        on()
                    end
                end,
                function() return true, "" end,
                function()
                    on()
                end
            )
        end
    }

    return t
end

t[#t+1] = Def.Quad {
    Name = "SearchFilterBGQuad",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.Height)
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "PrimaryBackground")
    end
}

t[#t+1] = Def.Quad {
    Name = "SearchFilterLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.TopLipHeight)
        self:diffusealpha(0.6)
        registerActorToColorConfigElement(self, "main", "SecondaryBackground")
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "SearchFilterTitle",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
        self:zoom(textSize)
        self:maxwidth(actuals.Width / textSize - textZoomFudge)
        self:settext(translations["Title"])
        registerActorToColorConfigElement(self, "main", "PrimaryText")
    end
}

t[#t+1] = Def.Quad {
    Name = "Divider",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:xy(actuals.EdgePadding, actuals.UpperSectionHeight)
        self:zoomto(actuals.Width - actuals.EdgePadding * 2, actuals.DividerThickness)
        registerActorToColorConfigElement(self, "main", "SeparationDivider")
    end
}

t[#t+1] = upperSection()
t[#t+1] = lowerSection()

-- Load default preset if it exists. We should only be setting the values once
-- at startup. Subsequent calls should not occur.
filterpreset.load_preset("default", false, PLAYER_1)

return t
