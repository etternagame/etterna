local spacing = 10 -- Spacing between the judgetypes
local frameWidth = 60 -- Width of the Frame
local frameHeight = ((#jdgT + 1) * spacing) -- Height of the Frame
local judgeFontSize = 0.40 -- Font sizes for different text elements
local countFontSize = 0.35
local gradeFontSize = 0.45

local t = Def.ActorFrame {
	Name = "JudgeCounter",
	InitCommand = function(self)
		self:xy(MovableValues.JudgeCounterX, MovableValues.JudgeCounterY)
	end,
	OnCommand = function(self)
		for i = 1, #jdgT do
			jdgCounts[jdgT[i]] = self:GetChild(jdgT[i])
		end
	end,
	SpottedOffsetCommand = function(self)
		if jdgCur and jdgCounts[jdgCur] ~= nil then
			jdgCounter[jdgCur]:settext(jdgct)
		end
	end,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:zoomto(frameWidth, frameHeight):diffuse(color("0,0,0,0.4"))
		end
	},
}

local function makeJudgeText(judge, index)
	return LoadFont("Common normal") .. {
        InitCommand = function(self)
            self:xy(-frameWidth / 2 + 5, -frameHeight / 2 + (index * spacing)):zoom(judgeFontSize):halign(0)
        end,
        OnCommand = function(self)
            settext(self, getShortJudgeStrings(judge))
            diffuse(self, jcT[judge])
        end
    }
end

local function makeJudgeCount(judge, index)
	return LoadFont("Common Normal") .. {
        Name = judge,
        InitCommand = function(self)
            self:xy(frameWidth / 2 - 5, -frameHeight / 2 + (index * spacing)):zoom(countFontSize):halign(1):settext(0)
        end,
        PracticeModeResetMessageCommand= function(self)
            self:settext(0)
        end
    }
end

for i = 1, #jdgT do
	t[#t+1] = makeJudgeText(jdgT[i], i)
	t[#t+1] = makeJudgeCount(jdgT[i], i)
end

return t