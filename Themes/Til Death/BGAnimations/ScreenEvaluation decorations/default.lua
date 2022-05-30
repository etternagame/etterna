local t = Def.ActorFrame {}

local scoreType = themeConfig:get_data().global.DefaultScoreType

if GAMESTATE:GetNumPlayersEnabled() == 1 and themeConfig:get_data().eval.ScoreBoardEnabled then
	if Var("LoadingScreen") == "ScreenNetEvaluation" then
		t[#t + 1] = LoadActor("MPscoreboard")
	else
		t[#t + 1] = LoadActor("scoreboard")
	end
end

local translated_info = {
	CCOn = THEME:GetString("ScreenEvaluation", "ChordCohesionOn"),
	MAPARatio = THEME:GetString("ScreenEvaluation", "MAPARatio")
}

local tso = ms.JudgeScalers
local originaljudge = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty())

-- im going to cry
local aboutToForceWindowSettings = false

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

local judge = PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty()
local judge2 = judge
local score = SCOREMAN:GetMostRecentScore()
if not score then
	score = SCOREMAN:GetTempReplayScore()
end

t[#t + 1] = LoadFont("Common Large") .. {
	InitCommand = function(self)
		self:xy(SCREEN_CENTER_X, capWideScale(124, 150)):zoom(0.25):maxwidth(capWideScale(250 / 0.25, 180 / 0.25))
	end,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
	end
}

t[#t + 1] = LoadFont("Common Large") .. {
	InitCommand = function(self)
		self:xy(SCREEN_CENTER_X, capWideScale(139, 165)):zoom(0.25):maxwidth(180 / 0.25)
	end,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		if GAMESTATE:IsCourseMode() then
			self:settext(GAMESTATE:GetCurrentCourse():GetScripter())
		else
			self:settext(GAMESTATE:GetCurrentSong():GetDisplayArtist())
		end
	end
}

-- Rate String
t[#t + 1] = LoadFont("Common Large") .. {
	InitCommand = function(self)
		self:xy(SCREEN_CENTER_X, capWideScale(154, 180)):zoom(0.25):halign(0.5)
		self:queuecommand("Set")
	end,
	ScoreChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		local top = SCREENMAN:GetTopScreen()
		local rate
		if top:GetName() == "ScreenNetEvaluation" then
			rate = score:GetMusicRate()
		else
			rate = top:GetReplayRate()
			if not rate then rate = getCurRateValue() end
		end
		rate = notShit.round(rate,3)
		local ratestr = getRateString(rate)
		if ratestr == "1x" then
			self:settext("")
		else
			self:settext(ratestr)
		end
	end
}

-- lifegraph
local function GraphDisplay(pn)
	local t = Def.ActorFrame {
		Def.GraphDisplay {
			InitCommand = function(self)
				self:Load("GraphDisplay")
			end,
			BeginCommand = function(self)
				local ss = SCREENMAN:GetTopScreen():GetStageStats()
				self:Set(ss, ss:GetPlayerStageStats())
				self:diffusealpha(0.7)
				self:GetChild("Line"):diffusealpha(0)
				self:zoom(0.8)
				self:xy(-22, 8)
			end,
			ScoreChangedMessageCommand = function(self)
				if score and judge then
					self:playcommand("RecalculateGraphs", {judge=judge})
				end
			end,
			RecalculateGraphsMessageCommand = function(self, params)
				-- called by the end of a codemessagecommand somewhere else
				if not tso[params.judge] then return end
				local success = SCREENMAN:GetTopScreen():SetPlayerStageStatsFromReplayData(SCREENMAN:GetTopScreen():GetStageStats():GetPlayerStageStats(), tso[params.judge], score)
				if not success then return end
				self:playcommand("Begin")
				MESSAGEMAN:Broadcast("SetComboGraph")
			end
		}
	}
	return t
end

local function ComboGraph(pn)
	local t = Def.ActorFrame {
		Def.ComboGraph {
			InitCommand = function(self)
				self:Load("ComboGraph" .. ToEnumShortString(pn))
			end,
			BeginCommand = function(self)
				local ss = SCREENMAN:GetTopScreen():GetStageStats()
				self:Set(ss, ss:GetPlayerStageStats())
				self:zoom(0.8)
				self:xy(-22, -2)
			end,
			SetComboGraphMessageCommand = function(self)
				self:Clear()
				self:Load("ComboGraph" .. ToEnumShortString(pn))
				self:playcommand("Begin")
			end
		}
	}
	return t
end

--ScoreBoard
local judges = {
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss"
}

local dvt = {}
local totalTaps

local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats()

-- a helper to get the radar value for a score and fall back to playerstagestats if that fails
local function gatherRadarValue(radar, score)
    local n = score:GetRadarValues():GetValue(radar)
    if n == -1 then
        return pss:GetRadarActual():GetValue(radar)
    end
    return n
end

local getRescoreElements = function(score)
	local o = {}
	o["dvt"] = dvt
    o["totalHolds"] = pss:GetRadarPossible():GetValue("RadarCategory_Holds") + pss:GetRadarPossible():GetValue("RadarCategory_Rolls")
	o["holdsHit"] = gatherRadarValue("RadarCategory_Holds", score) + gatherRadarValue("RadarCategory_Rolls", score)
	o["holdsMissed"] = o["totalHolds"] - o["holdsHit"]
    o["minesHit"] = pss:GetRadarPossible():GetValue("RadarCategory_Mines") - gatherRadarValue("RadarCategory_Mines", score)
	o["totalTaps"] = totalTaps
	return o
end

local frameX = 20
local frameY = 140
local frameWidth = SCREEN_CENTER_X - 120

local function scoreBoard(pn, position)
	local dvtTmp = {}
	local tvt = {}
	dvt = {}
	totalTaps = 0

	local function setupNewScoreData(score)
		local dvtTmp = score:GetOffsetVector()
		local tvt = score:GetTapNoteTypeVector()
		-- if available, filter out non taps from the deviation list
		-- (hitting mines directly without filtering would make them appear here)
		if tvt ~= nil and #tvt > 0 then
			dvt = {}
			for i, d in ipairs(dvtTmp) do
				local ty = tvt[i]
				if ty == "TapNoteType_Tap" or ty == "TapNoteType_HoldHead" or ty == "TapNoteType_Lift" then
					dvt[#dvt+1] = d
				end
			end
		else
			dvt = dvtTmp
		end

		totalTaps = 0
		for k, v in ipairs(judges) do
			totalTaps = totalTaps + score:GetTapNoteScore(v)
		end
	end
	setupNewScoreData(score)

	-- we removed j1-3 so uhhh this stops things lazily
	local function clampJudge()
		if judge < 4 then judge = 4 end
		if judge > 9 then judge = 9 end
	end
	clampJudge()

	local t = Def.ActorFrame {
		Name = "BLah",
		BeginCommand = function(self)
			if position == 1 then
				self:x(SCREEN_WIDTH - (frameX * 2) - frameWidth)
			end
			if PREFSMAN:GetPreference("SortBySSRNormPercent") then
				judge = 4
				judge2 = judge
				-- you ever hack something so hard?
				aboutToForceWindowSettings = true
				MESSAGEMAN:Broadcast("ForceWindow", {judge=4})
				MESSAGEMAN:Broadcast("RecalculateGraphs", {judge=4})
			else
				judge = scaleToJudge(SCREENMAN:GetTopScreen():GetReplayJudge())
				clampJudge()
				judge2 = judge
				MESSAGEMAN:Broadcast("ForceWindow", {judge=judge})
				MESSAGEMAN:Broadcast("RecalculateGraphs", {judge=judge})
			end
		end,
		ChangeScoreCommand = function(self, params)
			if params.score then
				score = params.score

				setupNewScoreData(score)
			end

			MESSAGEMAN:Broadcast("ScoreChanged")
		end,
		UpdateNetEvalStatsMessageCommand = function(self)
			local s = SCREENMAN:GetTopScreen():GetHighScore()
			if s then
				score = s
			end
			local dvtTmp = score:GetOffsetVector()
			local tvt = score:GetTapNoteTypeVector()
			-- if available, filter out non taps from the deviation list
			-- (hitting mines directly without filtering would make them appear here)
			if tvt ~= nil and #tvt > 0 then
				dvt = {}
				for i, d in ipairs(dvtTmp) do
					local ty = tvt[i]
					if ty == "TapNoteType_Tap" or ty == "TapNoteType_HoldHead" or ty == "TapNoteType_Lift" then
						dvt[#dvt+1] = d
					end
				end
			else
				dvt = dvtTmp
			end
			totalTaps = 0
			for k, v in ipairs(judges) do
				totalTaps = totalTaps + score:GetTapNoteScore(v)
			end
			MESSAGEMAN:Broadcast("ScoreChanged")
		end
	}
	t[#t + 1] = Def.Quad {
		InitCommand = function(self)
			self:xy(frameX - 5, frameY + 5):zoomto(frameWidth + 10, 217):halign(0):valign(0)
			self:diffuse(getMainColor("tabs"))
		end
	}
	t[#t + 1] = Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY + 30):zoomto(frameWidth, 2):halign(0):diffuse(getMainColor("highlight")):diffusealpha(0.5)
		end
	}
	t[#t + 1] = Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY + 55):zoomto(frameWidth, 2):halign(0):diffuse(getMainColor("highlight")):diffusealpha(0.5)
		end
	}

	t[#t + 1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:xy(frameX + 3, frameY + 32):zoom(0.5):halign(0):valign(0):maxwidth(200)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		ScoreChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local top = SCREENMAN:GetTopScreen()
			local rate
			if top:GetName() == "ScreenNetEvaluation" then
				rate = score:GetMusicRate()
			else
				rate = top:GetReplayRate()
				if not rate then rate = getCurRateValue() end
			end
			rate = notShit.round(rate,3)
			local meter = GAMESTATE:GetCurrentSteps():GetMSD(rate, 1)
			if meter < 10 then
				self:settextf("%4.2f", meter)
			else
				self:settextf("%5.2f", meter)
			end
			self:diffuse(byMSD(meter))
		end
	}
	t[#t + 1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:xy(frameWidth + frameX - 3, frameY + 32):zoom(0.5):halign(1):valign(0):maxwidth(200)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		ScoreChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local meter = score:GetSkillsetSSR("Overall")
			self:settextf("%5.2f", meter)
			self:diffuse(byMSD(meter))
		end
	}
	t[#t + 1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:xy(frameWidth + frameX - 3, frameY + 7):zoom(0.5):halign(1):valign(0):maxwidth(200)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local steps = GAMESTATE:GetCurrentSteps()
			local diff = getDifficulty(steps:GetDifficulty())
			self:settext(getShortDifficulty(diff))
			self:diffuse(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(), steps:GetDifficulty())))
		end
	}

	-- Wife percent
	t[#t + 1] = Def.ActorFrame {
		InitCommand = function(self)
			self:SetUpdateFunction(function(self)
				self:queuecommand("PercentMouseover")
			end)
		end,
		ForceWindowMessageCommand = function(self, params)
			self:playcommand("Set")
		end,
		Def.Quad {
			InitCommand = function(self)
				self:xy(frameX + 3, frameY + 9):zoomto(capWideScale(320,490)/2.2,20):halign(0):valign(0)
				self:diffusealpha(0)
			end,
			PercentMouseoverCommand = function(self)
				if isOver(self) and self:IsVisible() then
					self:GetParent():GetChild("NormalText"):visible(false)
					self:GetParent():GetChild("LongerText"):visible(true)
				else
					self:GetParent():GetChild("NormalText"):visible(true)
					self:GetParent():GetChild("LongerText"):visible(false)
				end
			end
		},
		LoadFont("Common Large") .. {
			Name = "NormalText",
			InitCommand = function(self)
				self:xy(frameX + 3, frameY + 9):zoom(0.45):halign(0):valign(0):maxwidth(capWideScale(320, 500))
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local wv = score:GetWifeVers()
				local js = judge ~= 9 and judge or "ustice"
				local rescoretable = getRescoreElements(score)
				local rescorepercent = getRescoredWife3Judge(3, judge, rescoretable)
				wv = 3 -- this should really only be applicable if we can convert the score
				local ws = "Wife" .. wv .. " J"
				self:diffuse(getGradeColor(score:GetWifeGrade()))
				self:settextf(
					"%05.2f%% (%s)",
					notShit.floor(rescorepercent, 2), ws .. js
				)
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CodeMessageCommand = function(self, params)
				local rescoretable = getRescoreElements(score)
				local rescorepercent = 0
				local ws = "Wife3" .. " J"
				if params.Name == "PrevJudge" and judge > 4 then
					judge = judge - 1
					clampJudge()
					rescorepercent = getRescoredWife3Judge(3, judge, rescoretable)
					self:settextf(
						"%05.2f%% (%s)", notShit.floor(rescorepercent, 2), ws .. judge
					)
					MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
				elseif params.Name == "NextJudge" and judge < 9 then
					judge = judge + 1
					clampJudge()
					rescorepercent = getRescoredWife3Judge(3, judge, rescoretable)
					local js = judge ~= 9 and judge or "ustice"
						self:settextf(
							"%05.2f%% (%s)", notShit.floor(rescorepercent, 2), ws .. js
					)
					MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
				end
				if params.Name == "ResetJudge" then
					judge = GetTimingDifficulty()
					clampJudge()
					self:playcommand("Set")
					MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
				end
			end
		},
		LoadFont("Common Large") ..	{-- high precision rollover
			Name = "LongerText",
			InitCommand = function(self)
				self:xy(frameX + 3, frameY + 9):zoom(0.45):halign(0):valign(0):maxwidth(capWideScale(320, 500))
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local wv = score:GetWifeVers()
				local js = judge ~= 9 and judge or "ustice"
				local rescoretable = getRescoreElements(score)
				local rescorepercent = getRescoredWife3Judge(3, judge, rescoretable)
				wv = 3 -- this should really only be applicable if we can convert the score
				local ws = "Wife" .. wv .. " J"
				self:diffuse(getGradeColor(score:GetWifeGrade()))
				self:settextf(
					"%05.5f%% (%s)",
					notShit.floor(rescorepercent, 5), ws .. js
				)
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CodeMessageCommand = function(self, params)
				local rescoretable = getRescoreElements(score)
				local rescorepercent = 0
				local wv = score:GetWifeVers()
				local ws = "Wife3" .. " J"
				if params.Name == "PrevJudge" and judge2 > 4 then
					judge2 = judge2 - 1
					rescorepercent = getRescoredWife3Judge(3, judge2, rescoretable)
					self:settextf(
						"%05.5f%% (%s)", notShit.floor(rescorepercent, 5), ws .. judge2
					)
				elseif params.Name == "NextJudge" and judge2 < 9 then
					judge2 = judge2 + 1
					rescorepercent = getRescoredWife3Judge(3, judge2, rescoretable)
					local js = judge2 ~= 9 and judge2 or "ustice"
					self:settextf(
					"%05.5f%% (%s)", notShit.floor(rescorepercent, 5), ws .. js
				)
				end
				if params.Name == "ResetJudge" then
					judge2 = GetTimingDifficulty()
					self:playcommand("Set")
				end
			end
		}
	}

	t[#t + 1] = LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:xy(frameX + 2.4, frameY + 63):zoom(0.40):halign(0):maxwidth(frameWidth / 0.41)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local mstring = GAMESTATE:GetPlayerState():GetPlayerOptionsString("ModsLevel_Current")
			local ss = SCREENMAN:GetTopScreen():GetStageStats()
			if not ss:GetLivePlay() then
				mstring = SCREENMAN:GetTopScreen():GetReplayModifiers()
			end
			self:settext(getModifierTranslations(mstring))
		end,
		ScoreChangedMessageCommand = function(self)
			local mstring = score:GetModifiers()
			self:settext(getModifierTranslations(mstring))
		end
	}

	for k, v in ipairs(judges) do
		t[#t + 1] = Def.Quad {
			InitCommand = function(self)
				self:xy(frameX, frameY + 80 + ((k - 1) * 22)):zoomto(frameWidth, 18):halign(0):diffuse(byJudgment(v)):diffusealpha(
					0.5
				)
			end
		}
		t[#t + 1] = Def.Quad {
			InitCommand = function(self)
				self:xy(frameX, frameY + 80 + ((k - 1) * 22)):zoomto(0, 18):halign(0):diffuse(byJudgment(v)):diffusealpha(0.5)
			end,
			BeginCommand = function(self)
				self:glowshift():effectcolor1(color("1,1,1," .. tostring(score:GetTapNoteScore(v) / totalTaps * 0.4))):effectcolor2(
					color("1,1,1,0")
				)
				if aboutToForceWindowSettings then return end
				self:sleep(0.2):smooth(1.5):zoomx(frameWidth * score:GetTapNoteScore(v) / totalTaps)
			end,
			ForceWindowMessageCommand = function(self, params)
				local rescoreJudges = getRescoredJudge(dvt, judge, k)
				self:finishtweening():smooth(0.2):zoomx(frameWidth * rescoreJudges / totalTaps)
			end,
			ScoreChangedMessageCommand = function(self)
				self:zoomx(frameWidth * score:GetTapNoteScore(v) / totalTaps)
			end,
			CodeMessageCommand = function(self, params)
				if params.Name == "PrevJudge" or params.Name == "NextJudge" then
					local rescoreJudges = getRescoredJudge(dvt, judge, k)
					self:finishtweening():bounceend(0.2):zoomx(frameWidth * rescoreJudges / totalTaps)
				end
				if params.Name == "ResetJudge" then
					self:finishtweening():bounceend(0.2):zoomx(frameWidth * score:GetTapNoteScore(v) / totalTaps)
				end
			end
		}
		t[#t + 1] = LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:xy(frameX + 10, frameY + 79.3 + ((k - 1) * 22)):zoom(0.25):halign(0)
			end,
			BeginCommand = function(self)
				if aboutToForceWindowSettings then return end
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settext(getJudgeStrings(v))
			end,
			ForceWindowMessageCommand = function(self, params)
				self:playcommand("Set")
			end,
			CodeMessageCommand = function(self, params)
				if params.Name == "ResetJudge" then
					self:playcommand("Set")
				end
			end
		}
		t[#t + 1] = LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:xy(frameX + frameWidth - 40, frameY + 79.3 + ((k - 1) * 22)):zoom(0.25):halign(1)
			end,
			BeginCommand = function(self)
				if aboutToForceWindowSettings then return end
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settext(score:GetTapNoteScore(v))
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ForceWindowMessageCommand = function(self, params)
				self:settext(getRescoredJudge(dvt, judge, k))
			end,
			CodeMessageCommand = function(self, params)
				if params.Name == "PrevJudge" or params.Name == "NextJudge" then
					self:settext(getRescoredJudge(dvt, judge, k))
				end
				if params.Name == "ResetJudge" then
					self:playcommand("Set")
				end
			end
		}
		t[#t + 1] = LoadFont("Common Normal") .. {
			InitCommand = function(self)
				self:xy(frameX + frameWidth - 38, frameY + 79.7 + ((k - 1) * 22)):zoom(0.3):halign(0)
			end,
			BeginCommand = function(self)
				if aboutToForceWindowSettings then return end
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settextf("(%03.2f%%)", score:GetTapNoteScore(v) / totalTaps * 100)
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ForceWindowMessageCommand = function(self, params)
				local rescoredJudge
				rescoredJudge = getRescoredJudge(dvt, params.judge, k)
				self:settextf("(%03.2f%%)", rescoredJudge / totalTaps * 100)
			end,
			CodeMessageCommand = function(self, params)
				if params.Name == "PrevJudge" or params.Name == "NextJudge" then

					local rescoredJudge = getRescoredJudge(dvt, judge, k)
					self:settextf("(%03.2f%%)", rescoredJudge / totalTaps * 100)
				end
				if params.Name == "ResetJudge" then
					self:playcommand("Set")
				end
			end
		}
	end

	t[#t + 1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:xy(frameX + 3, frameY + 210):zoom(0.25):halign(0)
			self:maxwidth(capWideScale(get43size(100), 160)/0.25)
			self:settext(translated_info["CCOn"])
			self:visible(false)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		ScoreChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if score:GetChordCohesion() then
				self:visible(true)
			else
				self:visible(false)
			end
		end
	}

	--[[
	The following section first adds the ratioText and the maRatio. Then the paRatio is added and positioned. The right
	values for maRatio and paRatio are then filled in. Finally ratioText and maRatio are aligned to paRatio.
	--]]
	local ratioText, maRatio, paRatio, marvelousTaps, perfectTaps, greatTaps
	t[#t + 1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			ratioText = self
			self:settextf("%s:", translated_info["MAPARatio"])
			self:zoom(0.25):halign(1)
		end
	}
	t[#t + 1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			maRatio = self
			self:zoom(0.25):halign(1):diffuse(byJudgment(judges[1]))
		end
	}
	t[#t + 1] = LoadFont("Common Large") .. {
		InitCommand = function(self)
			paRatio = self
			self:xy(frameWidth + frameX, frameY + 210):zoom(0.25):halign(1):diffuse(byJudgment(judges[2]))

			marvelousTaps = score:GetTapNoteScore(judges[1])
			perfectTaps = score:GetTapNoteScore(judges[2])
			greatTaps = score:GetTapNoteScore(judges[3])
			self:playcommand("Set")
		end,
		SetCommand = function(self)

			-- Fill in maRatio and paRatio
			maRatio:settextf("%.1f:1", marvelousTaps / perfectTaps)
			paRatio:settextf("%.1f:1", perfectTaps / greatTaps)

			-- Align ratioText and maRatio to paRatio (self)
			maRatioX = paRatio:GetX() - paRatio:GetZoomedWidth() - 10
			maRatio:xy(maRatioX, paRatio:GetY())

			ratioTextX = maRatioX - maRatio:GetZoomedWidth() - 10
			ratioText:xy(ratioTextX, paRatio:GetY())
			if score:GetChordCohesion() == true then
				maRatio:maxwidth(maRatio:GetZoomedWidth()/0.25)
				self:maxwidth(self:GetZoomedWidth()/0.25)
				ratioText:maxwidth(capWideScale(get43size(65), 85)/0.27)
			end
		end,
		CodeMessageCommand = function(self, params)
			if params.Name == "PrevJudge" or params.Name == "NextJudge" then
					marvelousTaps = getRescoredJudge(dvt, judge, 1)
					perfectTaps = getRescoredJudge(dvt, judge, 2)
					greatTaps = getRescoredJudge(dvt, judge, 3)
				self:playcommand("Set")
			end
			if params.Name == "ResetJudge" then
				marvelousTaps = score:GetTapNoteScore(judges[1])
				perfectTaps = score:GetTapNoteScore(judges[2])
				greatTaps = score:GetTapNoteScore(judges[3])
				self:playcommand("Set")
			end
		end,
		ForceWindowMessageCommand = function(self)
			marvelousTaps = getRescoredJudge(dvt, judge, 1)
			perfectTaps = getRescoredJudge(dvt, judge, 2)
			greatTaps = getRescoredJudge(dvt, judge, 3)
			self:playcommand("Set")
		end,
		ScoreChangedMessageCommand = function(self)
			self:playcommand("ForceWindow")
		end
	}

	local fart = {"Holds", "Mines", "Rolls", "Lifts", "Fakes"}
	local fart_translated = {
		Holds = THEME:GetString("RadarCategory", "Holds"),
		Mines = THEME:GetString("RadarCategory", "Mines"),
		Rolls = THEME:GetString("RadarCategory", "Rolls"),
		Lifts = THEME:GetString("RadarCategory", "Lifts"),
		Fakes = THEME:GetString("RadarCategory", "Fakes")
	}
	t[#t + 1] = Def.Quad {
		InitCommand = function(self)
			self:xy(frameX - 5, frameY + 226):zoomto(frameWidth / 2 - 10, 56.5):halign(0):valign(0)
			self:diffuse(getMainColor("tabs"))
		end
	}
	for i = 1, #fart do
		t[#t + 1] = LoadFont("Common Normal") .. {
			InitCommand = function(self)
				self:xy(frameX, frameY + 224 + 10 * i):zoom(0.4):halign(0):settext(fart_translated[fart[i]])
			end
		}
		t[#t + 1] = LoadFont("Common Normal") .. {
			InitCommand = function(self)
				self:xy(frameWidth / 2, frameY + 224 + 10 * i):zoom(0.4):halign(1)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settextf(
					"%03d/%03d",
					gatherRadarValue("RadarCategory_" .. fart[i], score),
					score:GetRadarPossible():GetValue("RadarCategory_" .. fart[i])
				)
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
	end

	local function hahahahahaha(score)
		local tracks = score:GetTrackVector()
		local devianceTable = score:GetOffsetVector() or {}

		local cbl = 0
		local cbr = 0
		local cbm = 0
		local tst = ms.JudgeScalers
		local tso = tst[judge]
		local ncol = GAMESTATE:GetCurrentSteps():GetNumColumns() - 1
		local middleCol = ncol/2

		for i = 1, #devianceTable do
			if tracks[i] then
				if math.abs(devianceTable[i]) > tso * 90 then
					if tracks[i] < middleCol then
						cbl = cbl + 1
					elseif tracks[i] > middleCol then
						cbr = cbr + 1
					else
						cbm = cbm + 1
					end
				end
			end
		end

		t[#t + 1] = Def.Quad {
			InitCommand = function(self)
				self:xy(frameWidth + 25, frameY + 230):zoomto(frameWidth / 2 + 10, 60):halign(1):valign(0)
				self:diffuse(getMainColor("tabs"))
			end
		}
		local smallest, largest = wifeRange(devianceTable)
		local doot = {
			THEME:GetString("ScreenEvaluation", "Mean"),
			THEME:GetString("ScreenEvaluation", "StandardDev"),
			THEME:GetString("ScreenEvaluation", "LargestDev"),
			THEME:GetString("ScreenEvaluation", "LeftCB"),
			THEME:GetString("ScreenEvaluation", "RightCB"),
			THEME:GetString("ScreenEvaluation", "MiddleCB")
		}

		local mcscoot = {
			wifeMean(devianceTable),
			wifeSd(devianceTable),
			largest,
			cbl,
			cbr,
			cbm
		}
		return mcscoot
	end

	-- stats stuff
	local tracks = score:GetTrackVector()
	local devianceTable = score:GetOffsetVector()
	local cbl = 0
	local cbr = 0
	local cbm = 0

	-- basic per-hand stats to be expanded on later
	local tst = ms.JudgeScalers
	local tso = tst[judge]
	local ncol = GAMESTATE:GetCurrentSteps():GetNumColumns() - 1 -- cpp indexing -mina
	local middleCol = ncol/2

	if devianceTable then
		for i = 1, #devianceTable do
			if tracks[i] then	-- we dont load track data when reconstructing eval screen apparently so we have to nil check -mina
				if math.abs(devianceTable[i]) > tso * 90 then
					if tracks[i] < middleCol then
						cbl = cbl + 1
					elseif tracks[i] > middleCol then
						cbr = cbr + 1
					else
						cbm = cbm + 1
					end
				end
			end
		end


	t[#t + 1] = Def.Quad {
		InitCommand = function(self)
			self:diffuse(getMainColor("tabs"))
			self:xy(frameWidth + 25, frameY + 226):zoomto(frameWidth / 2 + 10, 56.5):halign(1):valign(0)
		end
	}
	local smallest, largest = wifeRange(devianceTable)
	local doot = {
		THEME:GetString("ScreenEvaluation", "Mean"),
		THEME:GetString("ScreenEvaluation", "StandardDev"),
		THEME:GetString("ScreenEvaluation", "LargestDev"),
		THEME:GetString("ScreenEvaluation", "LeftCB"),
		THEME:GetString("ScreenEvaluation", "RightCB"),
		THEME:GetString("ScreenEvaluation", "MiddleCB")
	}
	local mcscoot = {
		wifeMean(devianceTable),
		wifeSd(devianceTable),
		largest,
		cbl,
		cbr,
		cbm
	}

	-- if theres a middle lane, display its cbs too
	local lines = ((ncol+1) % 2 == 0) and #doot-1  or #doot
	local tzoom = lines == 5 and 0.4 or 0.3
	local ySpacing = lines == 5 and 10 or 8.5

	for i = 1, lines do
		t[#t + 1] = LoadFont("Common Normal") .. {
			InitCommand = function(self)
				self:xy(frameX + capWideScale(get43size(130), 153), frameY + 224 + ySpacing * i):zoom(tzoom):halign(0):settext(doot[i])
			end
		}
		t[#t + 1] = LoadFont("Common Normal") .. {
			Name=i,
			InitCommand = function(self)
				if i < 4 then
					self:xy(frameWidth + 20, frameY + 224 + ySpacing * i):zoom(tzoom):halign(1):settextf("%5.2fms", mcscoot[i])
				else
					self:xy(frameWidth + 20, frameY + 224 + ySpacing * i):zoom(tzoom):halign(1):settext(mcscoot[i])
				end
			end,
			ChangeScoreCommand = function(self, params)
				local mcscoot = hahahahahaha(params.score)
				if i < 4 then
					self:xy(frameWidth + 20, frameY + 224 + ySpacing * i):zoom(tzoom):halign(1):settextf("%5.2fms", mcscoot[i])
				else
					self:xy(frameWidth + 20, frameY + 224 + ySpacing * i):zoom(tzoom):halign(1):settext(mcscoot[i])
				end
			end,
			CodeMessageCommand = function(self, params)
				local j = tonumber(self:GetName())
				if j > 3 and (params.Name == "PrevJudge" or params.Name == "NextJudge") then
					if j == 4 then
						local tso = tst[judge]
						mcscoot[j] = 0
						mcscoot[j+1] = 0
						for i = 1, #devianceTable do
							if tracks[i] then	-- it would probably make sense to move all this to c++
								if math.abs(devianceTable[i]) > tso * 90 then
									if tracks[i] <= math.floor(ncol/2) then
										mcscoot[j] = mcscoot[j] + 1
									else
										mcscoot[j+1] = mcscoot[j+1] + 1
									end
								end
							end
						end
					end
					self:xy(frameWidth + 20, frameY + 224 + 10 * j):zoom(0.4):halign(1):settext(mcscoot[j])
				end
			end,
			ForceWindowMessageCommand = function(self)
				local j = tonumber(self:GetName())
				if j > 3 then
					if j == 4 then
						local tso = tst[judge]
						mcscoot[j] = 0
						mcscoot[j+1] = 0
						for i = 1, #devianceTable do
							if tracks[i] then	-- it would probably make sense to move all this to c++
								if math.abs(devianceTable[i]) > tso * 90 then
									if tracks[i] <= math.floor(ncol/2) then
										mcscoot[j] = mcscoot[j] + 1
									else
										mcscoot[j+1] = mcscoot[j+1] + 1
									end
								end
							end
						end
					end
					self:xy(frameWidth + 20, frameY + 224 + 10 * j):zoom(0.4):halign(1):settext(mcscoot[j])
				end
			end
		}
	end
end
	return t
end

if GAMESTATE:IsPlayerEnabled() then
	t[#t + 1] = scoreBoard(PLAYER_1, 0)
	t[#t + 1] = StandardDecorationFromTable("GraphDisplay" .. ToEnumShortString(PLAYER_1), GraphDisplay(PLAYER_1))
	t[#t + 1] = StandardDecorationFromTable("ComboGraph" .. ToEnumShortString(PLAYER_1), ComboGraph(PLAYER_1))
end

t[#t + 1] = LoadActor("../offsetplot")
updateDiscordStatus(true)

return t
