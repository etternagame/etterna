local ratios = {
    Width = 782 / 1920,
    Height = 971 / 1080,
    TopLipHeight = 44 / 1080,
    EdgePadding = 13 / 1920, -- distance from left and right edges for everything
    DividerThickness = 2 / 1080, -- consistently 2 pixels basically
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

            if artistpos ~= nil or authorpos ~= nil or titlepos ~= nil or subtitlepos ~= nil then
                if artistpos ~= nil then
                    local strend = input:find("[;]", artistpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundartist = input:sub(artistpos + 7, strend)
                end
                if authorpos ~= nil then
                    local strend = input:find("[;]", authorpos+1)
                    if strend == nil then strend = #input else strend = strend-1 end
                    foundauthor = input:sub(authorpos + 7, strend)
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

    local filterCategoryTable = {1,1,1,1,1,1,1,1,1}

    local function filterSlider(i)

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
        end
    }

    t[#t+1] = filterMiscLine(2) .. {
        InitCommand = function(self)
            self:settext("Min Rate: ")
        end
    }

    t[#t+1] = filterMiscLine(3) .. {
        InitCommand = function(self)
            self:settext("Mode: ")
        end
    }

    t[#t+1] = filterMiscLine(4) .. {
        InitCommand = function(self)
            self:settext("Highest Only: ")
        end
    }

    t[#t+1] = filterMiscLine(5) .. {
        InitCommand = function(self)
            self:settext("Matches: ")
        end
    }

    t[#t+1] = filterMiscLine(6) .. {
        InitCommand = function(self)
            self:settext("Reset")
        end
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