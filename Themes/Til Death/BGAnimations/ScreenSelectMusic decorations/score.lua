-- refactored a bit but still needs work -mina
local collapsed = false
local rtTable
local rates
local rateIndex = 1
local scoreIndex = 1
local score
local pn = GAMESTATE:GetEnabledPlayers()[1]
local nestedTab = 1
local nestedTabs = {
	THEME:GetString("TabScore", "NestedLocal"),
	THEME:GetString("TabScore", "NestedOnline")
}
local hasReplayData

local frameX = 10
local frameY = 40
local frameWidth = SCREEN_WIDTH * 0.56
local frameHeight = 368
local fontScale = 0.4
local offsetX = 10
local offsetY = 20
local netScoresPerPage = 8
local netScoresCurrentPage = 1
local nestedTabButtonWidth = 153
local nestedTabButtonHeight = 20
local netPageButtonWidth = 50
local netPageButtonHeight = 50
local headeroffY = 26 / 1.5

local selectedrateonly

local judges = {
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss",
	"HoldNoteScore_Held",
	"HoldNoteScore_LetGo"
}

local translated_info = {
	MaxCombo = THEME:GetString("TabScore", "MaxCombo"),
	MissCount = THEME:GetString("TabScore", "MissCount"),
	DateAchieved = THEME:GetString("TabScore", "DateAchieved"),
	Mods = THEME:GetString("TabScore", "Mods"),
	Rate = THEME:GetString("TabScore", "Rate"), -- used in conjunction with Showing
	Showing = THEME:GetString("TabScore", "Showing"), -- to produce a scuffed thing
	ChordCohesion = THEME:GetString("TabScore", "ChordCohesion"),
	Judge = THEME:GetString("TabScore", "ScoreJudge"),
	NoScores = THEME:GetString("TabScore", "NoScores"),
	Yes = THEME:GetString("OptionNames", "Yes"),
	No = THEME:GetString("OptionNames", "No"),
	ShowOffset = THEME:GetString("TabScore", "ShowOffsetPlot"),
	NoReplayData = THEME:GetString("TabScore", "NoReplayData"),
	ShowReplay = THEME:GetString("TabScore", "ShowReplay"),
	ShowEval = THEME:GetString("TabScore", "ShowEval"),
	UploadReplay = THEME:GetString("TabScore", "Upload Replay"),
	UploadingReplay = THEME:GetString("TabScore", "UploadingReplay")
}

local defaultRateText = ""
if themeConfig:get_data().global.RateSort then
	defaultRateText = "1.0x"
else
	defaultRateText = "All"
end

-- should maybe make some of these generic
local function highlight(self)
	if self:IsVisible() then
		self:queuecommand("Highlight")
	end
end

-- note: will use the local isover functionality
local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end
local moped
-- Only works if ... it should work
-- You know, if we can see the place where the scores should be.
local function updateLeaderBoardForCurrentChart()
	local top = SCREENMAN:GetTopScreen()
	if top:GetName() == "ScreenSelectMusic" or top:GetName() == "ScreenNetSelectMusic" then
		if top:GetMusicWheel():IsSettled() and ((getTabIndex() == 2 and nestedTab == 2) or collapsed) then
			local steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
			if steps then
				local leaderboardAttempt = DLMAN:GetChartLeaderboard(steps:GetChartKey())
				if #leaderboardAttempt > 0 then
					moped:playcommand("SetFromLeaderboard", leaderboardAttempt)
				else
					DLMAN:RequestChartLeaderBoardFromOnline(
						steps:GetChartKey(),
						function(leaderboard)
							moped:queuecommand("SetFromLeaderboard", leaderboard)
						end
					)
				end
			else
				moped:playcommand("SetFromLeaderboard", {})
			end
		end
	end
end

