-- if score is ever nil we done goofed way before this screen is ever loaded -mina
local score = getOnlineScoreForPlot()
local replayData = score.replaydata

local tst = {1.50, 1.33, 1.16, 1.00, 0.84, 0.66, 0.50, 0.33, 0.20}
local plotWidth, plotHeight = SCREEN_WIDTH, SCREEN_WIDTH * 0.3
local plotX, plotY = SCREEN_CENTER_X, SCREEN_CENTER_Y
local dotDims, plotMargin = 2, 4
local judge = GetTimingDifficulty()
local maxOffset = math.max(180, 180 * tst[judge])

local o =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(plotX, plotY)
	end,
	CodeMessageCommand = function(self, params)
		if
			params.Name == "PlotCancel" or params.Name == "PlotExit" or params.Name == "PlotThickens" or
				params.Name == "PlotTwist" or
				params.Name == "StarPlot64" or
				params.Name == "SheriffOfPlottingham"
		 then
			SCREENMAN:GetTopScreen():Cancel()
		end
		if params.Name == "PrevJudge" and judge > 1 then
			judge = judge - 1
		elseif params.Name == "NextJudge" and judge < 9 then
			judge = judge + 1
		end
		maxOffset = math.max(180, 180 * tst[judge])
		MESSAGEMAN:Broadcast("JudgeDisplayChanged")
	end
}

local td = GAMESTATE:GetCurrentSteps(PLAYER_1):GetTimingData()
local finalSecond = GAMESTATE:GetCurrentSong(PLAYER_1):GetLastSecond()

local function fitX(x) -- Scale time values to fit within plot width.
	return x / finalSecond * plotWidth - plotWidth / 2
end

local function fitY(y) -- Scale offset values to fit within plot height
	return -1 * y / maxOffset * plotHeight / 2
end

local function plotOffset(nr, dv)
	if dv == 1000 then -- 1000 denotes a miss for which we use a different marker
		return Def.Quad {
			InitCommand = function(self)
				self:xy(fitX(nr), fitY(math.max(184, tst[judge] * 184))):zoomto(dotDims, dotDims):diffuse(
					offsetToJudgeColor(dv / 1000)
				):valign(0)
			end
		}
	end
	return Def.Quad {
		InitCommand = function(self)
			self:xy(fitX(nr), fitY(dv)):zoomto(dotDims, dotDims):diffuse(offsetToJudgeColor(dv / 1000))
		end,
		JudgeDisplayChangedMessageCommand = function(self)
			local pos = fitY(dv)
			if math.abs(pos) > plotHeight / 2 then
				self:y(fitY(math.max(184, tst[judge] * 184)))
			else
				self:y(pos)
			end
			self:diffuse(offsetToJudgeColor(dv / 1000, tst[judge]))
		end
	}
end

-- Center Bar
o[#o + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:zoomto(plotWidth + plotMargin, 1):diffuse(byJudgment("TapNoteScore_W1"))
	end
}
local fantabars = {22.5, 45, 90, 135}
local bantafars = {"TapNoteScore_W2", "TapNoteScore_W3", "TapNoteScore_W4", "TapNoteScore_W5"}
for i = 1, #fantabars do
	o[#o + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:y(fitY(tst[judge] * fantabars[i])):zoomto(plotWidth + plotMargin, 1):diffuse(byJudgment(bantafars[i]))
		end
	}
	o[#o + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:y(fitY(-tst[judge] * fantabars[i])):zoomto(plotWidth + plotMargin, 1):diffuse(byJudgment(bantafars[i]))
		end
	}
end
-- Background
o[#o + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:zoomto(plotWidth + plotMargin, plotHeight + plotMargin):diffuse(color("0.05,0.05,0.05,0.05")):diffusealpha(0.95)
	end
}
-- Convert noterows to timestamps and plot dots
for i = 1, #replayData do
	o[#o + 1] = plotOffset(replayData[i][1], replayData[i][2])
end

-- Early/Late markers
o[#o + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(-plotWidth / 2, -plotHeight / 2 + 2):settextf("Late (+%ims)", maxOffset):zoom(0.35):halign(0):valign(0)
		end,
		JudgeDisplayChangedMessageCommand = function(self)
			self:settextf("Late (+%ims)", maxOffset)
		end
	}
o[#o + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(-plotWidth / 2, plotHeight / 2 - 2):settextf("Early (-%ims)", maxOffset):zoom(0.35):halign(0):valign(1)
		end,
		JudgeDisplayChangedMessageCommand = function(self)
			self:settextf("Early (-%ims)", maxOffset)
		end
	}

return o
