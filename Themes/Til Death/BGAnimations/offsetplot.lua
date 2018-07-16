-- Note this file is different from the scoretaboffsetplot because it takes values from pss and not a highscore object
-- this diffrentiation should probably be handled in a single file to reduce code redundancy -mina

local judges = { "marv", "perf", "great", "good", "boo", "miss" }
local tst = { 1.50,1.33,1.16,1.00,0.84,0.66,0.50,0.33,0.20 }
local judge = GetTimingDifficulty()
local tso = tst[judge]

local enabledCustomWindows = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomEvaluationWindowTimings
judge = enabledCustomWindows and 0 or judge
local customWindowsData = timingWindowConfig:get_data()
local customWindows = customWindowsData.customWindows
local customWindow

local plotWidth, plotHeight = 400,120
local plotX, plotY = SCREEN_WIDTH - 9 - plotWidth/2, SCREEN_HEIGHT - 56 - plotHeight/2
local dotDims, plotMargin = 2, 4
local maxOffset = math.max(180, 180*tso)

local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1)
local dvt = pss:GetOffsetVector()
local nrt = pss:GetNoteRowVector()
local td = GAMESTATE:GetCurrentSteps(PLAYER_1):GetTimingData()
local wuab = {}
local finalSecond = GAMESTATE:GetCurrentSong(PLAYER_1):GetLastSecond()

local function fitX(x)	-- Scale time values to fit within plot width.
	return x/finalSecond*plotWidth - plotWidth/2
end

local function fitY(y)	-- Scale offset values to fit within plot height
	return -1*y/maxOffset*plotHeight/2
end

local o = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(plotX,plotY)
		MESSAGEMAN:Broadcast("JudgeDisplayChanged")		-- prim really handled all this much more elegantly
	end,
	CodeMessageCommand=function(self,params)
		if enabledCustomWindows then
			if params.Name == "PrevJudge" then
				judge = judge < 2 and #customWindows or judge - 1
				customWindow = customWindowsData[customWindows[judge]]
			elseif params.Name == "NextJudge" then
				judge = judge == #customWindows and 1 or judge + 1
				customWindow = customWindowsData[customWindows[judge]]
			end
		elseif params.Name == "PrevJudge" and judge > 1 then
			judge = judge - 1
			tso = tst[judge]
		elseif params.Name == "NextJudge" and judge < 9 then
			judge = judge + 1
			tso = tst[judge]
		end
		if params.Name == "ResetJudge" then
			judge = enabledCustomWindows and 0 or GetTimingDifficulty()
			tso = tst[GetTimingDifficulty()]
		end
		maxOffset = (enabledCustomWindows and judge ~= 0) and customWindow.judgeWindows.boo or math.max(180, 180*tso)
		MESSAGEMAN:Broadcast("JudgeDisplayChanged")
	end,
	UpdateNetEvalStatsMessageCommand = function(self)
		local s = SCREENMAN:GetTopScreen():GetHighScore()
		if s then
			score = s
			dvt = score:GetOffsetVector()
			nrt = score:GetNoteRowVector()
			for i=1,#nrt do
				wuab[i] = td:GetElapsedTimeFromNoteRow(nrt[i])
			end
		end
		MESSAGEMAN:Broadcast("JudgeDisplayChanged")
	end,
}
-- Center Bar
o[#o+1] = Def.Quad{
	InitCommand=function(self)
		self:zoomto(plotWidth+plotMargin,1):diffuse(byJudgment("TapNoteScore_W1"))
	end
}
local fantabars = {22.5, 45, 90, 135}
local bantafars = {"TapNoteScore_W2", "TapNoteScore_W3", "TapNoteScore_W4", "TapNoteScore_W5"}
for i=1, #fantabars do 
	o[#o+1] = Def.Quad{
		InitCommand=function(self)
			self:y( fitY(tso*fantabars[i])):zoomto(plotWidth+plotMargin,1):diffuse(byJudgment(bantafars[i]))
		end,
		JudgeDisplayChangedMessageCommand=function(self)
			local fit = (enabledCustomWindows and judge ~= 0) and customWindow.judgeWindows[judges[i]] or tso*fantabars[i]
			self:y( fitY(fit))
		end,
	}
	o[#o+1] = Def.Quad{
		InitCommand=function(self)
			self:y( fitY(-tso*fantabars[i])):zoomto(plotWidth+plotMargin,1):diffuse(byJudgment(bantafars[i]))
		end,
		JudgeDisplayChangedMessageCommand=function(self)
			local fit = (enabledCustomWindows and judge ~= 0) and customWindow.judgeWindows[judges[i]] or tso*fantabars[i]
			self:y( fitY(-fit))
		end,
	}
end
-- Background
o[#o+1] = Def.Quad{InitCommand=function(self)
	self:zoomto(plotWidth+plotMargin,plotHeight+plotMargin):diffuse(color("0.05,0.05,0.05,0.05")):diffusealpha(0.8)
end}
-- Convert noterows to timestamps and plot dots
for i=1,#nrt do
	wuab[i] = td:GetElapsedTimeFromNoteRow(nrt[i])
end

local dotWidth = dotDims / 2
o[#o+1] = Def.ActorMultiVertex{
	JudgeDisplayChangedMessageCommand=function(self)
		local verts = {}
		for i=1,#dvt do
			local x = fitX(wuab[i])
			local y = fitY(dvt[i])
			local fit = (enabledCustomWindows and judge ~= 0) and customWindow.judgeWindows.boo + 3 or math.max(183, 183*tso)
			local color = (enabledCustomWindows and judge ~= 0) and customOffsetToJudgeColor(dvt[i], customWindow.judgeWindows) or offsetToJudgeColor(dvt[i]/1000, tst[judge])
			if math.abs(y) > plotHeight/2 then
				y = fitY(fit)
			end
			verts[#verts+1] = {{x-dotWidth,y+dotWidth,0}, color}
			verts[#verts+1] = {{x+dotWidth,y+dotWidth,0}, color}
			verts[#verts+1] = {{x+dotWidth,y-dotWidth,0}, color}
			verts[#verts+1] = {{x-dotWidth,y-dotWidth,0}, color}
		end
		self:SetVertices(verts)
		self:SetDrawState{Mode="DrawMode_Quads", First = 1, Num=#verts}
	end
}

-- Early/Late markers
o[#o+1] = LoadFont("Common Normal")..{
	InitCommand=function(self)
		self:xy(-plotWidth/2,-plotHeight/2+2):settextf("Late (+%ims)", maxOffset):zoom(0.35):halign(0):valign(0)
	end,
	JudgeDisplayChangedMessageCommand=function(self)
		self:settextf("Late (+%ims)", maxOffset)
	end
}
o[#o+1] = LoadFont("Common Normal")..{
	InitCommand=function(self)
		self:xy(-plotWidth/2,plotHeight/2-2):settextf("Early (-%ims)", maxOffset):zoom(0.35):halign(0):valign(1)
	end,
	JudgeDisplayChangedMessageCommand=function(self)
		self:settextf("Early (-%ims)", maxOffset)
	end
}

return o