local ret =
	Def.ActorFrame {
	BeginCommand = function(self)
		moped = self:GetChild("ScoreDisplay")
		self:queuecommand("Set"):visible(false)
		self:GetChild("LocalScores"):xy(frameX, frameY):visible(false)
		moped:xy(frameX, frameY):visible(false)

		if FILTERMAN:oopsimlazylol() then -- set saved position and auto collapse
			nestedTab = 2
			self:GetChild("LocalScores"):visible(false)
			moped:xy(FILTERMAN:grabposx("Doot"), FILTERMAN:grabposy("Doot")):visible(true)
			self:playcommand("Collapse")
		end
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0) -- visible(false)
		self:GetChild("LocalScores"):visible(false)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
		if getTabIndex() == 2 and nestedTab == 1 then
			self:GetChild("LocalScores"):visible(true)
		else
			self:GetChild("LocalScores"):visible(false)
		end
	end,
	SetCommand = function(self)
		self:finishtweening(1)
		if getTabIndex() == 2 then -- switching to this tab
			if nestedTab == 2 then
				self:GetParent():GetChild("StepsDisplay"):visible(false)
			end
			if collapsed then -- expand if collaped
				self:queuecommand("Expand")
			else
				self:queuecommand("On")
				self:visible(true)
			end
		elseif collapsed and getTabIndex() == 0 then -- display on general tab if collapsed
			self:queuecommand("On")
			self:visible(true) -- not sure about whether this works or is needed
		elseif collapsed and getTabIndex() ~= 0 then -- but not others
			self:queuecommand("Off")
		elseif not collapsed then -- if not collapsed, never display outside of this tab
			self:queuecommand("Off")
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
		updateLeaderBoardForCurrentChart()
	end,
	ChangeStepsMessageCommand = function(self)
		self:queuecommand("Set")
		updateLeaderBoardForCurrentChart()
	end,
	CollapseCommand = function(self)
		collapsed = true
		resetTabIndex()
		MESSAGEMAN:Broadcast("TabChanged")
	end,
	ExpandCommand = function(self)
		collapsed = false
		if getTabIndex() ~= 2 then
			setTabIndex(2)
		end
		self:GetChild("ScoreDisplay"):xy(frameX, frameY)
		MESSAGEMAN:Broadcast("TabChanged")
	end,
	DelayedChartUpdateMessageCommand = function(self)
		local leaderboardEnabled =
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).leaderboardEnabled and DLMAN:IsLoggedIn()
		if GAMESTATE:GetCurrentSteps(PLAYER_1) then
			local chartkey = GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey()
			if leaderboardEnabled then
			DLMAN:RequestChartLeaderBoardFromOnline(
				chartkey,
				function(leaderboard)
					moped:playcommand("SetFromLeaderboard", leaderboard)
				end
			)	-- this is also intentionally super bad so we actually do something about it -mina
			elseif (SCREENMAN:GetTopScreen():GetName() == "ScreenSelectMusic" or SCREENMAN:GetTopScreen():GetName() == "ScreenNetSelectMusic") and ((getTabIndex() == 2 and nestedTab == 2) or collapsed) then
				DLMAN:RequestChartLeaderBoardFromOnline(
				chartkey,
				function(leaderboard)
					moped:playcommand("SetFromLeaderboard", leaderboard)
				end
			)
			end
		end
	end,
	NestedTabChangedMessageCommand = function(self)
		self:queuecommand("Set")
		updateLeaderBoardForCurrentChart()
	end,
	CurrentStepsP1ChangedMessageCommand = function(self)
		self:queuecommand("Set")
		updateLeaderBoardForCurrentChart()
	end,
	CodeMessageCommand = function(self, params) -- this is intentionally bad to remind me to fix other things that are bad -mina
		if ((getTabIndex() == 2 and nestedTab == 2) and not collapsed) and DLMAN:GetCurrentRateFilter() then
			local rate = getCurRateValue()
			if params.Name == "PrevScore" and rate < 2.95 then
				GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate + 0.1)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate + 0.1)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate + 0.1)
				MESSAGEMAN:Broadcast("CurrentRateChanged")
			elseif params.Name == "NextScore" and rate > 0.75 then
				GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate - 0.1)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate - 0.1)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate - 0.1)
				MESSAGEMAN:Broadcast("CurrentRateChanged")
			end
			if params.Name == "PrevRate" and rate < 3 then
				GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate + 0.05)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate + 0.05)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate + 0.05)
				MESSAGEMAN:Broadcast("CurrentRateChanged")
			elseif params.Name == "NextRate" and rate > 0.7 then
				GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(rate - 0.05)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(rate - 0.05)
				GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(rate - 0.05)
				MESSAGEMAN:Broadcast("CurrentRateChanged")
			end
		end
	end,
	CurrentRateChangedMessageCommand = function(self)
		if ((getTabIndex() == 2 and nestedTab == 2) or collapsed) and DLMAN:GetCurrentRateFilter() then
			moped:queuecommand("GetFilteredLeaderboard")
		end
	end
}

