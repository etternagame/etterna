local t = Def.ActorFrame {Name = "GeneralBoxFile"}

local ratios = {
    LeftGap = 1056 / 1920, -- left side of screen to left edge of frame
    TopGap = 497 / 1080, -- top of screen to top of frame
    Width = 384 / 1920,
    Height = 571 / 1080,
    HorizontalDividerGap = 6 / 1920, -- this is 6 on both sides
    HorizontalDividerLowerGap = 113 / 1080, -- from bottom to center of divider
    HorizontalDividerLength = 367 / 1920,
    VerticalDividerLeftGap = 196 / 1920, -- from left to center of divider
    VerticalDividerTopGap = 195 / 1080,
    VerticalDividerLength = 247 / 1080,
    DividerThickness = 2 / 1920, -- ehhh
    UpperLipHeight = 43 / 1080,
    
    LeftTextLeftGap = 12 / 1920, -- from left edge to left edge of text
    LeftTextRightGap = 24 / 1920, -- from center of divider to right edge of text
    MSDTextGap = 173 / 1920, -- from left edge to RIGHT edge of text
    MSDTextTopGap = 96 / 1080, -- from top of frame to center of text

    BottomTextLine1 = 43 / 1080, -- from bottom to center of lowest line
    BottomTextLine2 = 85 / 1080, -- second lowest line
    MidTextLine6 = 159 / 1080, -- from bottom to center of lowest middle line
    MidTextLine5 = 201 / 1080,
    MidTextLine4 = 243 / 1080,
    MidTextLine3 = 285 / 1080,
    MidTextLine2 = 327 / 1080,
    MidTextLine1 = 369 / 1080,

    RightTextLeftGap = 12 / 1920,
    PageTextTopGap = 24 / 1080, -- from top edge to center of text
}

local actuals = {
    LeftGap = ratios.LeftGap * SCREEN_WIDTH,
    TopGap = ratios.TopGap * SCREEN_HEIGHT,
    Width = ratios.Width * SCREEN_WIDTH,
    Height = ratios.Height * SCREEN_HEIGHT,
    HorizontalDividerGap = ratios.HorizontalDividerGap * SCREEN_WIDTH,
    HorizontalDividerLowerGap = ratios.HorizontalDividerLowerGap * SCREEN_HEIGHT,
    HorizontalDividerLength = ratios.HorizontalDividerLength * SCREEN_WIDTH,
    VerticalDividerLeftGap = ratios.VerticalDividerLeftGap * SCREEN_WIDTH,
    VerticalDividerTopGap = ratios.VerticalDividerTopGap * SCREEN_HEIGHT,
    VerticalDividerLength = ratios.VerticalDividerLength * SCREEN_HEIGHT,
    DividerThickness = 2, -- a constant
    UpperLipHeight = ratios.UpperLipHeight * SCREEN_HEIGHT,
    LeftTextLeftGap = ratios.LeftTextLeftGap * SCREEN_WIDTH,
    LeftTextRightGap = ratios.LeftTextRightGap * SCREEN_WIDTH,
    MSDTextGap = ratios.MSDTextGap * SCREEN_WIDTH,
    MSDTextTopGap = ratios.MSDTextTopGap * SCREEN_HEIGHT,
    BottomTextLine1 = ratios.BottomTextLine1 * SCREEN_HEIGHT,
    BottomTextLine2 = ratios.BottomTextLine2 * SCREEN_HEIGHT,
    MidTextLine6 = ratios.MidTextLine6 * SCREEN_HEIGHT,
    MidTextLine5 = ratios.MidTextLine5 * SCREEN_HEIGHT,
    MidTextLine4 = ratios.MidTextLine4 * SCREEN_HEIGHT,
    MidTextLine3 = ratios.MidTextLine3 * SCREEN_HEIGHT,
    MidTextLine2 = ratios.MidTextLine2 * SCREEN_HEIGHT,
    MidTextLine1 = ratios.MidTextLine1 * SCREEN_HEIGHT,
    RightTextLeftGap = ratios.RightTextLeftGap * SCREEN_WIDTH,
    PageTextTopGap = ratios.PageTextTopGap * SCREEN_HEIGHT,
}

