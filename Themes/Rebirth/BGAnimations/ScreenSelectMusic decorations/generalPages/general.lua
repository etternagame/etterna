local displayScore
local t = Def.ActorFrame {
    Name = "GeneralPageFile",
    InitCommand = function(self)
        -- hide all general box tabs on startup
        self:diffusealpha(0)
    end,
    WheelSettledMessageCommand = function(self, params)
        -- update displayscore
        -- it sets to nil properly by itself
        displayScore = GetDisplayScore()

        -- could not think of a good place to put this
        -- this turns off mirror for the specific situation if you just finished a permamirrored chart
        -- i have no idea why it works but it does
        -- (shouldnt turning mirror off every time song changes break regular mirror? it doesnt)
        if GetPlayerOrMachineProfile(PLAYER_1):IsCurrentChartPermamirror() then
            local modslevel = topscreen == "ScreenEditOptions" and "ModsLevel_Stage" or "ModsLevel_Preferred"
            local playeroptions = GAMESTATE:GetPlayerState():GetPlayerOptions(modslevel)
            playeroptions:Mirror(false)
        end

        -- cascade visual update to everything
        self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
    end,
    GeneralTabSetMessageCommand = function(self, params)
        if params and params.tab ~= nil then
            if params.tab == SCUFF.generaltabindex then
                self:z(200)
                self:smooth(0.2)
                self:diffusealpha(1)
            else
                self:z(-100)
                self:smooth(0.2)
                self:diffusealpha(0)
            end
        end
    end,
    CurrentRateChangedMessageCommand = function(self)
        -- update displayscore
        -- it sets to nil properly by itself
        displayScore = GetDisplayScore()
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = GAMESTATE:GetCurrentSteps()})
    end,
    ChangedStepsMessageCommand = function(self, params)
        displayScore = GetDisplayScore()
        self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), hovered = lastHovered, steps = params.steps})
    end,
}

local ratios = {
    VerticalDividerLeftGap = 387 / 1920, -- from left edge to left edge of divider
    VerticalDividerUpperGap = 42 / 1080, -- from top edge to top edge
    VerticalDividerHeight = 374 / 1080,
    HorizontalDividerLeftGap = 11 / 1920, -- from left edge to left edge of divider
    HorizontalDividerUpperGap = 431 / 1080, -- from top edge to top edge
    HorizontalDividerLength = 753 / 1920,
    DividerThickness = 2 / 1080, -- consistently 2 pixels basically

    LeftTextColumn1NumbersMargin = 172 / 1920, -- from left edge to right edge of text
    LeftTextColumn1LabelsMargin = 12 / 1920, -- from left edge to left edge of text
    LeftTextColumn2Margin = 207 / 1920, -- from left edge to left edge
    LeftTextUpperGap = 169 / 1080, -- from top edge to top edge
    -- use the column1 and column2 x positions for the tag locations as well
    MSDUpperGap = 43 / 1080, -- top edge to top edge
    WifePercentUpperGap = 90 / 1080, -- top edge to top edge

    RightTextLabelsMargin = 327 / 1920, -- from right edge to left edge of text
    RightTextNumbersMargin = 66 / 1920, -- from right edge to right edge of text

    LeftTextAllottedVerticalSpace = 210 / 1080, -- from top edge of top text to top edge of bottom text
    -- right text also has allotted space but we will extrapolate from this number
    TagTextUpperGap = 453 / 1080, -- from top edge to top edge
    TagTextAllottedVerticalSpace = 42 / 1080, -- from top edge of top text to top edge of bottom text

    CDTitleRightGap = 5 / 1920, -- estimated right side gap for the cdtitle (restrict width)
    CDTitleLeftGap = 190 / 1920, -- left edge to approximate left edge
    -- CDTitle width is VerticalDividerX - CDTitleRightGap - CDTitleLeftGap
    CDTitleUpperGap = 36 / 1080, -- top edge to approximate top edge
    CDTitleAllowedHeight = 100 / 1080, -- approximated allowed height, from top edge to bottom edge
}

