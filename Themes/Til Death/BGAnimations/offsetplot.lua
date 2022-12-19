-- updated to handle both immediate evaluation when pulling data from pss (doesnt require invoking calls to loadreplay data) and scoretab plot construction (does) -mina

local judges = {"marv", "perf", "great", "good", "boo", "miss"}
local tst = ms.JudgeScalers
local judge = GetTimingDifficulty()
local tso = tst[judge]

local plotWidth, plotHeight = 400, 120
local plotX, plotY = SCREEN_WIDTH - 5 - plotWidth / 2, SCREEN_HEIGHT - 59.5 - plotHeight / 2
local dotDims, plotMargin = 2, 4
local maxOffset = math.max(180, 180 * tso)
local baralpha = 0.2
local bgalpha = 0.8
local textzoom = 0.35
local forcedWindow = false

local translated_info = {
	Left = THEME:GetString("OffsetPlot", "ExplainLeft"),
	Middle = THEME:GetString("OffsetPlot", "ExplainMiddle"),
	Right = THEME:GetString("OffsetPlot", "ExplainRight"),
	Down = THEME:GetString("OffsetPlot", "ExplainDown"),
	Early = THEME:GetString("OffsetPlot", "Early"),
	Late = THEME:GetString("OffsetPlot", "Late"),
	SD = THEME:GetString("ScreenEvaluation", "StandardDev"),
	Mean = THEME:GetString("ScreenEvaluation", "Mean"),
	UsingReprioritized = THEME:GetString("OffsetPlot", "UsingReprioritized"),
	TapNoteScore_W1 = getJudgeStrings("TapNoteScore_W1"),
	TapNoteScore_W2 = getJudgeStrings("TapNoteScore_W2"),
	TapNoteScore_W3 = getJudgeStrings("TapNoteScore_W3"),
	TapNoteScore_W4 = getJudgeStrings("TapNoteScore_W4"),
	TapNoteScore_W5 = getJudgeStrings("TapNoteScore_W5"),
	TapNoteScore_Miss = getJudgeStrings("TapNoteScore_Miss"),
}

-- initialize tables we need for replay data here, we don't know where we'll be loading from yet
local dvt = {}
local nrt = {}
local ctt = {}
local ntt = {}
local wuab = {}
local finalSecond = GAMESTATE:GetCurrentSteps():GetLastSecond()
local td = GAMESTATE:GetCurrentSteps():GetTimingData()
local oddColumns = false
local middleColumn = 1.5 -- middle column for 4k but accounting for trackvector indexing at 0

local handspecific = false
local left = false
local down = false
local up = false
local right = false
local middle = false
local usingCustomWindows = false

local function fitX(x) -- Scale time values to fit within plot width.
	if finalSecond == 0 then
		return 0
	end
	return x / finalSecond * plotWidth - plotWidth / 2
end

local function fitY(y) -- Scale offset values to fit within plot height
	return -1 * y / maxOffset * plotHeight / 2
end

local function HighlightUpdaterThing(self)
	self:GetChild("BGQuad"):queuecommand("Highlight")
end

-- we removed j1-3 so uhhh this stops things lazily
local function clampJudge()
	if judge < 4 then judge = 4 end
	if judge > 9 then judge = 9 end
end
clampJudge()

local function scaleToJudge(scale)
	scale = notShit.round(scale, 2)
	local scales = ms.JudgeScalers
	local out = 4
	for k,v in pairs(scales) do
		if v == scale then
			out = k
		end
	end
	return out
end

-- convert a plot x position to a noterow
local function convertXToRow(x)
	local output = x + plotWidth/2
	output = output / plotWidth

	if output < 0 then output = 0 end
	if output > 1 then output = 1 end

	-- the 48 here is how many noterows there are per beat
	-- this is a const defined in the game
	-- and i sure hope it doesnt ever change
	local td = GAMESTATE:GetCurrentSteps():GetTimingData()
	local row = td:GetBeatFromElapsedTime(output * finalSecond) * 48

	return row
end

