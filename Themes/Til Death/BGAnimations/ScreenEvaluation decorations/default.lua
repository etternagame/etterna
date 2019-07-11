local t = Def.ActorFrame {}

local enabledCustomWindows = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomEvaluationWindowTimings

local customWindows = timingWindowConfig:get_data().customWindows

local scoreType = themeConfig:get_data().global.DefaultScoreType

if GAMESTATE:GetNumPlayersEnabled() == 1 and themeConfig:get_data().eval.ScoreBoardEnabled then
	t[#t + 1] = LoadActor("scoreboard")
end

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
			if getCurRateString() == "1x" then
				self:settext("")
			else
				self:settext(getCurRateString())
			end
		end
	}

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
	local judge = enabledCustomWindows and 0 or GetTimingDifficulty()
	local judge2 = judge
	local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(pn)
	local score = SCOREMAN:GetMostRecentScore()
	if not score then 
		score = SCOREMAN:GetTempReplayScore()
	end
	local dvt = pss:GetOffsetVector()
	local totalTaps = pss:GetTotalTaps()

	local t =
		Def.ActorFrame {
		BeginCommand = function(self)
			if position == 1 then
				self:x(SCREEN_WIDTH - (frameX * 2) - frameWidth)
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
				local meter = GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(), 1)
				self:settextf("%5.2f", meter)
				self:diffuse(byMSD(meter))
			end
		}
		t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 65, frameY + 46):zoom(0.15):halign(0):valign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local meter = GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(), 1)
				self:settext(":Mina Standardized Difficulty (MSD)")
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
				self:xy(frameX + 145, frameY + 35):zoom(0.15):halign(0):valign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local meter = GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(), 1)
				self:settext("Score Specific Rating (SSR):")
			end
		}

	t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameWidth + frameX, frameY + 7):zoom(0.25):halign(1):valign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
				local diff = getDifficulty(steps:GetDifficulty())
				local stype = ToEnumShortString(steps:GetStepsType()):gsub("%_"," ")
				self:settext(string.upper(stype.." "..diff))
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
				self:xy(frameX + 5, frameY + 9):zoom(0.45):halign(0):valign(0):maxwidth(capWideScale(320, 360))
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:diffuse(getGradeColor(score:GetWifeGrade()))
				self:settextf("%05.2f%% (%s)", notShit.floor(score:GetWifeScore() * 10000) / 100, "Wife")
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
						self:settextf(
							"%05.2f%% (%s)",
							getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps),
							customWindow.name
						)
					elseif params.Name == "NextJudge" then
						judge = judge == #customWindows and 1 or judge + 1
						customWindow = timingWindowConfig:get_data()[customWindows[judge]]
						self:settextf(
							"%05.2f%% (%s)",
							getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps),
							customWindow.name
						)
					end
				elseif params.Name == "PrevJudge" and judge > 1 then
					judge = judge - 1
					self:settextf(
						"%05.2f%% (%s)",
						getRescoredWifeJudge(dvt, judge, totalHolds - holdsHit, minesHit, totalTaps),
						"Wife J" .. judge
					)
				elseif params.Name == "NextJudge" and judge < 9 then
					judge = judge + 1
					if judge == 9 then
						self:settextf(
							"%05.2f%% (%s)",
							getRescoredWifeJudge(dvt, judge, (totalHolds - holdsHit), minesHit, totalTaps),
							"Wife Justice"
						)
					else
						self:settextf(
							"%05.2f%% (%s)",
							getRescoredWifeJudge(dvt, judge, (totalHolds - holdsHit), minesHit, totalTaps),
							"Wife J" .. judge
						)
					end
				end
				if params.Name == "ResetJudge" then
					judge = enabledCustomWindows and 0 or GetTimingDifficulty()
					self:playcommand("Set")
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
						self:settextf(
							"%05.4f%% (%s)",
							getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps),
							customWindow.name
						)
					elseif params.Name == "NextJudge" then
						judge2 = judge2 == #customWindows and 1 or judge2 + 1
						customWindow = timingWindowConfig:get_data()[customWindows[judge2]]
						self:settextf(
							"%05.4f%% (%s)",
							getRescoredCustomPercentage(dvt, customWindow, totalHolds, holdsHit, minesHit, totalTaps),
							customWindow.name
						)
					end
				elseif params.Name == "PrevJudge" and judge2 > 1 then
					judge2 = judge2 - 1
					self:settextf(
						"%05.4f%% (%s)",
						getRescoredWifeJudge(dvt, judge2, totalHolds - holdsHit, minesHit, totalTaps),
						"Wife J" .. judge2
					)
				elseif params.Name == "NextJudge" and judge2 < 9 then
					judge2 = judge2 + 1
					if judge2 == 9 then
						self:settextf(
							"%05.4f%% (%s)",
							getRescoredWifeJudge(dvt, judge2, (totalHolds - holdsHit), minesHit, totalTaps),
							"Wife Justice"
						)
					else
						self:settextf(
							"%05.4f%% (%s)",
							getRescoredWifeJudge(dvt, judge2, (totalHolds - holdsHit), minesHit, totalTaps),
							"Wife J" .. judge2
						)
					end
				end
				if params.Name == "ResetJudge" then
					judge2 = enabledCustomWindows and 0 or GetTimingDifficulty()
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
				self:settext(GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString("ModsLevel_Current"))
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
				):sleep(0.5):decelerate(2):zoomx(frameWidth * pss:GetPercentageOfTaps(v))
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
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					self:settext(getJudgeStrings(v))
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
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					self:settext(score:GetTapNoteScore(v))
				end,
				ScoreChangedMessageCommand = function(self)
					self:queuecommand("Set")
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
					self:queuecommand("Set")
				end,
				SetCommand = function(self)
					self:settextf("(%03.2f%%)", pss:GetPercentageOfTaps(v) * 100)
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

	t[#t + 1] = -- FA Ratio
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY * 2.49):zoom(0.25):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self) -- Replace with FA, EA, and CB Ratio
					self:diffuse(color(colorConfig:get_data().judgment["TapNoteScore_W1"]))
					self:settext("MA Ratio:")
			end
		}
	
	t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 55 , frameY * 2.49):zoom(0.25):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self) -- Replace with FA, EA, and CB Ratio
				local ratio = score:GetTapNoteScore("TapNoteScore_W1")/score:GetTapNoteScore("TapNoteScore_W2")
				self:settext(string.sub(ratio,0,5)..":1")
				if ratio == math.huge or ratio == "nan" then
					self:diffuse(color("#66CCFF"))
				elseif ratio > 10 then
					self:diffuse(color("#99FF99"))
				elseif ratio < 3 then
					self:diffuse(color("#FF0000"))
				elseif ratio < 4 and ratio > 3 then
					self:diffuse(color("#FFFF00"))
				end
			end
		}
	t[#t + 1] = -- EA Ratio
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 105, frameY * 2.49):zoom(0.25):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self) -- Replace with FA, EA, and CB Ratio
				self:diffuse(color(colorConfig:get_data().judgment["TapNoteScore_W2"]))
				self:settext("PA Ratio:")
			end
		}
	t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 160, frameY * 2.49):zoom(0.25):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self) -- Replace with FA, EA, and CB Ratio
				local ratio = score:GetTapNoteScore("TapNoteScore_W2")/score:GetTapNoteScore("TapNoteScore_W3")
				self:settext(string.sub(ratio,0,5)..":1")
				if ratio == math.huge or score:GetTapNoteScore("TapNoteScore_W3") == 0 then
					self:diffuse(color("#66CCFF"))
				elseif ratio > 10 then
					self:diffuse(color("#99FF99"))
				elseif ratio < 4 then
					self:diffuse(color("#FF0000"))
				elseif ratio < 5 and ratio > 4 then
					self:diffuse(color("#FFFF00"))
				end
			end
		}
	t[#t + 1] = -- CB Ratio
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 205, frameY * 2.49):zoom(0.25):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self) -- Replace with FA, EA, and CB Ratio
				self:diffuse(color(colorConfig:get_data().judgment["TapNoteScore_Miss"]))
				self:settext("CB Ratio:")
			end
		}
	t[#t + 1] =
		LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 260, frameY * 2.49):zoom(0.25):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			ScoreChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				local ratio = (score:GetTapNoteScore("TapNoteScore_W1")+score:GetTapNoteScore("TapNoteScore_W2")+score:GetTapNoteScore("TapNoteScore_W3"))/(score:GetTapNoteScore("TapNoteScore_W4")+score:GetTapNoteScore("TapNoteScore_W5")+score:GetTapNoteScore("TapNoteScore_Miss"))
				self:settext(string.sub(ratio,0,5)..":1")
				if ratio == math.huge or (score:GetTapNoteScore("TapNoteScore_W4")+score:GetTapNoteScore("TapNoteScore_W5")+score:GetTapNoteScore("TapNoteScore_Miss")) == 0 then
					self:diffuse(color("#66CCFF"))
				elseif ratio > 200 then
					self:diffuse(color("#99FF99"))
				elseif ratio < 75 then
					self:diffuse(color("#FF0000"))
				elseif ratio < 100 and ratio > 75 then
					self:diffuse(color("#FFFF00"))
				end
			end
		}
	local fart = {"Holds", "Mines", "Rolls", "Lifts", "Fakes"}
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
					self:xy(frameX, frameY + 230 + 10 * i):zoom(0.4):halign(0):settext(fart[i])
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

	-- basic per-hand stats to be expanded on later
	local tst = ms.JudgeScalers
	local tso = tst[judge]
	if enabledCustomWindows then
		tso = 1
	end
	local ncol = GAMESTATE:GetCurrentSteps(PLAYER_1):GetNumColumns() - 1 -- cpp indexing -mina
	for i = 1, #devianceTable do
		if tracks[i] then	-- we dont load track data when reconstructing eval screen apparently so we have to nil check -mina
			if math.abs(devianceTable[i]) > tso * 90 then
				if tracks[i] <= math.floor(ncol/2) then	-- just assume middle col in 7k is right hand thumb for now -mina
					cbl = cbl + 1
				else
					cbr = cbr + 1
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
	local doot = {"Mean", "Mean(Abs)", "Sd", "Left cbs", "Right cbs"}
	local mcscoot = {
		wifeMean(devianceTable),
		wifeAbsMean(devianceTable),
		wifeSd(devianceTable),
		cbl,
		cbr
	}

	for i = 1, #doot do
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX + capWideScale(get43size(130), 160), frameY + 230 + 10 * i):zoom(0.4):halign(0):settext(doot[i])
				end
			}
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				Name=i,
				InitCommand = function(self)
					if i < 4 then
						self:xy(frameWidth + 20, frameY + 230 + 10 * i):zoom(0.4):halign(1):settextf("%5.2fms", mcscoot[i])
					else
						self:xy(frameWidth + 20, frameY + 230 + 10 * i):zoom(0.4):halign(1):settext(mcscoot[i])
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
