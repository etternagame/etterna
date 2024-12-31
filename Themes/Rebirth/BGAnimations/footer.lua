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

quoteortip = LoadFont("Common Normal") .. {
    Name = "QuoteOrTip",
    InitCommand = function(self)
        self:halign(0)
        self:xy(actuals.TextHorizontalPadding, -actuals.Height / 2)
        self:zoom(textSize)
        self:maxwidth(actuals.Width * allowedPercentageForQuote / textSize - textZoomFudge)
        self:settext(getRandomQuote(themeConfig:get_data().global.TipType))
        registerActorToColorConfigElement(self, "main", "PrimaryText")
    end
}

gradecounter = LoadFont("Common Normal") .. {
    Name = "GradeCounter",
    InitCommand = function(self)
        local aaaaa = WHEELDATA:GetTotalClearsByGrade("Grade_Tier01")
        local aaaa = WHEELDATA:GetTotalClearsByGrade("Grade_Tier02") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier03") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier04")
        local aaa = WHEELDATA:GetTotalClearsByGrade("Grade_Tier05") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier06") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier07")
        local aa = WHEELDATA:GetTotalClearsByGrade("Grade_Tier08") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier09") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier10")
        local a = WHEELDATA:GetTotalClearsByGrade("Grade_Tier11") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier12") + WHEELDATA:GetTotalClearsByGrade("Grade_Tier13")
        self:xy(actuals.Width * allowedPercentageForQuote / 2, -actuals.Height / 2)
        self:zoom(textSize)
        self:maxwidth(actuals.Width * allowedPercentageForQuote / textSize - textZoomFudge)
        self:settextf("%d %s   %d %s   %d %s   %d %s   %d %s",
        aaaaa, getGradeStrings("Grade_Tier01"),
        aaaa, getGradeStrings("Grade_Tier04"),
        aaa, getGradeStrings("Grade_Tier07"),
        aa, getGradeStrings("Grade_Tier10"),
        a, getGradeStrings("Grade_Tier13"))
        registerActorToColorConfigElement(self, "main", "PrimaryText")
        -- uh
        local yea = 0
        function attributive(when, gren) --
            yea = yea + when
            self:AddAttribute(yea, {Length = #getGradeStrings(gren), Diffuse = colorByGrade(gren)})
        end
        attributive(#tostring(aaaaa) + 1, "Grade_Tier01")
        attributive(3 + #getGradeStrings("Grade_Tier01") + #tostring(aaaa) + 1, "Grade_Tier04")
        attributive(3 + #getGradeStrings("Grade_Tier04") + #tostring(aaa) + 1, "Grade_Tier07")
        attributive(3 + #getGradeStrings("Grade_Tier07") + #tostring(aa) + 1, "Grade_Tier10")
        attributive(3 + #getGradeStrings("Grade_Tier10") + #tostring(a) + 1, "Grade_Tier13")
    end,
}

tiptype = themeConfig:get_data().global.TipType

if tiptype == 3 then
    t[#t+1] = gradecounter
elseif tiptype == 1 or tiptype == 2 then
    t[#t+1] = quoteortip
end

return t