local o = Def.ActorFrame {
	Name = "OffsetPlot",
	OnCommand = function(self)
		self:xy(plotX, plotY)
		-- being explicit about the logic since atm these are the only 2 cases we handle
		local name = SCREENMAN:GetTopScreen():GetName()
		if name == "ScreenNetEvaluation" then -- moving away from grabbing anything in pss, dont want to mess with net stuff atm
			if not forcedWindow then
				judge = scaleToJudge(SCREENMAN:GetTopScreen():GetReplayJudge())
				clampJudge()
				tso = tst[judge]
			end
			local allowHovering = not SCREENMAN:GetTopScreen():ScoreUsedInvalidModifier()
			if allowHovering then
				self:SetUpdateFunction(HighlightUpdaterThing)
			end
			local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats()
			dvt = pss:GetOffsetVector()
			nrt = pss:GetNoteRowVector()
			ctt = pss:GetTrackVector() -- column information for each offset
			ntt = pss:GetTapNoteTypeVector() -- notetype information (we use this to handle mine hits differently- currently that means not displaying them)
		else -- should be default behavior
			if name == "ScreenScoreTabOffsetPlot" then
				local score = getScoreForPlot()
				plotWidth, plotHeight = SCREEN_WIDTH, SCREEN_WIDTH * 0.3
				self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
				textzoom = 0.5
				bgalpha = 1
				if score ~= nil then
					if score:HasReplayData() then
						dvt = score:GetOffsetVector()
						nrt = score:GetNoteRowVector()
						ctt = score:GetTrackVector()
						ntt = score:GetTapNoteTypeVector()
					end
				end
			else
				local allowHovering = not SCREENMAN:GetTopScreen():ScoreUsedInvalidModifier()
				if allowHovering then
					self:SetUpdateFunction(HighlightUpdaterThing)
				end
			end
		end

		-- missing noterows. this happens with many online replays.
		-- we can approximate, but i dont want to do that right now.
		if nrt == nil then
			return
		end

		oddColumns = GAMESTATE:GetCurrentStyle():ColumnsPerPlayer() % 2 ~= 0 -- hopefully the style is consistently set here
		middleColumn = (GAMESTATE:GetCurrentStyle():ColumnsPerPlayer()-1) / 2.0

		-- Convert noterows to timestamps and plot dots (this is important it determines plot x values!!!)
		wuab = {}
		for i = 1, #nrt do
			wuab[i] = td:GetElapsedTimeFromNoteRow(nrt[i])
		end

		MESSAGEMAN:Broadcast("JudgeDisplayChanged") -- prim really handled all this much more elegantly
	end,
	SetFromDisplayMessageCommand = function(self, params)
		if params.score then
			self:playcommand("SetFromScore", params)
		end
	end,
	SetFromScoreCommand = function(self, params)
		if params.score then
			local score = params.score

			if score:HasReplayData() then
				dvt = score:GetOffsetVector()
				nrt = score:GetNoteRowVector()
				ctt = score:GetTrackVector()
				ntt = score:GetTapNoteTypeVector()
			end

			wuab = {}
			for i = 1, #nrt do
				wuab[i] = td:GetElapsedTimeFromNoteRow(nrt[i])
			end

			MESSAGEMAN:Broadcast("JudgeDisplayChanged")
		end
	end,
	LoadedCustomWindowMessageCommand = function(self)
		usingCustomWindows = true
		local replay = REPLAYS:GetActiveReplay()
		wuab = {}
		dvt = replay:GetOffsetVector()
		nrt = replay:GetNoteRowVector()
		ctt = replay:GetTrackVector()
		ntt = replay:GetTapNoteTypeVector()
		for i = 1, #nrt do
			wuab [i] = td:GetElapsedTimeFromNoteRow(nrt[i])
		end

		MESSAGEMAN:Broadcast("JudgeDisplayChanged")
	end,
	UnloadedCustomWindowMessageCommand = function(self)
		usingCustomWindows = false
	end,
	CodeMessageCommand = function(self, params)
		if usingCustomWindows then return end

		if params.Name == "PrevJudge" and judge > 1 then
			judge = judge - 1
			clampJudge()
			tso = tst[judge]
		elseif params.Name == "NextJudge" and judge < 9 then
			judge = judge + 1
			clampJudge()
			tso = tst[judge]
		end
		if params.Name == "ToggleHands" and #ctt > 0 then --super ghetto toggle -mina
			if not handspecific then -- moving from none to left
				handspecific = true
				left = true
			elseif handspecific and left then
				down = true
				left = false
			elseif handspecific and down then
				down = false
				up = true
			elseif handspecific and up then
				up = false
				right = true
			elseif handspecific and right then -- moving from right to none
				right = false
				handspecific = false
			end
			MESSAGEMAN:Broadcast("JudgeDisplayChanged")
		end
		if params.Name == "ResetJudge" then
			judge = GetTimingDifficulty()
			clampJudge()
			tso = tst[GetTimingDifficulty()]
		end
		if params.Name ~= "ResetJudge" and params.Name ~= "PrevJudge" and params.Name ~= "NextJudge" and params.Name ~= "ToggleHands" then return end
		maxOffset = math.max(180, 180 * tso)
		MESSAGEMAN:Broadcast("JudgeDisplayChanged")
	end,
	ForceWindowMessageCommand = function(self, params)
		judge = params.judge
		clampJudge()
		tso = tst[judge]
		maxOffset = math.max(180, 180 * tso)
		forcedWindow = true
	end,
	UpdateNetEvalStatsMessageCommand = function(self) -- i haven't updated or tested neteval during last round of work -mina
		local s = SCREENMAN:GetTopScreen():GetHighScore()
		if s then
			score = s
			dvt = score:GetOffsetVector()
			nrt = score:GetNoteRowVector()
			ctt = score:GetTrackVector()
			wuab = {}
			for i = 1, #nrt do
				wuab[i] = td:GetElapsedTimeFromNoteRow(nrt[i])
			end
		end
		MESSAGEMAN:Broadcast("JudgeDisplayChanged")
	end
}
-- Background
o[#o + 1] = Def.Quad {
	Name = "BGQuad",
	JudgeDisplayChangedMessageCommand = function(self)
		self:zoomto(plotWidth + plotMargin, plotHeight + plotMargin)
		self:diffuse(color("0.05,0.05,0.05,0.05"))
		self:diffusealpha(bgalpha)
	end,
	HighlightCommand = function(self)
		local bar = self:GetParent():GetChild("PosBar")
		local txt = self:GetParent():GetChild("PosText")
		local bg = self:GetParent():GetChild("PosBG")
		if isOver(self) then
			local xpos = INPUTFILTER:GetMouseX() - self:GetParent():GetX()
			bar:visible(true)
			txt:visible(true)
			bg:visible(true)
			bar:x(xpos)
			txt:x(xpos - 2)
			bg:x(xpos)
			bg:zoomto(txt:GetZoomedWidth() + 4, txt:GetZoomedHeight() + 4)
			local row = convertXToRow(xpos)
			local replay = REPLAYS:GetActiveReplay()
			local snapshot = replay:GetReplaySnapshotForNoterow(row)
			local judgments = snapshot:GetJudgments()
			local wifescore = snapshot:GetWifePercent() * 100
			local mean = snapshot:GetMean()
			local sd = snapshot:GetStandardDeviation()
			local timebro = td:GetElapsedTimeFromNoteRow(row) / getCurRateValue()

			local marvCount = judgments["W1"]
			local perfCount = judgments["W2"]
			local greatCount = judgments["W3"]
			local goodCount = judgments["W4"]
			local badCount = judgments["W5"]
			local missCount = judgments["Miss"]

			--txt:settextf("x %f\nrow %f\nbeat %f\nfinalsecond %f", xpos, row, row/48, finalSecond)
			-- The odd formatting here is in case we want to add translation support.
			txt:settextf("%f%%\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %0.2fms\n%s: %0.2fms\n%s: %0.2fs",
				wifescore,
				translated_info["TapNoteScore_W1"], marvCount,
				translated_info["TapNoteScore_W2"], perfCount,
				translated_info["TapNoteScore_W3"], greatCount,
				translated_info["TapNoteScore_W4"], goodCount,
				translated_info["TapNoteScore_W5"], badCount,
				translated_info["TapNoteScore_Miss"], missCount,
				translated_info["SD"], sd,
				translated_info["Mean"], mean,
				"Time", timebro
			)
		else
			bar:visible(false)
			txt:visible(false)
			bg:visible(false)
		end
	end
}
o[#o+1] = Def.ActorFrame {
	InitCommand = function(self)
		self:visible(false)
	end,
	JudgeDisplayChangedMessageCommand = function(self)
		self:visible(usingCustomWindows and currentCustomWindowConfigUsesOldestNoteFirst())
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(plotWidth/2,15)
			self:xy(-plotWidth/2 - plotMargin/2, -plotHeight/2 - plotMargin/2)
			self:halign(0):valign(1)
			self:diffuse(color("0.05,0.05,0.05,0.05"))
			self:diffusealpha(bgalpha)
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:xy(-plotWidth/4 - plotMargin/2, -plotHeight/2 - plotMargin/2 - 15/2)
			self:zoom(0.4)
			self:settext(translated_info["UsingReprioritized"])
		end,
	},
}
-- Center Bar
o[#o + 1] = Def.Quad {
	JudgeDisplayChangedMessageCommand = function(self)
		self:zoomto(plotWidth + plotMargin, 1):diffuse(byJudgment("TapNoteScore_W1")):diffusealpha(baralpha)
	end
}
local fantabars = {22.5, 45, 90, 135}
local bantafars = {"TapNoteScore_W2", "TapNoteScore_W3", "TapNoteScore_W4", "TapNoteScore_W5"}
local santabarf = {"TapNoteScore_W1", "TapNoteScore_W2", "TapNoteScore_W3", "TapNoteScore_W4"} -- ugh
for i = 1, #fantabars do
	o[#o + 1] = Def.Quad {
		JudgeDisplayChangedMessageCommand = function(self)
			self:zoomto(plotWidth + plotMargin, 1):diffuse(byJudgment(bantafars[i])):diffusealpha(baralpha)
			local fit = tso * fantabars[i]
			if usingCustomWindows then
				fit = getCustomWindowConfigJudgmentWindow(santabarf[i])
			end
			self:finishtweening()
			self:smooth(0.1)
			self:y(fitY(fit))
		end
	}
	o[#o + 1] = Def.Quad {
		JudgeDisplayChangedMessageCommand = function(self)
			self:zoomto(plotWidth + plotMargin, 1):diffuse(byJudgment(bantafars[i])):diffusealpha(baralpha)
			local fit = tso * fantabars[i]
			if usingCustomWindows then
				fit = getCustomWindowConfigJudgmentWindow(santabarf[i])
			end
			self:finishtweening()
			self:smooth(0.1)
			self:y(fitY(-fit))
		end
	}
