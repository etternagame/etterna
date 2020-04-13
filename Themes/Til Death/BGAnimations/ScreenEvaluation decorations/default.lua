local t = Def.ActorFrame {}

local enabledCustomWindows = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomEvaluationWindowTimings

local customWindows = timingWindowConfig:get_data().customWindows

local scoreType = themeConfig:get_data().global.DefaultScoreType

if GAMESTATE:GetNumPlayersEnabled() == 1 and themeConfig:get_data().eval.ScoreBoardEnabled then
	t[#t + 1] = LoadActor("scoreboard")
end

local translated_info = {
	CCOn = THEME:GetString("ScreenEvaluation", "ChordCohesionOn"),
	MAPARatio = THEME:GetString("ScreenEvaluation", "MAPARatio")
}

local tso = {1.50, 1.33, 1.16, 1.00, 0.84, 0.66, 0.50, 0.33, 0.20}
local originaljudge = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty())

-- im going to cry
local aboutToForceWindowSettings = false

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, capWideScale(135, 150)):zoom(0.4):maxwidth(capWideScale(250 / 0.4, 180 / 0.4))
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
		end
	}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, capWideScale(145, 160)):zoom(0.4):maxwidth(180 / 0.4)
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
t[#t + 1] =
	LoadFont("Common normal") ..
	{
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, capWideScale(155, 170)):zoom(0.5):halign(0.5)
		end,
		BeginCommand = function(self)
			local rate = SCREENMAN:GetTopScreen():GetReplayRate()
			if not rate then rate = getCurRateValue() end
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
	local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(pn)

	local t =
		Def.ActorFrame {
		Def.GraphDisplay {
			InitCommand = function(self)
				self:Load("GraphDisplay")
			end,
			BeginCommand = function(self)
				local ss = SCREENMAN:GetTopScreen():GetStageStats()
				self:Set(ss, ss:GetPlayerStageStats(pn))
				self:diffusealpha(0.7)
				self:GetChild("Line"):diffusealpha(0)
				self:zoom(0.8)
				self:xy(-22, 8)
			end,
			RecalculateGraphsMessageCommand = function(self, params)
				-- called by the end of a codemessagecommand somewhere else
				if not tso[params.judge] then return end
				local success = SCREENMAN:GetTopScreen():SetPlayerStageStatsFromReplayData(SCREENMAN:GetTopScreen():GetStageStats():GetPlayerStageStats(PLAYER_1), tso[params.judge])
				if not success then return end
				self:playcommand("Begin")
				MESSAGEMAN:Broadcast("SetComboGraph")
			end
		}
	}
	return t
end

local function ComboGraph(pn)
	local t =
		Def.ActorFrame {
		Def.ComboGraph {
			InitCommand = function(self)
				self:Load("ComboGraph" .. ToEnumShortString(pn))
			end,
			BeginCommand = function(self)
				local ss = SCREENMAN:GetTopScreen():GetStageStats()
				self:Set(ss, ss:GetPlayerStageStats(pn))
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

local pssP1 = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1)

local frameX = 20
local frameY = 140
local frameWidth = SCREEN_CENTER_X - 120

function scoreBoard(pn, position)
	local customWindow
	local judge = enabledCustomWindows and 0 or (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty())
	local judge2 = judge
	local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(pn)
	local score = SCOREMAN:GetMostRecentScore()
	if not score then 
		score = SCOREMAN:GetTempReplayScore()
	end
	local dvt = pss:GetOffsetVector()
	local totalTaps = pss:GetTotalTaps()

	local function scaleToJudge(scale)
		scale = notShit.round(scale, 2)
		local scales = {1.50, 1.33, 1.16, 1.00, 0.84, 0.66, 0.50, 0.33, 0.20}
		local out = 4
		for k,v in pairs(scales) do
			if v == scale then
				out = k
			end
		end
		return out
	end

	local t =
		Def.ActorFrame {
		BeginCommand = function(self)
			if position == 1 then
				self:x(SCREEN_WIDTH - (frameX * 2) - frameWidth)
			end
			if not enabledCustomWindows then
				if PREFSMAN:GetPreference("SortBySSRNormPercent") then
					judge = 4
					judge2 = judge
					-- you ever hack something so hard?
					aboutToForceWindowSettings = true
					MESSAGEMAN:Broadcast("ForceWindow", {judge=4})
					MESSAGEMAN:Broadcast("RecalculateGraphs", {judge=4})
				else
					judge = scaleToJudge(SCREENMAN:GetTopScreen():GetReplayJudge())
					judge2 = judge
				end
			end
		end,
		UpdateNetEvalStatsMessageCommand = function(self)
			local s = SCREENMAN:GetTopScreen():GetHighScore()
			if s then
				score = s
			end
			dvt = score:GetOffsetVector()
			MESSAGEMAN:Broadcast("ScoreChanged")
		end
	}
	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:xy(frameX - 5, capWideScale(frameY, frameY + 4)):zoomto(frameWidth + 10, 220):halign(0):valign(0):diffuse(
				color("#333333CC")
			)
		end
	}
	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY + 30):zoomto(frameWidth, 2):halign(0):diffuse(getMainColor("highlight")):diffusealpha(0.5)
		end
	}
	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY + 55):zoomto(frameWidth, 2):halign(0):diffuse(getMainColor("highlight")):diffusealpha(0.5)
		end
	}

	t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 5, frameY + 32):zoom(0.5):halign(0):valign(0):maxwidth(200)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local rate = SCREENMAN:GetTopScreen():GetReplayRate()
				if not rate then rate = getCurRateValue() end
				local meter = GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(rate, 1)
				self:settextf("%5.2f", meter)
				self:diffuse(byMSD(meter))
			end
		}
	t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameWidth + frameX, frameY + 32):zoom(0.5):halign(1):valign(0):maxwidth(200)
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
	t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameWidth + frameX, frameY + 7):zoom(0.5):halign(1):valign(0):maxwidth(200)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
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
				self:xy(frameX + 5, frameY + 9):zoomto(capWideScale(320,360)/2.2,20):halign(0):valign(0)
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
		LoadFont("Common Large") ..
		{
			Name = "NormalText",
			InitCommand = function(self)
				self:xy(frameX + capWideScale(220, 320), frameY + 9 + capWideScale(0, 30)):zoom(0.45):halign(0):valign(0):maxwidth(capWideScale(320, 360))
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:diffuse(getGradeColor(score:GetWifeGrade()))
				local ws = "Wife"
				local totalHolds =
					pss:GetRadarPossible():GetValue("RadarCategory_Holds") + pss:GetRadarPossible():GetValue("RadarCategory_Rolls")
				local holdsHit =
					score:GetRadarValues():GetValue("RadarCategory_Holds") + score:GetRadarValues():GetValue("RadarCategory_Rolls")
				local minesHit =
					pss:GetRadarPossible():GetValue("RadarCategory_Mines") - score:GetRadarValues():GetValue("RadarCategory_Mines")
				local wife2perc = 1
				local wife3perc1 = 1
				local wife3perc2 = 1
				local wife3perc3 = 1
				local wife3perc4 = 1
				local wife3perc5 = 1
				local wife3perc6 = 1
				if PREFSMAN:GetPreference("SortBySSRNormPercent") then
					ws = "Wife J4"
					wife2perc = getRescoredWifeJudge(dvt, 4, totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc1 = getRescoredWife3Judge(1, dvt, 4, totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc2 = getRescoredWife3Judge(2, dvt, 4, totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc3 = getRescoredWife3Judge(3, dvt, 4, totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc4 = getRescoredWife3Judge(4, dvt, 4, totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc5 = getRescoredWife3Judge(5, dvt, 4, totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc6 = getRescoredWife3Judge(6, dvt, 4, totalHolds - holdsHit, minesHit, totalTaps)
				else
					-- normalize to the currently selected judge, not the replay judge
					wife2perc = getRescoredWifeJudge(dvt, GetTimingDifficulty(), totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc1 = getRescoredWife3Judge(1, dvt, GetTimingDifficulty(), totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc2 = getRescoredWife3Judge(2, dvt, GetTimingDifficulty(), totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc3 = getRescoredWife3Judge(3, dvt, GetTimingDifficulty(), totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc4 = getRescoredWife3Judge(4, dvt, GetTimingDifficulty(), totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc5 = getRescoredWife3Judge(5, dvt, GetTimingDifficulty(), totalHolds - holdsHit, minesHit, totalTaps)
					wife3perc6 = getRescoredWife3Judge(6, dvt, GetTimingDifficulty(), totalHolds - holdsHit, minesHit, totalTaps)

				end
					self:settextf(
						"%05.2f%% (%s)\n%05.2f%% : %05.2f%% : %05.2f%%\n%05.2f%% : %05.2f%% : %05.2f%%", 
						notShit.floor(wife2perc * 100) / 100, ws..GetTimingDifficulty(),
						notShit.floor(wife3perc1 * 100) / 100, notShit.floor(wife3perc2 * 100) / 100, notShit.floor(wife3perc3 * 100) / 100, 
						notShit.floor(wife3perc4 * 100) / 100, notShit.floor(wife3perc5 * 100) / 100, notShit.floor(wife3perc6 * 100) / 100
					) -- uauagagugggahghghhhhhhh
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CodeMessageCommand = function(self, params)
				local totalHolds =
					pss:GetRadarPossible():GetValue("RadarCategory_Holds") + pss:GetRadarPossible():GetValue("RadarCategory_Rolls")
				local holdsHit =
					score:GetRadarValues():GetValue("RadarCategory_Holds") + score:GetRadarValues():GetValue("RadarCategory_Rolls")
				local minesHit =
					pss:GetRadarPossible():GetValue("RadarCategory_Mines") - score:GetRadarValues():GetValue("RadarCategory_Mines")
				if enabledCustomWindows then
					if params.Name == "PrevJudge" then
						judge = judge < 2 and #customWindows or judge - 1
						customWindow = timingWindowConfig:get_data()[customWindows[judge]]
						local perc = notShit.floor(getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps) * 100) / 100
						self:settextf(
							"%05.2f%% (%s)",
							perc,
							customWindow.name
						)
						MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
					elseif params.Name == "NextJudge" then
						judge = judge == #customWindows and 1 or judge + 1
						customWindow = timingWindowConfig:get_data()[customWindows[judge]]
						local perc = notShit.floor(getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps) * 100) / 100
						self:settextf(
							"%05.2f%% (%s)",
							perc,
							customWindow.name
						)
						MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
					end
				elseif params.Name == "PrevJudge" and judge > 1 then
					judge = judge - 1
					local wife2perc = getRescoredWifeJudge(dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc1 = getRescoredWife3Judge(1, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc2 = getRescoredWife3Judge(2, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc3 = getRescoredWife3Judge(3, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc4 = getRescoredWife3Judge(4, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc5 = getRescoredWife3Judge(5, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc6 = getRescoredWife3Judge(6, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					self:settextf(
						self:settextf(
							"%05.2f%% (%s)\n%05.2f%% : %05.2f%% : %05.2f%%\n%05.2f%% : %05.2f%% : %05.2f%%", 
							notShit.floor(wife2perc * 100) / 100, "Wife J" .. judge,
							notShit.floor(wife3perc1 * 100) / 100, notShit.floor(wife3perc2 * 100) / 100, notShit.floor(wife3perc3 * 100) / 100, 
							notShit.floor(wife3perc4 * 100) / 100, notShit.floor(wife3perc5 * 100) / 100, notShit.floor(wife3perc6 * 100) / 100
						)
					)
					MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
				elseif params.Name == "NextJudge" and judge < 9 then
					judge = judge + 1
					local wife2perc = getRescoredWifeJudge(dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc1 = getRescoredWife3Judge(1, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc2 = getRescoredWife3Judge(2, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc3 = getRescoredWife3Judge(3, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc4 = getRescoredWife3Judge(4, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc5 = getRescoredWife3Judge(5, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					local wife3perc6 = getRescoredWife3Judge(6, dvt, judge, totalHolds - holdsHit, minesHit, totalTaps)
					if judge == 9 then
						self:settextf(
							self:settextf(
								"%05.2f%% (%s)\n%05.2f%% : %05.2f%% : %05.2f%%\n%05.2f%% : %05.2f%% : %05.2f%%", 
								notShit.floor(wife2perc * 100) / 100, "justice",
								notShit.floor(wife3perc1 * 100) / 100, notShit.floor(wife3perc2 * 100) / 100, notShit.floor(wife3perc3 * 100) / 100, 
								notShit.floor(wife3perc4 * 100) / 100, notShit.floor(wife3perc5 * 100) / 100, notShit.floor(wife3perc6 * 100) / 100
							)
						)
					else
						self:settextf(
							"%05.2f%% (%s)\n%05.2f%% : %05.2f%% : %05.2f%%\n%05.2f%% : %05.2f%% : %05.2f%%", 
							notShit.floor(wife2perc * 100) / 100, "Wife J" .. judge, 
							notShit.floor(wife3perc1 * 100) / 100, notShit.floor(wife3perc2 * 100) / 100, notShit.floor(wife3perc3 * 100) / 100, 
							notShit.floor(wife3perc4 * 100) / 100, notShit.floor(wife3perc5 * 100) / 100, notShit.floor(wife3perc6 * 100) / 100
							
						)
					end
					MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
				end
				if params.Name == "ResetJudge" then
					judge = enabledCustomWindows and 0 or originaljudge
					self:playcommand("Set")
					MESSAGEMAN:Broadcast("RecalculateGraphs", {judge = judge})
				end
			end
		},
		LoadFont("Common Large") ..
		{
			Name = "LongerText",
			InitCommand = function(self)
				self:xy(frameX + 5, frameY + 9):zoom(0.45):halign(0):valign(0):maxwidth(capWideScale(320, 360))
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:diffuse(getGradeColor(score:GetWifeGrade()))
				self:settextf("%05.4f%% (%s)", notShit.floor(score:GetWifeScore() * 1000000) / 10000, "Wife")
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CodeMessageCommand = function(self, params)
				local totalHolds =
					pss:GetRadarPossible():GetValue("RadarCategory_Holds") + pss:GetRadarPossible():GetValue("RadarCategory_Rolls")
				local holdsHit =
					score:GetRadarValues():GetValue("RadarCategory_Holds") + score:GetRadarValues():GetValue("RadarCategory_Rolls")
				local minesHit =
					pss:GetRadarPossible():GetValue("RadarCategory_Mines") - score:GetRadarValues():GetValue("RadarCategory_Mines")
				if enabledCustomWindows then
					if params.Name == "PrevJudge" then
						judge2 = judge2 < 2 and #customWindows or judge2 - 1
						customWindow = timingWindowConfig:get_data()[customWindows[judge2]]
						local perc = notShit.floor(getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps) * 10000) / 10000
						self:settextf(
							"%05.4f%% (%s)",
							perc,
							customWindow.name
						)
					elseif params.Name == "NextJudge" then
						judge2 = judge2 == #customWindows and 1 or judge2 + 1
						customWindow = timingWindowConfig:get_data()[customWindows[judge2]]
						local perc = notShit.floor(getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps) * 10000) / 10000
						self:settextf(
							"%05.4f%% (%s)",
							perc,
							customWindow.name
						)
					end
				elseif params.Name == "PrevJudge" and judge2 > 1 then
					judge2 = judge2 - 1
					local perc = notShit.floor(getRescoredWifeJudge(dvt, judge, totalHolds - holdsHit, minesHit, totalTaps) * 10000) / 10000
					self:settextf(
						"%05.4f%% (%s)",
						perc,
						"Wife J" .. judge2
					)
				elseif params.Name == "NextJudge" and judge2 < 9 then
					judge2 = judge2 + 1
					local perc = notShit.floor(getRescoredWifeJudge(dvt, judge, totalHolds - holdsHit, minesHit, totalTaps) * 10000) / 10000
					if judge2 == 9 then
						self:settextf(
							"%05.4f%% (%s)",
							perc,
							"Wife Justice"
						)
					else
						self:settextf(
							"%05.4f%% (%s)",
							perc,
							"Wife J" .. judge2
						)
					end
				end
				if params.Name == "ResetJudge" then
					judge2 = enabledCustomWindows and 0 or originaljudge
					self:playcommand("Set")
				end
			end
		}
	}

	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 5, frameY + 63):zoom(0.40):halign(0):maxwidth(frameWidth / 0.4)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local mstring = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString("ModsLevel_Current")
				local ss = SCREENMAN:GetTopScreen():GetStageStats()
				if not ss:GetLivePlay() then
					mstring = SCREENMAN:GetTopScreen():GetReplayModifiers()
				end
				self:settext(getModifierTranslations(mstring))
			end
		}

	for k, v in ipairs(judges) do
		t[#t + 1] =
			Def.Quad {
			InitCommand = function(self)
				self:xy(frameX, frameY + 80 + ((k - 1) * 22)):zoomto(frameWidth, 18):halign(0):diffuse(byJudgment(v)):diffusealpha(
					0.5
				)
			end
		}
		t[#t + 1] =
			Def.Quad {
			InitCommand = function(self)
				self:xy(frameX, frameY + 80 + ((k - 1) * 22)):zoomto(0, 18):halign(0):diffuse(byJudgment(v)):diffusealpha(0.5)
			end,
			BeginCommand = function(self)
				self:glowshift():effectcolor1(color("1,1,1," .. tostring(pss:GetPercentageOfTaps(v) * 0.4))):effectcolor2(
					color("1,1,1,0")
				)
				if aboutToForceWindowSettings then return end
				self:sleep(0.5):decelerate(2):zoomx(frameWidth * pss:GetPercentageOfTaps(v))
			end,
			ForceWindowMessageCommand = function(self, params)
				if enabledCustomWindows then
					self:finishtweening():decelerate(2):zoomx(
						frameWidth * getRescoredCustomJudge(dvt, customWindow.judgeWindows, k) / totalTaps
					)
				else
					local rescoreJudges = getRescoredJudge(dvt, judge, k)
					self:finishtweening():decelerate(2):zoomx(frameWidth * rescoreJudges / totalTaps)
				end
			end,
			CodeMessageCommand = function(self, params)
				if params.Name == "PrevJudge" or params.Name == "NextJudge" then
					if enabledCustomWindows then
						self:finishtweening():decelerate(2):zoomx(
							frameWidth * getRescoredCustomJudge(dvt, customWindow.judgeWindows, k) / totalTaps
						)
					else
						local rescoreJudges = getRescoredJudge(dvt, judge, k)
						self:finishtweening():decelerate(2):zoomx(frameWidth * rescoreJudges / totalTaps)
					end
				end
				if params.Name == "ResetJudge" then
					self:finishtweening():decelerate(2):zoomx(frameWidth * pss:GetPercentageOfTaps(v))
				end
			end
		}
		t[#t + 1] =
			LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 10, frameY + 80 + ((k - 1) * 22)):zoom(0.25):halign(0)
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
					if enabledCustomWindows and (params.Name == "PrevJudge" or params.Name == "NextJudge") then
						self:settext(getCustomJudgeString(customWindow.judgeNames, k))
					end
					if params.Name == "ResetJudge" then
						self:playcommand("Set")
					end
				end
			}
		t[#t + 1] =
			LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(frameX + frameWidth - 40, frameY + 80 + ((k - 1) * 22)):zoom(0.25):halign(1)
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
					if enabledCustomWindows then
						self:settext(getRescoredCustomJudge(dvt, customWindow.judgeWindows, k))
					else
						self:settext(getRescoredJudge(dvt, judge, k))
					end
				end,
				CodeMessageCommand = function(self, params)
					if params.Name == "PrevJudge" or params.Name == "NextJudge" then
						if enabledCustomWindows then
							self:settext(getRescoredCustomJudge(dvt, customWindow.judgeWindows, k))
						else
							self:settext(getRescoredJudge(dvt, judge, k))
						end
					end
					if params.Name == "ResetJudge" then
						self:playcommand("Set")
					end
				end
			}
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX + frameWidth - 38, frameY + 80 + ((k - 1) * 22)):zoom(0.3):halign(0)
				end,
				BeginCommand = function(self)
					if aboutToForceWindowSettings then return end
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					self:settextf("(%03.2f%%)", pss:GetPercentageOfTaps(v) * 100)
				end,
				ForceWindowMessageCommand = function(self, params)
					local rescoredJudge
					if enabledCustomWindows then
						rescoredJudge = getRescoredCustomJudge(dvt, customWindow.judgeWindows, k)
					else
						rescoredJudge = getRescoredJudge(dvt, params.judge, k)
					end
					self:settextf("(%03.2f%%)", rescoredJudge / totalTaps * 100)
				end,
				CodeMessageCommand = function(self, params)
					if params.Name == "PrevJudge" or params.Name == "NextJudge" then
						local rescoredJudge
						if enabledCustomWindows then
							rescoredJudge = getRescoredCustomJudge(dvt, customWindow.judgeWindows, k)
						else
							rescoredJudge = getRescoredJudge(dvt, judge, k)
						end
						self:settextf("(%03.2f%%)", rescoredJudge / totalTaps * 100)
					end
					if params.Name == "ResetJudge" then
						self:playcommand("Set")
					end
				end
			}
	end

	if score:GetChordCohesion() == true then
		t[#t + 1] =
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 3, frameY + 210):zoom(0.25):halign(0)
					self:maxwidth(capWideScale(get43size(100), 160)/0.25)
				end,
				BeginCommand = function(self)
					self:queuecommand("Set")
				end,
				ScoreChangedMessageCommand = function(self)
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					self:settext(translated_info["CCOn"])
				end
			}
	end

	--[[
	The following section first adds the ratioText and the maRatio. Then the paRatio is added and positioned. The right
	values for maRatio and paRatio are then filled in. Finally ratioText and maRatio are aligned to paRatio.
	--]]
	local ratioText, maRatio, paRatio, marvelousTaps, perfectTaps, greatTaps
	t[#t + 1] =
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				ratioText = self
				self:settextf("%s:", translated_info["MAPARatio"])
				self:zoom(0.25):halign(1)
			end
		}
	t[#t + 1] =
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				maRatio = self
				self:zoom(0.25):halign(1):diffuse(byJudgment(judges[1]))
			end
		}
	t[#t + 1] =
	LoadFont("Common Large") ..
		{
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
					if enabledCustomWindows then
						marvelousTaps = getRescoredCustomJudge(dvt, customWindow.judgeWindows, 1)
						perfectTaps = getRescoredCustomJudge(dvt, customWindow.judgeWindows, 2)
						greatTaps = getRescoredCustomJudge(dvt, customWindow.judgeWindows, 3)
					else
						marvelousTaps = getRescoredJudge(dvt, judge, 1)
						perfectTaps = getRescoredJudge(dvt, judge, 2)
						greatTaps = getRescoredJudge(dvt, judge, 3)
					end
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
				if enabledCustomWindows then
					marvelousTaps = getRescoredCustomJudge(dvt, customWindow.judgeWindows, 1)
					perfectTaps = getRescoredCustomJudge(dvt, customWindow.judgeWindows, 2)
					greatTaps = getRescoredCustomJudge(dvt, customWindow.judgeWindows, 3)
				else
					marvelousTaps = getRescoredJudge(dvt, judge, 1)
					perfectTaps = getRescoredJudge(dvt, judge, 2)
					greatTaps = getRescoredJudge(dvt, judge, 3)
				end
				self:playcommand("Set")
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
	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:xy(frameX - 5, frameY + 230):zoomto(frameWidth / 2 - 10, 60):halign(0):valign(0):diffuse(color("#333333CC"))
		end
	}
	for i = 1, #fart do
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX, frameY + 230 + 10 * i):zoom(0.4):halign(0):settext(fart_translated[fart[i]])
				end
			}
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameWidth / 2, frameY + 230 + 10 * i):zoom(0.4):halign(1)
				end,
				BeginCommand = function(self)
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					self:settextf(
						"%03d/%03d",
						pss:GetRadarActual():GetValue("RadarCategory_" .. fart[i]),
						pss:GetRadarPossible():GetValue("RadarCategory_" .. fart[i])
					)
				end,
				ScoreChangedMessageCommand = function(self)
					self:queuecommand("Set")
				end
			}
	end

	-- stats stuff
	local tracks = pss:GetTrackVector()
	local devianceTable = pss:GetOffsetVector()
	local cbl = 0
	local cbr = 0
	local cbm = 0

	-- basic per-hand stats to be expanded on later
	local tst = ms.JudgeScalers
	local tso = tst[judge]
	if enabledCustomWindows then
		tso = 1
	end
	local ncol = GAMESTATE:GetCurrentSteps(PLAYER_1):GetNumColumns() - 1 -- cpp indexing -mina
	local middleCol = ncol/2
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

	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:xy(frameWidth + 25, frameY + 230):zoomto(frameWidth / 2 + 10, 60):halign(1):valign(0):diffuse(
				color("#333333CC")
			)
		end
	}
	local smallest, largest = wifeRange(devianceTable)
	local doot = {
		THEME:GetString("ScreenEvaluation", "Mean"),
		THEME:GetString("ScreenEvaluation", "StandardDev"),
		"Largest",
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
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX + capWideScale(get43size(130), 160), frameY + 230 + ySpacing * i):zoom(tzoom):halign(0):settext(doot[i])
				end
			}
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				Name=i,
				InitCommand = function(self)
					if i < 4 then
						self:xy(frameWidth + 20, frameY + 230 + ySpacing * i):zoom(tzoom):halign(1):settextf("%5.2fms", mcscoot[i])
					else
						self:xy(frameWidth + 20, frameY + 230 + ySpacing * i):zoom(tzoom):halign(1):settext(mcscoot[i])
					end
				end,
				CodeMessageCommand = function(self, params)
					local j = tonumber(self:GetName())
					if j > 3 and (params.Name == "PrevJudge" or params.Name == "NextJudge") then
						if j == 4 then
							local tso = tst[judge]
							if enabledCustomWindows then
								tso = 1
							end
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
						self:xy(frameWidth + 20, frameY + 230 + 10 * j):zoom(0.4):halign(1):settext(mcscoot[j])
					end
				end,
				ForceWindowMessageCommand = function(self)
					local j = tonumber(self:GetName())
					if j > 3 then
						if j == 4 then
							local tso = tst[judge]
							if enabledCustomWindows then
								tso = 1
							end
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
						self:xy(frameWidth + 20, frameY + 230 + 10 * j):zoom(0.4):halign(1):settext(mcscoot[j])
					end
				end
			}
	end

	return t
end

if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	t[#t + 1] = scoreBoard(PLAYER_1, 0)
	t[#t + 1] = StandardDecorationFromTable("GraphDisplay" .. ToEnumShortString(PLAYER_1), GraphDisplay(PLAYER_1))
	t[#t + 1] = StandardDecorationFromTable("ComboGraph" .. ToEnumShortString(PLAYER_1), ComboGraph(PLAYER_1))
end

t[#t + 1] = LoadActor("../offsetplot")

local score = SCOREMAN:GetMostRecentScore()
if not score then 
	score = SCOREMAN:GetTempReplayScore()
end
-- Discord thingies
local largeImageTooltip =
	GetPlayerOrMachineProfile(PLAYER_1):GetDisplayName() ..
	": " .. string.format("%5.2f", GetPlayerOrMachineProfile(PLAYER_1):GetPlayerRating())
local detail =
	GAMESTATE:GetCurrentSong():GetDisplayMainTitle() ..
	" " .. string.gsub(getCurRateDisplayString(), "Music", "") .. " [" .. GAMESTATE:GetCurrentSong():GetGroupName() .. "]"
if not STATSMAN:GetCurStageStats():GetLivePlay() then
	detail = "Replayed: "..detail
end
-- truncated to 128 characters(discord hard limit)
detail = #detail < 128 and detail or string.sub(detail, 1, 124) .. "..."
local state =
	"MSD: " ..
	string.format("%05.2f", GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(), 1)) ..
		" - " ..
			string.format("%05.2f%%", notShit.floor(pssP1:GetWifeScore() * 10000) / 100) ..
				" " .. THEME:GetString("Grade", ToEnumShortString(score:GetWifeGrade()))
GAMESTATE:UpdateDiscordPresence(largeImageTooltip, detail, state, 0)

return t