-- these names needs to match an actorframe name below
local choiceNames = {
    "General",
    "Breakdown",
}

local statNames = {
    "Notes",
    "Jumps",
    "Hands",
    "Holds",
    "Rolls",
    "Mines",
}

local choiceTextSize = 0.9
local generalTabTextSize = 0.85
local textzoomFudge = 5

local function createChoices()
    local function createChoice(i)
        return LoadFont("Common Normal") .. {
            Name = choiceNames[i],
            InitCommand = function(self)
                -- this position is the center of the text
                -- divides the space into slots for the choices then places them half way into them
                -- should work for any count of choices
                -- and the maxwidth will make sure they stay nonoverlapping
                self:x((actuals.Width / #choiceNames) * (i-1) + (actuals.Width / #choiceNames / 2))
                self:zoom(choiceTextSize)
                self:maxwidth(actuals.Width / #choiceNames / choiceTextSize - textzoomFudge)
                self:settext(choiceNames[i])
            end
        }
    end
    local t = Def.ActorFrame {
        Name = "Choices",
        InitCommand = function(self)
            self:y(actuals.PageTextTopGap)
        end
    }
    for i = 1, #choiceNames do
        t[#t+1] = createChoice(i)
    end
    return t
end

local function createStatLines()
    local function createStatLine(i)
        return Def.ActorFrame {
            Name = "Stat"..i,
            InitCommand = function(self)
                self:y(-actuals["MidTextLine"..i])
            end,

            LoadFont("Common Normal") .. {
                Name = "Label",
                InitCommand = function(self)
                    self:x(actuals.LeftTextLeftGap)
                    self:halign(0)
                    self:zoom(generalTabTextSize)
                    -- dont fudge this one since it is static and introduces extra squish
                    self:maxwidth(((actuals.VerticalDividerLeftGap - actuals.LeftTextLeftGap) / 2) / generalTabTextSize - textzoomFudge)
                    self:settextf("%s:", statNames[i])
                end
            },
            LoadFont("Common Normal") .. {
                Name = "Count",
                InitCommand = function(self)
                    self:x(actuals.VerticalDividerLeftGap - actuals.LeftTextRightGap)
                    self:halign(1)
                    self:zoom(generalTabTextSize)
                    self:maxwidth(((actuals.VerticalDividerLeftGap - actuals.LeftTextLeftGap) / 2) / generalTabTextSize - textzoomFudge)
                    self:settext("1234")
                end
            }
        }
    end
    local t = Def.ActorFrame {
        Name = "Stats",
        InitCommand = function(self)
            self:y(actuals.Height)
        end
    }
    for i = 1, #statNames do
        t[#t+1] = createStatLine(i)
    end
    return t
end

t[#t+1] = Def.ActorFrame {
    Name = "GeneralFrame",
    InitCommand = function(self)
        self:xy(actuals.LeftGap, actuals.TopGap)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.Height)
            self:diffuse(color("0,0,0,0.6"))
        end
    },
    Def.Quad {
        Name = "UpperLip",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(actuals.Width, actuals.UpperLipHeight)
            self:diffuse(color("0,0,0,0.6"))
        end
    },

    createChoices(),

    Def.ActorFrame {
        Name = "General",
        Def.Quad {
            Name = "VerticalDivider",
            InitCommand = function(self)
                self:valign(0)
                self:xy(actuals.VerticalDividerLeftGap, actuals.VerticalDividerTopGap)
                self:zoomto(actuals.DividerThickness, actuals.VerticalDividerLength)
            end
        },
        Def.Quad {
            Name = "HorizontalDivider",
            InitCommand = function(self)
                self:halign(0)
                self:xy(actuals.HorizontalDividerGap, actuals.Height - actuals.HorizontalDividerLowerGap)
                self:zoomto(actuals.HorizontalDividerLength, actuals.DividerThickness)
            end
        },
        createStatLines()
    },
    Def.ActorFrame {
        Name = "Breakdown",
    }
}

return t