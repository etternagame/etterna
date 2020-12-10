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

local visibleframeX = SCREEN_WIDTH - actuals.Width
local visibleframeY = SCREEN_HEIGHT - actuals.Height
local animationSeconds = 0.1
local focused = false

local t = Def.ActorFrame {
    Name = "SearchFile",
    InitCommand = function(self)
        -- lets just say uh ... despite the fact that this file might want to be portable ...
        -- lets ... just .... assume it always goes in the same place ... and the playerInfoFrame is the same size always
        self:x(SCREEN_WIDTH)
        self:y(visibleframeY)
    end,
    GeneralTabSetMessageCommand = function(self, params)
        -- if we ever get this message we need to hide the frame and just exit.
        focused = false
        self:finishtweening()
        self:smooth(animationSeconds)
        self:x(SCREEN_WIDTH)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self, params)
        if params.tab and params.tab == "Search" then
            self:finishtweening()
            self:sleep(0.01)
            self:queuecommand("FinishFocusing")
            self:smooth(animationSeconds)
            self:x(visibleframeX)
        end
    end,
    FinishFocusingCommand = function(self)
        -- the purpose of this is to delay the act of focusing the screen
        -- the reason is that we dont want to trigger Ctrl+1 inputting a 1 on the search field immediately
        focused = true
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Search")
    end
}


local textSize = 1
local textZoomFudge = 5
local textinputbuffer = 5 -- gap between "Search:" and input text
local buttonHoverAlpha = 0.6