local actuals = {
    VerticalDividerLeftGap = ratios.VerticalDividerLeftGap * SCREEN_WIDTH,
    VerticalDividerUpperGap = ratios.VerticalDividerUpperGap * SCREEN_HEIGHT,
    VerticalDividerHeight = ratios.VerticalDividerHeight * SCREEN_HEIGHT,
    HorizontalDividerLeftGap = ratios.HorizontalDividerLeftGap * SCREEN_WIDTH,
    HorizontalDividerUpperGap = ratios.HorizontalDividerUpperGap * SCREEN_HEIGHT,
    HorizontalDividerLength = ratios.HorizontalDividerLength * SCREEN_WIDTH,
    DividerThickness = ratios.DividerThickness * SCREEN_HEIGHT,
    LeftTextColumn1NumbersMargin = ratios.LeftTextColumn1NumbersMargin * SCREEN_WIDTH,
    LeftTextColumn1LabelsMargin = ratios.LeftTextColumn1LabelsMargin * SCREEN_WIDTH,
    LeftTextColumn2Margin = ratios.LeftTextColumn2Margin * SCREEN_WIDTH,
    LeftTextUpperGap = ratios.LeftTextUpperGap * SCREEN_HEIGHT,
    MSDUpperGap = ratios.MSDUpperGap * SCREEN_HEIGHT,
    WifePercentUpperGap = ratios.WifePercentUpperGap * SCREEN_HEIGHT,
    RightTextLabelsMargin = ratios.RightTextLabelsMargin * SCREEN_WIDTH,
    RightTextNumbersMargin = ratios.RightTextNumbersMargin * SCREEN_WIDTH,
    --RightTextNumbersMargin = ratios.LeftTextColumn1LabelsMargin * SCREEN_WIDTH, -- to have equal space as left stuff
    LeftTextAllottedVerticalSpace = ratios.LeftTextAllottedVerticalSpace * SCREEN_HEIGHT,
    TagTextUpperGap = ratios.TagTextUpperGap * SCREEN_HEIGHT,
    TagTextAllottedVerticalSpace = ratios.TagTextAllottedVerticalSpace * SCREEN_HEIGHT,
    CDTitleRightGap = ratios.CDTitleRightGap * SCREEN_WIDTH,
    CDTitleLeftGap = ratios.CDTitleLeftGap * SCREEN_WIDTH,
    CDTitleUpperGap = ratios.CDTitleUpperGap * SCREEN_HEIGHT,
    CDTitleAllowedHeight = ratios.CDTitleAllowedHeight * SCREEN_HEIGHT,
}

-- scoping magic
do
    -- copying the provided ratios and actuals tables to have access to the sizing for the overall frame
    local rt = Var("ratios")
    for k,v in pairs(rt) do
        ratios[k] = v
    end
    local at = Var("actuals")
    for k,v in pairs(at) do
        actuals[k] = v
    end
end

-- translations exclusive to this screen
local translations = {
    AverageNPS = THEME:GetString("ScreenSelectMusic General", "AverageNPS"),
    NegativeBPMs = THEME:GetString("ScreenSelectMusic General", "NegativeBPMs"),
}

local statNames = {
    THEME:GetString("RadarCategory", "Notes"),
    THEME:GetString("RadarCategory", "Jumps"),
    THEME:GetString("RadarCategory", "Hands"),
    THEME:GetString("RadarCategory", "Holds"),
    THEME:GetString("RadarCategory", "Rolls"),
    THEME:GetString("RadarCategory", "Mines"),
}

-- output of the relevant radars function is in a certain order
-- it isnt the order of the above list
-- so this list takes those indices and points them in another direction
local statMapping = {
       -- output -> desired
    1, -- notes - notes
    2, -- jumps - jumps
    3, -- hands - hands
    4, -- holds - holds
    6, -- mines - rolls
    5, -- rolls - mines
    7, -- lifts
    8, -- fakes
}

