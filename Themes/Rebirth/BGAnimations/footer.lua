local t = Def.ActorFrame {
    Name = "FooterFile",
    InitCommand = function(self)
        -- all positions should be relative to the bottom left corner of the screen
        self:y(SCREEN_HEIGHT)

        -- update current time
        self:SetUpdateFunction(function(self)
            self:GetChild("CurrentTime"):playcommand("UpdateTime")
        end)
        -- update once per second
        self:SetUpdateFunctionInterval(1)
    end
}

local ratios = {
    TextHorizontalPadding = 15 / 1920, -- distance to move text to align left or right from edge
}

local actuals = {
    TextHorizontalPadding = ratios.TextHorizontalPadding * SCREEN_WIDTH,
}

actuals.Width = Var("Width")
actuals.Height = Var("Height")
if actuals.Width == nil then actuals.Width = SCREEN_WIDTH end
if actuals.Height == nil then actuals.Height = 50 end

-- how much of the visible area can the quote take up before being restricted in width?
local allowedPercentageForQuote = 0.80
local textSize = 0.95
local textZoomFudge = 5


t[#t+1] = Def.Quad {
    Name = "BG",
    InitCommand = function(self)
        self:halign(0):valign(1)
        self:zoomto(actuals.Width, actuals.Height)
        self:diffusealpha(0.75)
        registerActorToColorConfigElement(self, "main", "SecondaryBackground")
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "CurrentTime",
    InitCommand = function(self)
        self:halign(1)
        self:xy(actuals.Width - actuals.TextHorizontalPadding, -actuals.Height / 2)
        self:zoom(textSize)
        self:maxwidth(actuals.Width * (1 - allowedPercentageForQuote) / textSize - textZoomFudge)
        registerActorToColorConfigElement(self, "main", "PrimaryText")
        self:playcommand("UpdateTime")
    end,
    UpdateTimeCommand = function(self)
        local year = Year()
        local month = MonthOfYear() + 1
        local day = DayOfMonth()
        local hour = Hour()
        local minute = Minute()
        local second = Second()
        self:settextf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second)
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "QuoteOrTip",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.TextHorizontalPadding, -actuals.Height / 2)
        self:zoom(textSize)
        self:maxwidth(actuals.Width * allowedPercentageForQuote / textSize - textZoomFudge)
        self:settext(getRandomQuote(3)) -- 3 is the quote thing, only quotes for now
        registerActorToColorConfigElement(self, "main", "PrimaryText")
    end
}

return t