end

-- Bar for current mouse position on graph
o[#o + 1] = Def.Quad {
	Name = "PosBar",
	InitCommand = function(self)
		self:visible(false)
		self:zoomto(2, plotHeight + plotMargin):diffuse(color("0.5,0.5,0.5,1"))
	end,
	JudgeDisplayChangedMessageCommand = function(self)
		self:zoomto(2, plotHeight + plotMargin)
	end
}

local dotWidth = dotDims / 2
local function setOffsetVerts(vt, x, y, c)
	vt[#vt + 1] = {{x - dotWidth, y + dotWidth, 0}, c}
	vt[#vt + 1] = {{x + dotWidth, y + dotWidth, 0}, c}
	vt[#vt + 1] = {{x + dotWidth, y - dotWidth, 0}, c}
	vt[#vt + 1] = {{x - dotWidth, y - dotWidth, 0}, c}
end
o[#o + 1] = Def.ActorMultiVertex {
	JudgeDisplayChangedMessageCommand = function(self)
		local verts = {}
		for i = 1, #dvt do
			local x = fitX(wuab[i])
			local y = fitY(dvt[i])
			local fit = math.max(183, 183 * tso)

			-- get the color for the tap
			local cullur = offsetToJudgeColor(dvt[i], tst[judge])
			if usingCustomWindows then
				cullur = customOffsetToJudgeColor(dvt[i], getCurrentCustomWindowConfigJudgmentWindowTable())
			end
			cullur[4] = 1
			local cullurFaded = {}

			if math.abs(y) > plotHeight / 2 then
				y = fitY(fit)
			end

			-- if fading by hands, get the faded color
			if handspecific then
				-- make a copy of the color to get the faded color
				for ind,c in pairs(cullur) do
					cullurFaded[ind] = c
				end
				cullurFaded[4] = 0.28 -- hex 48 is approximately this
			end

			-- remember that time i removed redundancy in this code 2 days ago and then did this -mina
			if ntt[i] ~= "TapNoteType_Mine" then
				if handspecific and left then
					if ctt[i] == 0 then
						setOffsetVerts(verts, x, y, cullur)
					else
						setOffsetVerts(verts, x, y, cullurFaded) -- highlight left
					end
				elseif handspecific and down then
					if ctt[i] == 1 then
						setOffsetVerts(verts, x, y, cullur)
					else
						setOffsetVerts(verts, x, y, cullurFaded)
					end
				elseif handspecific and up then
					if ctt[i] == 2 then
						setOffsetVerts(verts, x, y, cullur)
					else
						setOffsetVerts(verts, x, y, cullurFaded)
					end
				elseif handspecific and right then
					if ctt[i] == 3 then
						setOffsetVerts(verts, x, y, cullur)
					else
						setOffsetVerts(verts, x, y, cullurFaded) -- highlight right
					end
				else
					setOffsetVerts(verts, x, y, cullur)
				end
			end
		end
		self:SetVertices(verts)
		self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = #verts}
	end
}

-- filter
o[#o + 1] = LoadFont("Common Normal") .. {
	JudgeDisplayChangedMessageCommand = function(self)
		self:xy(0, plotHeight / 2 - 2):zoom(textzoom):halign(0.5):valign(1)
		if #ntt > 0 then
			if handspecific then
				if left then
					self:settext("left")
				elseif down then
					self:settext("down")
				elseif up then
					self:settext("up")
				elseif right then
					self:settext("right")
				end
			else
				self:settext(translated_info["Down"])
			end
		else
			self:settext("")
		end
	end
}

-- Early/Late markers
o[#o + 1] = LoadFont("Common Normal") .. {
	JudgeDisplayChangedMessageCommand = function(self)
		self:xy(-plotWidth / 2, -plotHeight / 2 + 2):zoom(textzoom):halign(0):valign(0):settextf("%s (+%ims)", translated_info["Late"], maxOffset)
	end
}
o[#o + 1] = LoadFont("Common Normal") .. {
	JudgeDisplayChangedMessageCommand = function(self)
		self:xy(-plotWidth / 2, plotHeight / 2 - 2):zoom(textzoom):halign(0):valign(1):settextf("%s (-%ims)", translated_info["Early"], maxOffset)
	end
}

-- Background for judgments at mouse position
o[#o + 1] = Def.Quad {
	Name = "PosBG",
	InitCommand = function(self)
		self:valign(1):halign(1):zoomto(30,30):diffuse(color(".1,.1,.1,.45")):y(-plotHeight / 2 - plotMargin)
		self:visible(false)
	end
}

-- Text for judgments at mouse position
o[#o + 1] = LoadFont("Common Normal") .. {
	Name = "PosText",
	InitCommand = function(self)
		self:x(8):valign(1):halign(1):zoom(0.4):y(-plotHeight / 2 - plotMargin - 2)
	end
}

-- Text for current judge window
-- Only for SelectMusic (not Eval)
o[#o + 1] = LoadFont("Common Normal") .. {
	Name = "JudgeText",
	InitCommand = function(self)
		self:valign(0):halign(0):zoom(0.4)
		self:xy(-plotWidth/2, -plotHeight/2)
		self:settext("")
	end,
	OnCommand = function(self)
		local name = SCREENMAN:GetTopScreen():GetName()
		if name ~= "ScreenScoreTabOffsetPlot" then
			self:visible(false)
		end
	end,
	SetCommand = function(self)
		local jdgname = "J" .. judge
		self:settextf("%s", jdgname)
	end,
	JudgeDisplayChangedMessageCommand = function(self)
		self:playcommand("Set")
		self:xy(-plotWidth / 2 + 5, -plotHeight / 2 + 15):zoom(textzoom):halign(0):valign(0)
	end
}

return o