local cheese
-- eats only inputs that would scroll to a new score
local function input(event)
	if isOver(cheese:GetChild("FrameDisplay")) then
		if event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" then
			moving = true
			if nestedTab == 1 and rtTable and rtTable[rates[rateIndex]] ~= nil then
				cheese:queuecommand("PrevScore")
				return true
			end
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" then
			if nestedTab == 1 and rtTable ~= nil and rtTable[rates[rateIndex]] ~= nil then
				cheese:queuecommand("NextScore")
				return true
			end
		elseif moving == true then
			moving = false
		end
	end
	return false
end

local t =
	Def.ActorFrame {
	Name = "LocalScores",
	InitCommand = function(self)
		rtTable = nil
		self:SetUpdateFunction(highlight)
		self:SetUpdateFunctionInterval(0.025)
		cheese = self
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	OnCommand = function(self)
		if nestedTab == 1 and self:IsVisible() then
			if GAMESTATE:GetCurrentSong() ~= nil then
				rtTable = getRateTable()
				if rtTable ~= nil then
					rates, rateIndex = getUsedRates(rtTable)
					scoreIndex = 1
					self:queuecommand("Display")
				else
					self:queuecommand("Init")
				end
			else
				self:queuecommand("Init")
			end
		end
	end,
	NestedTabChangedMessageCommand = function(self)
		self:visible(nestedTab == 1)
		self:queuecommand("Set")
	end,
	CodeMessageCommand = function(self, params)
		if nestedTab == 1 and rtTable ~= nil and rtTable[rates[rateIndex]] ~= nil then
			if params.Name == "NextRate" then
				self:queuecommand("NextRate")
			elseif params.Name == "PrevRate" then
				self:queuecommand("PrevRate")
			elseif params.Name == "NextScore" then
				self:queuecommand("NextScore")
			elseif params.Name == "PrevScore" then
				self:queuecommand("PrevScore")
			end
		end
	end,
	NextRateCommand = function(self)
		rateIndex = ((rateIndex) % (#rates)) + 1
		scoreIndex = 1
		self:queuecommand("Display")
	end,
	PrevRateCommand = function(self)
		rateIndex = ((rateIndex - 2) % (#rates)) + 1
		scoreIndex = 1
		self:queuecommand("Display")
	end,
	NextScoreCommand = function(self)
		scoreIndex = ((scoreIndex) % (#rtTable[rates[rateIndex]])) + 1
		self:queuecommand("Display")
	end,
	PrevScoreCommand = function(self)
		scoreIndex = ((scoreIndex - 2) % (#rtTable[rates[rateIndex]])) + 1
		self:queuecommand("Display")
	end,
	DisplayCommand = function(self)
		score = rtTable[rates[rateIndex]][scoreIndex]
		if getTabIndex() == 2 then
			hasReplayData = score:HasReplayData()
		else
			hasReplayData = false
		end
		setScoreForPlot(score)
	end,
	Def.Quad {
		Name = "FrameDisplay",
		InitCommand = function(self)
			self:zoomto(frameWidth, frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
		end,
		CollapseCommand = function(self)
			self:visible(false)
		end,
		ExpandCommand = function(self)
			self:visible(true)
		end
	}
}

-- header bar
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:zoomto(frameWidth, offsetY):halign(0):valign(0):diffuse(getMainColor("frames")):diffusealpha(0.5)
	end
}

local l =
	Def.ActorFrame {
	-- stuff inside the frame.. so we can move it all at once
	InitCommand = function(self)
		self:xy(offsetX, offsetY + headeroffY)
	end,
	LoadFont("Common Large") ..
		{
			Name = "Grades",
			InitCommand = function(self)
				self:y(20):zoom(0.6):halign(0):maxwidth(50 / 0.6):settext("")
			end,
			DisplayCommand = function(self)
				self:settext(THEME:GetString("Grade", ToEnumShortString(score:GetWifeGrade())))
				self:diffuse(getGradeColor(score:GetWifeGrade()))
			end
		},
	-- Wife display
	LoadFont("Common Normal") ..
		{
			Name = "Wife",
			InitCommand = function(self)
				self:xy(55, 15):zoom(0.6):halign(0):settext("")
			end,
			DisplayCommand = function(self)
				if score:GetWifeScore() == 0 then
					self:settextf("NA")
				else
					self:settextf("%05.2f%%", notShit.floor(score:GetWifeScore() * 10000) / 100):diffuse(byGrade(score:GetWifeGrade()))
				end
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Score",
			InitCommand = function(self)
				self:xy(55, 30):zoom(0.6):halign(0):settext("")
			end,
			DisplayCommand = function(self)
				if score:GetWifeScore() == 0 then
					self:settext("")
				else
					local overall = score:GetSkillsetSSR("Overall")
					self:settextf("%.2f", overall):diffuse(byMSD(overall))
				end
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Score",
			InitCommand = function(self)
				self:xy(55, 43):zoom(0.5):halign(0):settext("")
			end,
			DisplayCommand = function(self)
				if score:GetWifeScore() == 0 then
					self:settext("")
				else
					local ss = GAMESTATE:GetCurrentSteps(PLAYER_1):GetRelevantSkillsetsByMSDRank(getCurRateValue(), 1)
					if ss ~= "" then
						self:settext(THEME:GetString("Skillsets", ss))
					else
						self:settext("")
					end
				end
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "ClearType",
			InitCommand = function(self)
				self:y(43):zoom(0.5):halign(0):settext(""):diffuse(color(colorConfig:get_data().clearType["NoPlay"]))
			end,
			DisplayCommand = function(self)
				self:settext(getClearTypeFromScore(pn, score, 0))
				self:diffuse(getClearTypeFromScore(pn, score, 2))
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Combo",
			InitCommand = function(self)
				self:y(58):zoom(0.4):halign(0):settextf("%s:", translated_info["MaxCombo"])
			end,
			DisplayCommand = function(self)
				self:settextf("%s: %d", translated_info["MaxCombo"], score:GetMaxCombo())
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "MissCount",
			InitCommand = function(self)
				self:y(73):zoom(0.4):halign(0):settextf("%s:", translated_info["MissCount"])
			end,
			DisplayCommand = function(self)
				local missCount = getScoreMissCount(score)
				if missCount ~= nil then
					self:settextf("%s: %s", translated_info["MissCount"], missCount)
				else
					self:settextf("%s: -", translated_info["MissCount"])
				end
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Date",
			InitCommand = function(self)
				self:y(88):zoom(0.4):halign(0):settextf("%s:", translated_info["DateAchieved"])
			end,
			DisplayCommand = function(self)
				self:settextf("%s: %s", translated_info["DateAchieved"], getScoreDate(score))
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Mods",
			InitCommand = function(self)
				self:y(103):zoom(0.4):halign(0):settextf("%s:", translated_info["Mods"])
			end,
			DisplayCommand = function(self)
				self:settextf("%s: %s", translated_info["Mods"], getModifierTranslations(score:GetModifiers()))
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameWidth - offsetX - frameX, frameHeight - headeroffY - 10 - offsetY):zoom(0.4):halign(1)
				self:settext(translated_info["NoScores"])
			end,
			DisplayCommand = function(self)
				self:settextf("%s %s - %s %d/%d", translated_info["Rate"], rates[rateIndex], translated_info["Showing"], scoreIndex, #rtTable[rates[rateIndex]])
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "ChordCohesion",
			InitCommand = function(self)
				self:y(frameHeight - headeroffY - 10 - offsetY):zoom(0.4):halign(0):settextf("%s:", translated_info["ChordCohesion"])
			end,
			DisplayCommand = function(self)
				if score:GetChordCohesion() then
					self:settextf("%s: %s", translated_info["ChordCohesion"], translated_info["Yes"])
				else
					self:settextf("%s: %s", translated_info["ChordCohesion"], translated_info["No"])
				end
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "Judge",
			InitCommand = function(self)
				self:xy((frameWidth - offsetX - frameX) / 2, frameHeight - headeroffY - 10 - offsetY):zoom(0.4):settext("")
			end,
			DisplayCommand = function(self)
				local j = table.find(ms.JudgeScalers, notShit.round(score:GetJudgeScale(), 2))
				if not j then
					j = 4
				end
				self:settextf("%s %i", translated_info["Judge"], j)
			end
		}
}

local function makeText(index)
	return LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameWidth - frameX, offsetY + 105 + (index * 15)):zoom(fontScale + 0.05):halign(1):settext("")
			end,
			DisplayCommand = function(self)
				local count = 0
				if rtTable[rates[index]] ~= nil then
					count = #rtTable[rates[index]]
				end
				if index <= #rates then
					self:settextf("%s (%d)", rates[index], count)
					if index == rateIndex then
						self:diffuse(color("#FFFFFF"))
					else
						self:diffuse(getMainColor("positive"))
					end
				else
					self:settext("")
				end
			end,
			HighlightCommand = function(self)
				highlightIfOver(self)
			end,
			MouseLeftClickMessageCommand = function(self)
				if nestedTab == 1 and isOver(self) then
					rateIndex = index
					scoreIndex = 1
					self:GetParent():queuecommand("Display")
				end
			end
		}
end

for i = 1, 10 do
	t[#t + 1] = makeText(i)
end

local function makeJudge(index, judge)
	local t =
		Def.ActorFrame {
		InitCommand = function(self)
			self:y(125 + ((index - 1) * 18))
		end
	}

	--labels
	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:zoom(0.5):halign(0)
			end,
			BeginCommand = function(self)
				self:settext(getJudgeStrings(judge))
				self:diffuse(byJudgment(judge))
			end
		}

	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:x(120):zoom(0.5):halign(1):settext("0")
			end,
			DisplayCommand = function(self)
				if judge ~= "HoldNoteScore_Held" and judge ~= "HoldNoteScore_LetGo" then
					self:settext(getScoreTapNoteScore(score, judge))
				else
					self:settext(getScoreHoldNoteScore(score, judge))
				end
			end
		}

	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:x(122):zoom(0.3):halign(0):settext("")
			end,
			DisplayCommand = function(self)
				if judge ~= "HoldNoteScore_Held" and judge ~= "HoldNoteScore_LetGo" then
					local taps = math.max(1, getMaxNotes(pn))
					local count = getScoreTapNoteScore(score, judge)
					self:settextf("(%03.2f%%)", (count / taps) * 100)
				else
					local holds = math.max(1, getMaxHolds(pn))
					local count = getScoreHoldNoteScore(score, judge)
					self:settextf("(%03.2f%%)", (count / holds) * 100)
				end
			end
		}

	return t
end

for i = 1, #judges do
	l[#l + 1] = makeJudge(i, judges[i])
end

l[#l + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "Score",
		InitCommand = function(self)
			self:y(frameHeight - headeroffY - 30 - offsetY):zoom(0.5):halign(0):settext("")
		end,
		DisplayCommand = function(self)
			if hasReplayData then
				self:settext(translated_info["ShowOffset"])
			else
				self:settext(translated_info["NoReplayData"])
			end
		end,
		HighlightCommand = function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand = function(self)
			if nestedTab == 1 then
				if getTabIndex() == 2 and getScoreForPlot() and hasReplayData and isOver(self) then
					SCREENMAN:AddNewScreenToTop("ScreenScoreTabOffsetPlot")
				end
			end
		end
	}
l[#l + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "ReplayViewer",
		InitCommand = function(self)
			self:xy((frameWidth - offsetX - frameX) / 2, frameHeight - headeroffY - 30 - offsetY):zoom(0.5):settext("")
		end,
		BeginCommand = function(self)
			if SCREENMAN:GetTopScreen():GetName() == "ScreenNetSelectMusic" then
				self:visible(false)
			end
		end,
		DisplayCommand = function(self)
			if hasReplayData then
				self:settext(translated_info["ShowReplay"])
			else
				self:settext(translated_info["NoReplayData"])
			end
		end,
		HighlightCommand = function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand = function(self)
			if nestedTab == 1 then
				if getTabIndex() == 2 and getScoreForPlot() and hasReplayData and isOver(self) then
					SCREENMAN:GetTopScreen():PlayReplay(score)
				end
			end
		end
	}
l[#l + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "EvalViewer",
		InitCommand = function(self)
			self:xy(frameWidth - offsetX - frameX, frameHeight - headeroffY - 30 - offsetY):zoom(0.5):halign(1):settext("")
		end,
		BeginCommand = function(self)
			if SCREENMAN:GetTopScreen():GetName() == "ScreenNetSelectMusic" then
				self:visible(false)
			end
		end,
		DisplayCommand = function(self)
			if hasReplayData then
				self:settext(translated_info["ShowEval"])
			else
				self:settext("")
			end
		end,
		HighlightCommand = function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand = function(self)
			if nestedTab == 1 then
				if getTabIndex() == 2 and getScoreForPlot() and hasReplayData and isOver(self) then
					SCREENMAN:GetTopScreen():ShowEvalScreenForScore(score)
				end
			end
		end
	}

l[#l + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "TheDootButton",
		InitCommand = function(self)
			self:xy(frameWidth - offsetX - frameX, frameHeight - headeroffY - 70 - offsetY):zoom(0.5):halign(1):settext("")
		end,
		DisplayCommand = function(self)
			if hasReplayData then
				self:settext(translated_info["UploadReplay"])
			else
				self:settext("")
			end
		end,
		HighlightCommand = function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand = function(self)
			if nestedTab == 1 then
				if getTabIndex() == 2 and isOver(self) then
					DLMAN:SendReplayDataForOldScore(score:GetScoreKey())
					ms.ok(translated_info["UploadingReplay"]) --should have better feedback -mina
				end
			end
		end
	}
t[#t + 1] = l

t[#t + 1] =
	Def.Quad {
	Name = "ScrollBar",
	InitCommand = function(self)
		self:x(frameWidth):zoomto(4, 0):halign(1):valign(1):diffuse(getMainColor("highlight")):diffusealpha(0.75)
	end,
	DisplayCommand = function(self)
		self:finishtweening()
		self:smooth(0.2)
		self:zoomy(((frameHeight) / #rtTable[rates[rateIndex]]))
		self:y((((frameHeight) / #rtTable[rates[rateIndex]]) * scoreIndex))
	end
}

ret[#ret + 1] = t

function nestedTabButton(i)
	return Def.ActorFrame {
		InitCommand = function(self)
			self:xy(frameX + offsetX + (i - 1) * (nestedTabButtonWidth - capWideScale(100, 80)), frameY + offsetY - 2)
			self:SetUpdateFunction(highlight)
			self:SetUpdateFunctionInterval(0.025)
		end,
		CollapseCommand = function(self)
			self:visible(false)
		end,
		ExpandCommand = function(self)
			self:visible(true)
		end,
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:diffuse(getMainColor("positive")):maxwidth(nestedTabButtonWidth - 80):maxheight(40):zoom(0.65):settext(
						nestedTabs[i]
					):halign(0):valign(1)
				end,
				HighlightCommand = function(self)
					if isOver(self) and nestedTab ~= i then
						self:diffusealpha(0.75)
					elseif nestedTab == i then
						self:diffusealpha(1)
					else
						self:diffusealpha(0.6)
					end
				end,
				MouseLeftClickMessageCommand = function(self)
					if isOver(self) then
						nestedTab = i
						MESSAGEMAN:Broadcast("NestedTabChanged")
						if nestedTab == 1 then
							self:GetParent():GetParent():GetChild("ScoreDisplay"):visible(false)
							self:GetParent():GetParent():GetParent():GetChild("StepsDisplay"):visible(true)
						else
							self:GetParent():GetParent():GetChild("ScoreDisplay"):visible(true)
							self:GetParent():GetParent():GetParent():GetChild("StepsDisplay"):visible(false)
						end
					end
				end
			}
	}
end

-- online score display
ret[#ret + 1] = LoadActor("../superscoreboard")

for i = 1, #nestedTabs do
	ret[#ret + 1] = nestedTabButton(i)
end

return ret