local function upperSection()

    -- the base text for each line
    local entryTextTable = {
        "Any Seach", -- this is spelled wrong on purpose
        "Title Search",
        "Subtitle Search",
        "Artist Search",
        "Author Search",
    }

    -- used to actually search for things in WheelDataManager
    local searchentry = {
        Title = "",
        Subtitle = "",
        Artist = "",
        Author = "",
    }

    -- search on the wheel immediately based on the text entered
    local function searchNow()
        -- Main1 is the name of the Main SelectMusic context group
        -- we exit search context after executing a search and set the general box back up
        CONTEXTMAN:SetFocusedContextSet(SCREENMAN:GetTopScreen():GetName(), "Main1")
        local scr = SCREENMAN:GetTopScreen()
        local w = scr:GetChild("WheelFile")
        if w ~= nil then
            WHEELDATA:SetSearch(searchentry)
            w:sleep(0.01):queuecommand("UpdateFilters")
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
            -- must parse the input for title, subtitle, artist, author
            -- same formatting as the old search
            local artistpos = input:find("artist=")
            local authorpos = input:find("author=")
            local mapperpos = input:find("mapper=")
            local charterpos = input:find("charter=")
            local stepperpos = input:find("stepper=")
            local titlepos = input:find("title=")
            local subtitlepos = input:find("subtitle=")

            -- because title is a substring of subtitle we have to check to see if the match is incorrect
            if titlepos ~= nil and subtitlepos ~= nil and titlepos == subtitlepos + 3 then
                titlepos = input:find("title=", titlepos + 1)
            end

            local foundartist = ""
            local foundauthor = ""
            local foundtitle = ""
            local foundsubtitle = ""

            if artistpos ~= nil or authorpos ~= nil or titlepos ~= nil or subtitlepos ~= nil or mapperpos ~= nil or charterpos ~= nil or stepperpos ~= nil then
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
                searchentry.Title = foundtitle
                searchentry.Subtitle = foundsubtitle
                searchentry.Artist = foundartist
                searchentry.Author = foundauthor
            else
                searchentry.Title = input
                searchentry.Subtitle = ""
                searchentry.Artist = ""
                searchentry.Author = ""
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
                    self:settext("")
                    self:maxwidth((actuals.Width - actuals.EdgePadding * 2 - rowtext:GetZoomedWidth() - textinputbuffer) / textSize - textZoomFudge)
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
                        self:strokecolor(color("0.65,0.65,0.65,0.85"))
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
                        local focusedChild = self:GetChild("RowFrame_"..focusedField)

                        if btn == "DeviceButton_enter" or event.button == "Start" then
                            focusedChild:playcommand("Invoke")
                        elseif event.type == "InputEventType_FirstPress" and (btn == "DeviceButton_tab" and not shift) or btn == "DeviceButton_down" then
                            changeFocus(1)
                        elseif event.type == "InputEventType_FirstPress" and (btn == "DeviceButton_tab" and shift) or btn == "DeviceButton_up" then
                            changeFocus(-1)
                        elseif btn == "DeviceButton_escape" then
                            -- shortcut to escape out of search without searching
                            -- (alternatively ... just press a number)
                            MESSAGEMAN:Broadcast("GeneralTabSet")
                        else
                            local del = btn == "DeviceButton_delete"
                            local bs = btn == "DeviceButton_backspace"
                            local char = inputToCharacter(event)

                            -- require that ctrl is pressed for number entry
                            if char ~= nil and tonumber(char) and not INPUTFILTER:IsControlPressed() then
                                return
                            end

                            focusedChild:playcommand("Input", {delete = del, backspace = bs, char = char})

                            -- im just gonna.. update all the fields... for your information....
                            -- this is ... the ... worst possible way .... but also the best....
                            if focusedField == 1 then
                                self:GetChild("RowFrame_2"):GetChild("RowInput"):settext(searchentry.Title)
                                self:GetChild("RowFrame_3"):GetChild("RowInput"):settext(searchentry.Subtitle)
                                self:GetChild("RowFrame_4"):GetChild("RowInput"):settext(searchentry.Artist)
                                self:GetChild("RowFrame_5"):GetChild("RowInput"):settext(searchentry.Author)
                            else
                                -- backwards engineering the any search field
                                -- for the kids who have big brains and want bigger brains
                                local finalstr = ""
                                if searchentry.Title ~= "" or searchentry.Subtitle ~= "" or searchentry.Artist ~= "" or searchentry.Author ~= "" then
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
                                end
                                self:GetChild("RowFrame_1"):GetChild("RowInput"):settext(finalstr)
                            end
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
        "Overall",
        "Stream",
        "Jumpstream",
        "Handstream",
        "Stamina",
        "JackSpeed",
        "Chordjacks",
        "Technical",
        "Length",
    }

    -- defines the bounds for each filter line
    -- if a bound is at either limit, it is considered infinite in that direction
    -- so a Length filter of 1400,3600 is really >1400
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
    -- i know this looks bad but this is just in case we decide to make more filters in the future
    -- and possibly separate the behavior for upper/lower bounds in some case
    local filterCategoryFunction = {
        -- Overall range
        function(lb, ub)
            setSSFilter(1, lb, ub)
        end,
        -- Stream range
        function(lb, ub)
            setSSFilter(2, lb, ub)
        end,
        -- Jumpstream range
        function(lb, ub)
            setSSFilter(3, lb, ub)
        end,
        -- Handstream range
        function(lb, ub)
            setSSFilter(4, lb, ub)
        end,
        -- Stamina range
        function(lb, ub)
            setSSFilter(5, lb, ub)
        end,
        -- Jackspeed range
        function(lb, ub)
            setSSFilter(6, lb, ub)
        end,
        -- Chordjacks range
        function(lb, ub)
            setSSFilter(7, lb, ub)
        end,
        -- Tech range
        function(lb, ub)
            setSSFilter(8, lb, ub)
        end,
        -- Length range
        function(lb, ub)
            -- funny enough we put the length filter in the mysterious 9th skillset spot
            setSSFilter(9, lb, ub)
        end
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
        end
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
        local width = actuals.RightColumnLeftGap - actuals.EdgePadding - actuals.SliderColumnLeftGap

        -- internal vars
        local grabbedDot = nil -- either 0 or 1 for left/right, nil for none

        local function gatherToolTipString()
            local lb, ub = theGetter()
            -- specifically for upper bounds, display 9999 instead to imply infinite
            if ub == 0 then
                ub = 9999
            end
            return string.format("%s\n%d - %d", theName, lb, ub)
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
                end,
            },
            Def.ActorFrame {
                Name = "SliderFrame",
                InitCommand = function(self)
                    self:x(xp)
                end,

                UIElements.SpriteButton(1, 1, THEME:GetPathG("", "roundedCapsBar")) .. {
                    Name = "SliderBG",
                    InitCommand = function(self)
                        self:valign(0)
                        self:rotationz(-90)
                        self:diffuse(color("0,0,0"))
                        self:diffusealpha(0.6)
                        self:zoomto(actuals.SliderThickness, width)
                    end,
                    MouseOverCommand = function(self)
                        if self:IsInvisible() then return end
                        if grabbedSlider == nil then
                            TOOLTIP:SetText(gatherToolTipString())
                            TOOLTIP:Show()
                        end
                    end,
                    MouseOutCommand = function(self)
                        if self:IsInvisible() then return end
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
                            -- convert upper 0 to 100% (infinite)
                            if hi == 0 then hi = ub end
                            local percentX = localX / width
                            local leftDotPercent = lo / ub
                            local rightDotPercent = hi / ub

                            -- set the grabbed dot to the closest dot
                            if math.abs(percentX - rightDotPercent) < math.abs(percentX - leftDotPercent) then
                                -- closer to the right dot
                                grabbedDot = 1
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
                            local localX = clamp(params.MouseX - xp, 0, width)
                            local localY = clamp(params.MouseY, 0, actuals.SliderThickness)

                            local lo, hi = theGetter()
                            local lb, ub = theLimits[1], theLimits[2]
                            -- convert upper 0 to 100% (infinite)
                            if hi == 0 then hi = ub end
                            local percentX = localX / width
                            local leftDotPercent = lo / ub
                            local rightDotPercent = hi / ub

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

                            local fLower = ub * leftDotPercent
                            local fUpper = ub * rightDotPercent
                            -- an upper limit of 100% is meant to be 0 so it can be interpreted as infinite
                            if fUpper >= ub then fUpper = 0 end
                            fLower = clamp(fLower, 0, ub)
                            fUpper = clamp(fUpper, 0, ub)

                            theSetter(fLower, fUpper)
                            TOOLTIP:SetText(gatherToolTipString())
                            self:GetParent():GetChild("LowerBound"):x(width * leftDotPercent)
                            self:GetParent():GetChild("UpperBound"):x(width * rightDotPercent)
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
                Def.Quad {
                    Name = "LowerBound",
                    InitCommand = function(self)
                        -- we use the hypotenuse of a triangle to find the size of the dot but then make it smaller
                        local hypotenuse = math.sqrt(2 * (actuals.SliderThickness ^ 2)) / 2
                        self:rotationz(45)
                        self:zoomto(hypotenuse, hypotenuse)
                        local lb, ub = theGetter()
                        local percentX = lb / theLimits[2]
                        self:x(percentX * width)
                    end,
                },
                Def.Quad {
                    Name = "UpperBound",
                    InitCommand = function(self)
                        -- we use the hypotenuse of a triangle to find the size of the dot but then make it smaller
                        local hypotenuse = math.sqrt(2 * (actuals.SliderThickness ^ 2)) / 2
                        self:rotationz(45)
                        self:zoomto(hypotenuse, hypotenuse)
                        local lb, ub = theGetter()
                        if ub == 0 then ub = theLimits[2] end
                        local percentX = ub / theLimits[2]
                        self:x(percentX * width)
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
        InitCommand = function(self)
            self:settext("Max Rate: ")
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
    }

    t[#t+1] = filterMiscLine(2) .. {
        InitCommand = function(self)
            self:settext("Min Rate: ")
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
    }

    t[#t+1] = filterMiscLine(3) .. {
        InitCommand = function(self)
            self:settext("Mode: ")
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
    }

    t[#t+1] = filterMiscLine(4) .. {
        InitCommand = function(self)
            self:settext("Highest Only: ")
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
    }

    t[#t+1] = filterMiscLine(5) .. {
        InitCommand = function(self)
            self:settext("Matches: ")
        end
    }

    t[#t+1] = filterMiscLine(6) .. {
        InitCommand = function(self)
            self:settext("Reset")
        end,
        MouseOverCommand = onHover,
        MouseOutCommand = onUnHover,
    }

    return t
end

t[#t+1] = Def.Quad {
    Name = "SearchFilterBGQuad",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.Height)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = Def.Quad {
    Name = "SearchFilterLip",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoomto(actuals.Width, actuals.TopLipHeight)
        self:diffuse(color("#111111"))
        self:diffusealpha(0.6)
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "SearchFilterTitle",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.EdgePadding, actuals.TopLipHeight / 2)
        self:zoom(textSize)
        self:maxwidth(actuals.Width / textSize - textZoomFudge)
        self:settext("Search and Filters")
    end
}

t[#t+1] = Def.Quad {
    Name = "Divider",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:xy(actuals.EdgePadding, actuals.UpperSectionHeight)
        self:zoomto(actuals.Width - actuals.EdgePadding * 2, actuals.DividerThickness)
        self:diffuse(color("1,1,1,1"))
    end
}

t[#t+1] = upperSection()
t[#t+1] = lowerSection()

return t