local msdNames = {
    translations["AverageNPS"],
    ms.SkillSetsTranslatedByName["Stream"],
    ms.SkillSetsTranslatedByName["Jumpstream"],
    ms.SkillSetsTranslatedByName["Handstream"],
    ms.SkillSetsTranslatedByName["Stamina"],
    ms.SkillSetsTranslatedByName["JackSpeed"],
    ms.SkillSetsTranslatedByName["Chordjack"],
    ms.SkillSetsTranslatedByName["Technical"],
}

local mainTextSize = 1
local largerTextSize = 1.35
local displayScoreInfoTextSize = 0.75

local textzoomFudge = 5
-- bump the second line of the display score info down by this much
local displayScoreBump = 8

local function createStatLines()
    local function createStatLine(i)
        return Def.ActorFrame {
            Name = "Stat"..i,
            InitCommand = function(self)
                self:y(actuals.LeftTextUpperGap + (actuals.LeftTextAllottedVerticalSpace / (#statNames-1)) * (i-1))
            end,

            LoadFont("Common Normal") .. {
                Name = "Label",
                InitCommand = function(self)
                    self:x(actuals.LeftTextColumn1LabelsMargin)
                    self:halign(0):valign(0)
                    self:zoom(mainTextSize)
                    -- dont fudge this to avoid compressing static text
                    self:maxwidth(((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / 1.9) / mainTextSize)
                    self:settextf("%s:", statNames[i])
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end
            },
            Def.RollingNumbers {
                Name = "Count",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:x(actuals.LeftTextColumn1NumbersMargin)
                    self:halign(1):valign(0)
                    self:zoom(mainTextSize)
                    self:maxwidth(((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / 2) / mainTextSize - textzoomFudge)
                    self:Load("RollingNumbersNoLead")
                    registerActorToColorConfigElement(self, "main", "SecondaryText")
                end,
                SetCommand = function(self, params)
                    if params.steps then
                        self:targetnumber(params.steps:GetRelevantRadars()[statMapping[i]])
                    else
                        self:targetnumber(0)
                    end
                end
            }
        }
    end
    local t = Def.ActorFrame {Name = "Stats"}
    for i = 1, #statNames do
        t[#t+1] = createStatLine(i)
    end
    return t
end

local function createTopSkillsetLines()
    local function createSkillsetLine(i)
        return Def.ActorFrame {
            Name = "SkillsetLine"..i,
            InitCommand = function(self)
                self:y(actuals.LeftTextUpperGap + (actuals.LeftTextAllottedVerticalSpace / (#statNames-1)) * (i-1))
            end,

            LoadFont("Common Normal") .. {
                Name = "Text",
                InitCommand = function(self)
                    self:x(actuals.LeftTextColumn2Margin)
                    self:halign(0):valign(0)
                    self:zoom(mainTextSize)
                    self:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftTextColumn1LabelsMargin - actuals.LeftTextColumn2Margin) / mainTextSize - textzoomFudge)
                    self:settext("Jumpstream")
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetCommand = function(self, params)
                    if params.steps then
                        local ss = params.steps:GetRelevantSkillsetsByMSDRank(getCurRateValue(), i)
                        self:settext(ss)
                    else
                        self:settext("")
                    end
                end
            }
        }
    end
    local t = Def.ActorFrame {Name = "TopSkillsets"}
    for i = 1, 3 do
        t[#t+1] = createSkillsetLine(i)
    end
    return t
end

local function createMSDLines()
    local function createMSDLine(i)
        local labeltext = msdNames[i]
        return Def.ActorFrame {
            Name = "MSDLine"..i,
            InitCommand = function(self)
                self:y(actuals.LeftTextUpperGap + (actuals.LeftTextAllottedVerticalSpace / (#statNames-1)) * (i-1 - 2))
            end,

            LoadFont("Common Normal") .. {
                Name = "Label",
                InitCommand = function(self)
                    self:x(actuals.Width - actuals.RightTextLabelsMargin)
                    self:halign(0):valign(0)
                    self:zoom(mainTextSize)
                    -- dont fudge this to avoid compressing static text
                    self:maxwidth(((actuals.RightTextLabelsMargin - actuals.RightTextNumbersMargin) / 1.7) / mainTextSize)
                    if labeltext then
                        self:settextf("%s:", labeltext)
                    end
                    registerActorToColorConfigElement(self, "main", "PrimaryText")
                end,
                SetCommand = function(self, params)
                    -- HACKS HACKS HACKS
                    -- (remove when validating negbpms soon)
                    if i == 0 then
                        if params.steps then
                            if params.steps:GetTimingData():HasWarps() then
                                self:settext(translations["NegativeBPMs"])
                                self:diffusealpha(1)
                            else
                                self:diffusealpha(0)
                            end
                        else
                            self:diffusealpha(0)
                        end
                    end
                end,
            },
            Def.RollingNumbers {
                Name = "Number",
                Font = "Common Normal",
                InitCommand = function(self)
                    self:x(actuals.Width - actuals.RightTextNumbersMargin)
                    self:halign(1):valign(0)
                    self:zoom(mainTextSize)
                    self:maxwidth(((actuals.RightTextLabelsMargin - actuals.RightTextNumbersMargin) / 2) / mainTextSize - textzoomFudge)
                    self:Load("RollingNumbers2Decimal")
                    if not labeltext then self:visible(false) end
                end,
                SetCommand = function(self, params)
                    if i == 0 then return end -- negbpm indicator HACKS remove when validating negbpms soon
                    -- i == 1 is Average NPS, otherwise are skillsets
                    if i == 1 then
                        if params.steps then
                            -- notecount / length * rate
                            local len = params.steps:GetLengthSeconds()
                            local notes = params.steps:GetRadarValues(PLAYER_1):GetValue("RadarCategory_Notes")
                            local avg = notes / len
                            if len == 0 then
                                if notes > 0 then
                                    avg = notes
                                else
                                    avg = 0
                                end
                            elseif len < 1 then
                                avg = clamp(avg, 0, notes)
                            end
                            self:targetnumber(avg)
                            self:diffuse(colorByNPS(avg))
                        else
                            -- failsafe
                            self:targetnumber(0)
                            self:diffuse(color("1,1,1,1"))
                        end
                    else
                        if params.song then
                            if params.steps then
                                local val = params.steps:GetMSD(getCurRateValue(), i)
                                self:targetnumber(val)
                                self:diffuse(colorByMSD(val))
                            else
                                -- failsafe
                                self:targetnumber(0)
                                self:diffuse(color("1,1,1,1"))
                            end
                        else
                            self:targetnumber(0)
                            self:diffuse(color("1,1,1,1"))
                        end
                    end
                end
            }
        }
    end
    local t = Def.ActorFrame {Name = "MSDLines"}
    for i = 0, #msdNames do -- starts at 0 for NegBPMs
        t[#t+1] = createMSDLine(i)
    end
    return t
end

-- only accounting for room for 4 tags
local function createTagDisplays()
    local currentTags = {"","","",""}
    local function createTagDisplay(i)
        local xPos = i < 3 and actuals.LeftTextColumn1LabelsMargin or actuals.LeftTextColumn2Margin
        return LoadFont("Common Normal") .. {
            Name = "Tag"..i,
            InitCommand = function(self)
                self:xy(xPos, actuals.TagTextUpperGap + (actuals.TagTextAllottedVerticalSpace * ((i-1) % 2)))
                self:halign(0):valign(0)
                self:zoom(mainTextSize)
                self:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftTextColumn1LabelsMargin - actuals.LeftTextColumn2Margin) / mainTextSize - textzoomFudge)
                registerActorToColorConfigElement(self, "main", "SecondaryText")
            end,
            SetCommand = function(self, params)
                if params.steps then
                    if currentTags[i] then
                        self:settext(currentTags[i])
                    else
                        self:settext("")
                    end
                else
                    self:settext("")
                end
            end
        }
    end
    local t = Def.ActorFrame {
        Name = "TagDisplays",
        SetCommand = function(self, params)
            -- update tag data
            currentTags = {}
            if params.song and params.steps then
                local playerTags = TAGMAN:get_data().playerTags
                local ck = params.steps:GetChartKey()
                for k,v in pairs(playerTags) do
                    if playerTags[k][ck] then
                        currentTags[#currentTags+1] = k
                    end
                end
                table.sort(
                    currentTags,
                    function(a,b) return a:lower() < b:lower() end
                )
            end
        end,
        ReassignedTagsMessageCommand = function(self)
            self:playcommand("Set", {song = GAMESTATE:GetCurrentSong(), steps = GAMESTATE:GetCurrentSteps()})
        end
    }
    for i = 1, #currentTags do
        t[#t+1] = createTagDisplay(i)
    end
    return t
end

t[#t+1] = Def.Quad {
    Name = "HorizontalDivider",
    InitCommand = function(self)
        self:xy(actuals.HorizontalDividerLeftGap, actuals.HorizontalDividerUpperGap)
        self:zoomto(actuals.HorizontalDividerLength, actuals.DividerThickness)
        self:halign(0):valign(0)
        registerActorToColorConfigElement(self, "main", "SeparationDivider")
    end
}

t[#t+1] = Def.Quad {
    Name = "VerticalDivider",
    InitCommand = function(self)
        self:xy(actuals.VerticalDividerLeftGap, actuals.VerticalDividerUpperGap)
        self:zoomto(actuals.DividerThickness, actuals.VerticalDividerHeight)
        self:halign(0):valign(0)
        registerActorToColorConfigElement(self, "main", "SeparationDivider")
    end,
}

t[#t+1] = Def.RollingNumbers {
    Name = "MSD",
    Font = "Common Normal",
    InitCommand = function(self)
        self:xy(actuals.LeftTextColumn1NumbersMargin, actuals.MSDUpperGap)
        self:halign(1):valign(0)
        self:zoom(largerTextSize)
        self:maxwidth((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / largerTextSize - textzoomFudge)
        self:Load("RollingNumbers2Decimal")
    end,
    SetCommand = function(self, params)
        if params.steps then
            local meter = params.steps:GetMSD(getCurRateValue(), 1)
            self:targetnumber(meter)
            self:diffuse(colorByMSD(meter))
        else
            self:targetnumber(0)
            self:diffuse(color("1,1,1,1"))
        end
    end
}

t[#t+1] = Def.ActorFrame {
    Name = "DisplayScoreFrame",
    InitCommand = function(self)
        self:xy(actuals.LeftTextColumn1NumbersMargin, actuals.WifePercentUpperGap)
    end,

    LoadFont("Common Normal") .. {
        Name = "WifePercent",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(largerTextSize)
            self:maxwidth((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / largerTextSize - textzoomFudge)
            self:settext("99.99%")
        end,
        ColorConfigUpdatedMessageCommand = function(self)
            self:playcommand("Set")
        end,
        SetCommand = function(self, params)
            if displayScore then
                local wife = displayScore:GetWifeScore()
                local wifestr = checkWifeStr(wife)
                self:settext(wifestr)
                self:diffuse(colorByGrade(displayScore:GetWifeGrade()))
            else
                self:settext("")
            end
        end
    },
    LoadFont("Common Normal") .. {
        Name = "CurScoreInfoIndicator",
        InitCommand = function(self)
            self:valign(0):halign(1)
            -- bump
            self:y(self:GetZoomedHeight() * largerTextSize)
            self:zoom(displayScoreInfoTextSize)
            self:maxwidth((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / displayScoreInfoTextSize - textzoomFudge)
            registerActorToColorConfigElement(self, "main", "SecondaryText")
        end,
        BeginCommand = function(self)
            --self:x(-self:GetParent():GetX() +(actuals.LeftTextColumn1LabelsMargin + actuals.LeftTextColumn1NumbersMargin) / 2)
            self:y(self:GetParent():GetChild("WifePercent"):GetZoomedHeight() + displayScoreBump)
        end,
        SetCommand = function(self, params)
            if displayScore then
                local wvstr = "W"..displayScore:GetWifeVers()

                local rate = notShit.round(displayScore:GetMusicRate(), 3)
                local notCurRate = notShit.round(getCurRateValue(), 3) ~= rate
                if notCurRate then
                    local ratestr = string.format("%.2f", rate) .. "x"
                    self:settextf("%s [%s]", wvstr, ratestr)
                else
                    self:settext(wvstr)
                end
            else
                self:settext("")
            end
        end
    }
}

t[#t+1] = UIElements.SpriteButton(1, 1, nil) .. {
    Name = "CDTitle",
    InitCommand = function(self)
        -- lets... avoid aligning this.
        -- we want to try to avoid moving the cdtitle a lot
        -- so find the center position of the measure coordinates
        local leftEdge = actuals.CDTitleLeftGap
        local rightEdge = actuals.VerticalDividerLeftGap - actuals.CDTitleRightGap
        local bottomEdge = actuals.CDTitleUpperGap + actuals.CDTitleAllowedHeight
        local topEdge = actuals.CDTitleUpperGap
        local cX = (leftEdge + rightEdge) / 2
        local cY = (bottomEdge + topEdge) / 2
        self:xy(cX, cY)
    end,
    SetCommand = function(self, params)
        self:finishtweening()
        self.song = params.song
        if params.song then
            self:diffusealpha(1)

            -- load the cdtitle if it is present
            if params.song:HasCDTitle() then
                self:Load(params.song:GetCDTitlePath())
            else
                -- otherwise, load the invisible (blank) image and stretch it so you can still hover it
                self:Load(THEME:GetPathG("", "_blank"))

                local allowedWidth = actuals.VerticalDividerLeftGap - actuals.CDTitleRightGap - actuals.CDTitleLeftGap
                self:zoomto(allowedWidth, actuals.CDTitleAllowedHeight)
                return
            end

            local h = self:GetHeight()
            local w = self:GetWidth()
            local allowedWidth = actuals.VerticalDividerLeftGap - actuals.CDTitleRightGap - actuals.CDTitleLeftGap
            if h >= actuals.CDTitleAllowedHeight and w >= allowedWidth then
                if h * (allowedWidth / actuals.CDTitleAllowedHeight) >= w then
                    self:zoom(actuals.CDTitleAllowedHeight / h)
                else
                    self:zoom(allowedWidth / w)
                end
            elseif h >= actuals.CDTitleAllowedHeight then
                self:zoom(actuals.CDTitleAllowedHeight / h)
            elseif w >= allowedWidth then
                self:zoom(allowedWidth / w)
            else
                self:zoom(1)
            end
        else
            self:diffusealpha(0)
        end
        if isOver(self) then
            self:playcommand("ToolTip")
        end
    end,
    ToolTipCommand = function(self)
        if isOver(self) then
            if self.song and not self:IsInvisible() then
                local auth = self.song:GetOrTryAtLeastToGetSimfileAuthor()
                if auth and #auth > 0 then
                    TOOLTIP:SetText(auth)
                    TOOLTIP:Show()
                end
            else
                TOOLTIP:Hide()
            end
        end
    end,
    MouseOverCommand = function(self)
        if self:IsInvisible() then return end
        self:playcommand("ToolTip")
    end,
    MouseOutCommand = function(self)
        if self:IsInvisible() then return end
        TOOLTIP:Hide()
    end,
}

t[#t+1] = createStatLines()
t[#t+1] = createTopSkillsetLines()
t[#t+1] = createMSDLines()
t[#t+1] = createTagDisplays()

return t
