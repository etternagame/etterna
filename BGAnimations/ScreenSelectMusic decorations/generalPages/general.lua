local t = Def.ActorFrame {Name = "GeneralPageFile"}

local ratios = {
    VerticalDividerLeftGap = 387 / 1920, -- from left edge to left edge of divider
    VerticalDividerUpperGap = 68 / 1080, -- from top edge to top edge
    VerticalDividerHeight = 374 / 1080,
    HorizontalDividerLeftGap = 11 / 1920, -- from left edge to left edge of divider
    HorizontalDividerUpperGap = 457 / 1080, -- from top edge to top edge
    HorizontalDividerLength = 753 / 1920,
    DividerThickness = 2 / 1080, -- consistently 2 pixels basically

    LeftTextColumn1NumbersMargin = 172 / 1920, -- from left edge to right edge of text
    LeftTextColumn1LabelsMargin = 12 / 1920, -- from left edge to left edge of text
    LeftTextColumn2Margin = 207 / 1920, -- from left edge to left edge
    LeftTextUpperGap = 195 / 1080, -- from top edge to top edge
    -- use the column1 and column2 x positions for the tag locations as well
    MSDUpperGap = 69 / 1080, -- top edge to top edge
    WifePercentUpperGap = 127 / 1080, -- top edge to top edge

    RightTextLabelsMargin = 327 / 1920, -- from right edge to left edge of text
    RightTextNumbersMargin = 66 / 1920, -- from right edge to right edge of text

    LeftTextAllottedVerticalSpace = 210 / 1080, -- from top edge of top text to top edge of bottom text
    -- right text also has allotted space but we will extrapolate from this number
    TagTextUpperGap = 479 / 1080, -- from top edge to top edge
    TagTextAllottedVerticalSpace = 42 / 1080, -- from top edge of top text to top edge of bottom text
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

local statNames = {
    "Notes",
    "Jumps",
    "Hands",
    "Holds",
    "Rolls",
    "Mines",
}
local msdNames = {
    "Average NPS",
    "Stream",
    "Jumpstream",
    "Handstream",
    "Stamina",
    "JackSpeed",
    "Chordjack",
    "Technical",
}

local mainTextSize = 1
local largerTextSize = 1.3
local textzoomFudge = 5

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
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Count",
                InitCommand = function(self)
                    self:x(actuals.LeftTextColumn1NumbersMargin)
                    self:halign(1):valign(0)
                    self:zoom(mainTextSize)
                    self:maxwidth(((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / 2) / mainTextSize - textzoomFudge)
                    self:settext("1234")
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
                    self:settextf("%s:", msdNames[i])
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Number",
                InitCommand = function(self)
                    self:x(actuals.Width - actuals.RightTextNumbersMargin)
                    self:halign(1):valign(0)
                    self:zoom(mainTextSize)
                    self:maxwidth(((actuals.RightTextLabelsMargin - actuals.RightTextNumbersMargin) / 2) / mainTextSize - textzoomFudge)
                    self:settext("12.34")
                end
            }
        }
    end
    local t = Def.ActorFrame {Name = "MSDLines"}
    for i = 1, #msdNames do
        t[#t+1] = createMSDLine(i)
    end
    return t
end

-- only accounting for room for 4 tags
local function createTagDisplays()
    local function createTagDisplay(i)
        local xPos = i < 3 and actuals.LeftTextColumn1LabelsMargin or actuals.LeftTextColumn2Margin
        return LoadFont("Common Normal") .. {
            Name = "Tag"..i,
            InitCommand = function(self)
                self:xy(xPos, actuals.TagTextUpperGap + (actuals.TagTextAllottedVerticalSpace * ((i-1) % 2)))
                self:halign(0):valign(0)
                self:zoom(mainTextSize)
                self:maxwidth((actuals.VerticalDividerLeftGap - actuals.LeftTextColumn1LabelsMargin - actuals.LeftTextColumn2Margin) / mainTextSize - textzoomFudge)
                self:settext("Placeholder Tag?")
            end
        }
    end
    local t = Def.ActorFrame {Name = "TagDisplays"}
    for i = 1, 4 do
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
    end
}

t[#t+1] = Def.Quad {
    Name = "VerticalDivider",
    InitCommand = function(self)
        self:xy(actuals.VerticalDividerLeftGap, actuals.VerticalDividerUpperGap)
        self:zoomto(actuals.DividerThickness, actuals.VerticalDividerHeight)
        self:halign(0):valign(0)
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "MSD",
    InitCommand = function(self)
        self:xy(actuals.LeftTextColumn1NumbersMargin, actuals.MSDUpperGap)
        self:halign(1):valign(0)
        self:zoom(largerTextSize)
        self:maxwidth((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / largerTextSize - textzoomFudge)
        self:settext("24.24")
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "WifePercent",
    InitCommand = function(self)
        self:xy(actuals.LeftTextColumn1NumbersMargin, actuals.WifePercentUpperGap)
        self:halign(1):valign(0)
        self:zoom(largerTextSize)
        self:maxwidth((actuals.LeftTextColumn1NumbersMargin - actuals.LeftTextColumn1LabelsMargin) / largerTextSize - textzoomFudge)
        self:settext("99.99%")
    end
}

t[#t+1] = createStatLines()
t[#t+1] = createTopSkillsetLines()
t[#t+1] = createMSDLines()
t[#t+1] = createTagDisplays()

return t