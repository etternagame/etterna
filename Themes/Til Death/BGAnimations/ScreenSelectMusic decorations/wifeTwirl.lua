local profile = PROFILEMAN:GetProfile(PLAYER_1)
local frameX = 10
local frameY = 250 + capWideScale(get43size(120), 90)
local frameWidth = capWideScale(get43size(455), 455)
local scoreType = themeConfig:get_data().global.DefaultScoreType
local score
local song
local steps
local alreadybroadcasted
local noteField = false
local heyiwasusingthat = false
local mcbootlarder
local prevX = capWideScale(get43size(98), 98)
local idkwhatimdoing = capWideScale(prevX+8, prevX/2+4)
local usingreverse = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
local prevY = 55
local prevrevY = 208

local update = false
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == 0 then
			if heyiwasusingthat and GAMESTATE:GetCurrentSong() and noteField then	-- these can prolly be wrapped better too -mina
				mcbootlarder:visible(true)
				MESSAGEMAN:Broadcast("ChartPreviewOn")
				heyiwasusingthat = false
			end
			self:queuecommand("On")
			update = true
		else
			if GAMESTATE:GetCurrentSong() and noteField and mcbootlarder:GetVisible() then 
				mcbootlarder:visible(false)
				MESSAGEMAN:Broadcast("ChartPreviewOff")
				heyiwasusingthat = true
			end
			self:queuecommand("Off")
			update = false
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

-- Music Rate Display
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(18, SCREEN_BOTTOM - 225):visible(true):halign(0):zoom(0.4):maxwidth(
				capWideScale(get43size(360), 360) / capWideScale(get43size(0.45), 0.45)
			)
		end,
		SetCommand = function(self)
			self:settext(getCurRateDisplayString())
		end,
		CodeMessageCommand = function(self, params)
			local rate = getCurRateValue()
			ChangeMusicRate(rate, params)
			self:settext(getCurRateDisplayString())
		end,
		GoalSelectedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

-- Temporary update control tower; it would be nice if the basic song/step change commands were thorough and explicit and non-redundant
t[#t + 1] =
	Def.Actor {
	SetCommand = function(self)
		if song and not alreadybroadcasted then -- if this is true it means we've just exited a pack's songlist into the packlist
			song = GAMESTATE:GetCurrentSong() -- also apprently true if we're tabbing around within a songlist and then stop...
			MESSAGEMAN:Broadcast("UpdateChart") -- ms.ok(whee:GetSelectedSection( )) -- use this later to detect pack changes
			MESSAGEMAN:Broadcast("RefreshChartInfo")
		else
			alreadybroadcasted = false
		end
	end,
	CurrentStepsP1ChangedMessageCommand = function(self)
		song = GAMESTATE:GetCurrentSong()
		MESSAGEMAN:Broadcast("UpdateChart")
	end,
	CurrentSongChangedMessageCommand = function(self)
		-- This will disable mirror when switching songs if OneShotMirror is enabled or if permamirror is flagged on the chart (it is enabled if so in screengameplayunderlay/default)
		if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).OneShotMirror or profile:IsCurrentChartPermamirror() then
			local modslevel = topscreen == "ScreenEditOptions" and "ModsLevel_Stage" or "ModsLevel_Preferred"
			local playeroptions = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptions(modslevel)
			playeroptions:Mirror(false)
		end
		if not GAMESTATE:GetCurrentSong() and noteField and mcbootlarder:GetVisible() then 
			mcbootlarder:visible(false)
			MESSAGEMAN:Broadcast("ChartPreviewOff")
			heyiwasusingthat = true
		end
		if heyiwasusingthat and GAMESTATE:GetCurrentSong() and noteField and getTabIndex() == 0 then
			mcbootlarder:visible(true)
			MESSAGEMAN:Broadcast("ChartPreviewOn")
			heyiwasusingthat = false
		end
		self:queuecommand("Set")
	end
}

local function GetBestScoreByFilter(perc, CurRate)
	local rtTable = getRateTable()
	if not rtTable then
		return nil
	end

	local rates = tableKeys(rtTable)
	local scores, score

	if CurRate then
		local tmp = getCurRateString()
		if tmp == "1x" then
			tmp = "1.0x"
		end
		if tmp == "2x" then
			tmp = "2.0x"
		end
		rates = {tmp}
		if not rtTable[rates[1]] then
			return nil
		end
	end

	table.sort(
		rates,
		function(a, b)
			a = a:gsub("x", "")
			b = b:gsub("x", "")
			return a < b
		end
	)
	for i = #rates, 1, -1 do
		scores = rtTable[rates[i]]
		local bestscore = 0
		local index

		for ii = 1, #scores do
			score = scores[ii]
			if score:ConvertDpToWife() > bestscore then
				index = ii
				bestscore = score:ConvertDpToWife()
			end
		end

		if index and scores[index]:GetWifeScore() == 0 and GetPercentDP(scores[index]) > perc * 100 then
			return scores[index]
		end

		if bestscore > perc then
			return scores[index]
		end
	end
end

local function GetDisplayScore()
	local score
	score = GetBestScoreByFilter(0, true)

	if not score then
		score = GetBestScoreByFilter(0.9, false)
	end
	if not score then
		score = GetBestScoreByFilter(0.5, false)
	end
	if not score then
		score = GetBestScoreByFilter(0, false)
	end
	return score
end

local function toggleNoteField()
	if song and not noteField then	-- first time setup
		noteField = true
		MESSAGEMAN:Broadcast("ChartPreviewOn") -- for banner reaction... lazy -mina
		mcbootlarder:playcommand("SetupNoteField")
		mcbootlarder:xy(prevX,prevY)
		mcbootlarder:GetChild("NoteField"):xy(prevX+idkwhatimdoing, prevY*1.5)
		if usingreverse then
			mcbootlarder:GetChild("NoteField"):y(prevY*1.5 + prevrevY)
		end
	return end

	if song then 
		if mcbootlarder:GetVisible() then
			mcbootlarder:visible(false)
			MESSAGEMAN:Broadcast("ChartPreviewOff")
		else
			mcbootlarder:visible(true)
			MESSAGEMAN:Broadcast("ChartPreviewOn")
		end
	end
end

t[#t + 1] =
	Def.Actor {
	SetCommand = function(self)
		if song then
			ptags = tags:get_data().playerTags
			steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
			chartKey = steps:GetChartKey()
			score = GetDisplayScore()
			ctags = {}
			for k, v in pairs(ptags) do
				if ptags[k][chartKey] then
					ctags[#ctags + 1] = k
				end
			end
			MESSAGEMAN:Broadcast("RefreshChartInfo")
		end
	end,
	UpdateChartMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	CurrentRateChangedMessageCommand = function()
		score = GetDisplayScore()
	end
}

t[#t + 1] =
	Def.ActorFrame {
	-- **frames/bars**
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY - 76):zoomto(110, 94):halign(0):valign(0):diffuse(color("#333333CC")):diffusealpha(0.66)
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY + 18):zoomto(frameWidth + 4, 50):halign(0):valign(0):diffuse(color("#333333CC")):diffusealpha(
				0.66
			)
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX, frameY - 76):zoomto(8, 144):halign(0):valign(0):diffuse(getMainColor("highlight")):diffusealpha(0.5)
		end
	},
	-- **score related stuff** These need to be updated with rate changed commands
	-- Primary percent score
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 55, frameY + 50):zoom(0.6):halign(0.5):maxwidth(125):valign(1)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				if song and score then
					self:settextf("%05.2f%%", notShit.floor(score:GetWifeScore() * 10000) / 100)
					self:diffuse(getGradeColor(score:GetWifeGrade()))
				else
					self:settext("")
				end
			end,
			RefreshChartInfoMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CurrentRateChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	-- Rate for the displayed score
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 55, frameY + 58):zoom(0.5):halign(0.5)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				if song and score then
					local rate = notShit.round(score:GetMusicRate(), 3)
					local notCurRate = notShit.round(getCurRateValue(), 3) ~= rate

					local rate = string.format("%.2f", rate)
					if rate:sub(#rate, #rate) == "0" then
						rate = rate:sub(0, #rate - 1)
					end
					rate = rate .. "x"

					if notCurRate then
						self:settext("(" .. rate .. ")")
					else
						self:settext(rate)
					end
				else
					self:settext("")
				end
			end,
			CurrentRateChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			RefreshChartInfoMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	-- Date score achieved on
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 185, frameY + 59):zoom(0.4):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				if song and score then
					self:settext(score:GetDate())
				else
					self:settext("")
				end
			end,
			CurrentRateChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			RefreshChartInfoMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	-- MaxCombo
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 185, frameY + 49):zoom(0.4):halign(0)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				if song and score then
					self:settextf("Max Combo: %d", score:GetMaxCombo())
				else
					self:settext("")
				end
			end,
			CurrentRateChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			RefreshChartInfoMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
	-- **End score related stuff**
}

-- "Radar values" aka basic chart information
local function radarPairs(i)
	local o =
		Def.ActorFrame {
		LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 13, frameY - 52 + 13 * i):zoom(0.5):halign(0):maxwidth(120)
				end,
				SetCommand = function(self)
					if song then
						self:settext(ms.RelevantRadarsShort[i])
					else
						self:settext("")
					end
				end,
				RefreshChartInfoMessageCommand = function(self)
					self:queuecommand("Set")
				end
			},
		LoadFont("Common Normal") ..
			{
				InitCommand = function(self)
					self:xy(frameX + 105, frameY + -52 + 13 * i):zoom(0.5):halign(1):maxwidth(60)
				end,
				SetCommand = function(self)
					if song then
						self:settext(steps:GetRelevantRadars(PLAYER_1)[i])
					else
						self:settext("")
					end
				end,
				RefreshChartInfoMessageCommand = function(self)
					self:queuecommand("Set")
				end
			}
	}
	return o
end

-- Create the radar values
for i = 1, 5 do
	t[#t + 1] = radarPairs(i)
end

-- Difficulty value ("meter"), need to change this later
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 58, frameY - 62):halign(0.5):zoom(0.6):maxwidth(110 / 0.6)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song then
				if steps:GetStepsType() == "StepsType_Dance_Single" then
					local meter = steps:GetMSD(getCurRateValue(), 1)
					self:settextf("%05.2f", meter)
					self:diffuse(byMSD(meter))
				else
					self:settextf("%5.2f", steps:GetMeter())
					self:diffuse(byDifficulty(steps:GetDifficulty()))
				end
			else
				self:settext("")
			end
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

-- -- test adjustment index
-- t[#t+1] = LoadFont("Common normal")..{
-- InitCommand=function(self)
-- self:xy(frameX+92,frameY-70):halign(0):zoom(0.4)
-- end,
-- ChartLeaderboardUpdateMessageCommand = function(self,params)
-- local val = params.mmm
-- if val then
-- if val > 0 then
-- self:settextf("%+5.1f", val)
-- else
-- self:settextf("%5.1f", val)
-- end
-- else
-- self:settext("")
-- end
-- end,
-- LogOutMessageCommand=function(self)
-- self:settext("")
-- end,
-- RefreshChartInfoMessageCommand=function(self)
-- if song then
-- self:visible(true)
-- else
-- self:visible(false)
-- end
-- end,
-- }

-- Song duration
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy((capWideScale(get43size(384), 384)) + 62, SCREEN_BOTTOM - 85):visible(true):halign(1):zoom(
				capWideScale(get43size(0.6), 0.6)
			):maxwidth(capWideScale(get43size(360), 360) / capWideScale(get43size(0.45), 0.45))
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song then
				local playabletime = GetPlayableTime()
				self:settext(SecondsToMMSS(playabletime))
				self:diffuse(byMusicLength(playabletime))
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

-- BPM display/label not sure why this was never with the chart info in the first place
t[#t + 1] =
	Def.BPMDisplay {
	File = THEME:GetPathF("BPMDisplay", "bpm"),
	Name = "BPMDisplay",
	InitCommand = function(self)
		self:xy(capWideScale(get43size(384), 384) + 62, SCREEN_BOTTOM - 100):halign(1):zoom(0.50)
	end,
	SetCommand = function(self)
		if song then
			self:visible(1)
			self:SetFromSong(song)
		else
			self:visible(0)
		end
	end,
	CurrentRateChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	RefreshChartInfoMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		SetCommand = function(self)
			if song then
				self:settext("BPM")
			else
				self:settext("")
			end
		end,
		InitCommand = function(self)
			self:xy(capWideScale(get43size(384), 384) + 41, SCREEN_BOTTOM - 100):halign(1):zoom(0.50)
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

-- CDtitle, need to figure out a better place for this later. -mina
--Gonna move the cdtitle right next to selected song similar to ultralight. -Misterkister
t[#t + 1] =
	Def.Sprite {
	InitCommand = function(self)
		self:xy(capWideScale(get43size(344), 364) + 50, capWideScale(get43size(345), 255)):halign(0.5):valign(1)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if GAMESTATE:GetCurrentSong() then
			local song = GAMESTATE:GetCurrentSong()
			if song then
				if song:HasCDTitle() then
					self:visible(true)
					self:Load(song:GetCDTitlePath())
				else
					self:visible(false)
				end
			else
				self:visible(false)
			end
			local height = self:GetHeight()
			local width = self:GetWidth()

			if height >= 60 and width >= 75 then
				if height * (75 / 60) >= width then
					self:zoom(60 / height)
				else
					self:zoom(75 / width)
				end
			elseif height >= 60 then
				self:zoom(60 / height)
			elseif width >= 75 then
				self:zoom(75 / width)
			else
				self:zoom(1)
			end
		else
			self:visible(false)
		end
	end,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	RefreshChartInfoMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX, frameY - 120):halign(0):zoom(0.4)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song and steps:GetTimingData():HasWarps() then
				self:settext("NegBpms!")
			else
				self:settext("")
			end
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 135, frameY + 45):zoom(0.3):halign(0.5):valign(1)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song and steps then
				local goal = profile:GetEasiestGoalForChartAndRate(steps:GetChartKey(), getCurRateValue())
				if goal then
					self:settext("Target:")
				else
					self:settext("")
				end
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 135, frameY + 60):zoom(0.3):halign(0.5):valign(1)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song and steps then
				local goal = profile:GetEasiestGoalForChartAndRate(steps:GetChartKey(), getCurRateValue())
				if goal then
					self:settextf("%.2f%%", goal:GetPercent() * 100)
				else
					self:settext("")
				end
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX + 135, frameY + 45):zoomto(50, 40):diffusealpha(0)
	end,
	MouseLeftClickMessageCommand = function(self)
		if song and steps then
			local sg = profile:GetEasiestGoalForChartAndRate(steps:GetChartKey(), getCurRateValue())
			if sg and isOver(self) and update then
				sg:SetPercent(sg:GetPercent() + 0.01)
				MESSAGEMAN:Broadcast("RefreshChartInfo")
			end
		end
	end,
	MouseRightClickMessageCommand = function(self)
		if song and steps then
			local sg = profile:GetEasiestGoalForChartAndRate(steps:GetChartKey(), getCurRateValue())
			if sg and isOver(self) and update then
				sg:SetPercent(sg:GetPercent() - 0.01)
				MESSAGEMAN:Broadcast("RefreshChartInfo")
			end
		end
	end
}

-- perhaps need this perhaps not
-- t[#t+1] = LoadFont("Common Large") .. {
-- InitCommand=function(self)
-- 	self:xy(frameX+135,frameY+65):zoom(0.3):halign(0.5):valign(1)
-- end,
-- BeginCommand=function(self)
-- 	self:queuecommand("Set")
-- end,
-- SetCommand=function(self)
-- if steps then
-- local goal = profile:GetEasiestGoalForChartAndRate(steps:GetChartKey(), getCurRateValue())
-- if goal then
-- self:settextf("%.2f", goal:GetRate())
-- else
-- self:settext("")
-- end
-- else
-- self:settext("")
-- end
-- end,
-- CurrentStepsP1ChangedMessageCommand=function(self)
-- 	self:queuecommand("Set")
-- end,
-- RefreshChartInfoMessageCommand=function(self)
-- 	self:queuecommand("Set")
-- end,
-- }

-- t[#t+1] = LoadFont("Common Large") .. {
-- InitCommand=function(self)
-- 	self:xy((capWideScale(get43size(384),384))+68,SCREEN_BOTTOM-135):halign(1):zoom(0.4,maxwidth,125)
-- end,
-- BeginCommand=function(self)
-- 	self:queuecommand("Set")
-- end,
-- SetCommand=function(self)
-- if song then
-- self:settext(song:GetOrTryAtLeastToGetSimfileAuthor())
-- else
-- self:settext("")
-- end
-- end,
-- CurrentStepsP1ChangedMessageCommand=function(self)
-- 	self:queuecommand("Set")
-- end,
-- RefreshChartInfoMessageCommand=function(self)
-- 	self:queuecommand("Set")
-- end,
-- }

-- active filters display
-- t[#t+1] = Def.Quad{InitCommand=cmd(xy,16,capWideScale(SCREEN_TOP+172,SCREEN_TOP+194);zoomto,SCREEN_WIDTH*1.35*0.4 + 8,24;halign,0;valign,0.5;diffuse,color("#000000");diffusealpha,0),
-- EndingSearchMessageCommand=function(self)
-- self:diffusealpha(1)
-- end
-- }
-- t[#t+1] = LoadFont("Common Large") .. {
-- InitCommand=function(self)
-- 	self:xy(20,capWideScale(SCREEN_TOP+170,SCREEN_TOP+194)):halign(0):zoom(0.4):settext("Active Filters: "..GetPersistentSearch()):maxwidth(SCREEN_WIDTH*1.35)
-- end,
-- EndingSearchMessageCommand=function(self, msg)
-- self:settext("Active Filters: "..msg.ActiveFilter)
-- end
-- }

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 120, frameY - 60):halign(0):zoom(0.4, maxwidth, 125)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song then
				self:settext(steps:GetRelevantSkillsetsByMSDRank(getCurRateValue(), 1))
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		ChartPreviewOnMessageCommand = function(self)
			self:visible(false)
		end,
		ChartPreviewOffMessageCommand = function(self)
				self:visible(true)
		end,
	}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 120, frameY - 30):halign(0):zoom(0.4, maxwidth, 125)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song then
				self:settext(steps:GetRelevantSkillsetsByMSDRank(getCurRateValue(), 2))
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		ChartPreviewOnMessageCommand = function(self)
			self:visible(false)
		end,
		ChartPreviewOffMessageCommand = function(self)
				self:visible(true)
		end,
	}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 120, frameY):halign(0):zoom(0.4, maxwidth, 125)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song then
				self:settext(steps:GetRelevantSkillsetsByMSDRank(getCurRateValue(), 3))
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		ChartPreviewOnMessageCommand = function(self)
			self:visible(false)
		end,
		ChartPreviewOffMessageCommand = function(self)
				self:visible(true)
		end,
	}

-- tags?
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 300, frameY - 60):halign(0):zoom(0.4):maxwidth(450)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song and ctags[1] then
				self:settext(ctags[1])
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 300, frameY - 30):halign(0):zoom(0.4):maxwidth(450)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song and ctags[2] then
				self:settext(ctags[2])
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + 300, frameY):halign(0):zoom(0.4):maxwidth(450)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			if song and ctags[3] then
				self:settext(ctags[3])
			else
				self:settext("")
			end
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

--test actor
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX, frameY - 120):halign(0):zoom(0.4, maxwidth, 125)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			--ms.type(profile:GetGoalByKey(getCurKey()))
			self:settext("")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		RefreshChartInfoMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

	local yesiwantnotefield = false
--Chart Preview Button

local function ihatestickinginputcallbackseverywhere(event)
	if event.type ~= "InputEventType_Release" and getTabIndex() == 0 then
				if event.DeviceInput.button == "DeviceButton_space" then
					toggleNoteField()
				end
			end
	return false
end

t[#t + 1] = LoadFont("Common Normal") .. {
	Name = "PreviewViewer",
	BeginCommand = function(self)
		mcbootlarder = self:GetParent():GetChild("ChartPreview")
		SCREENMAN:GetTopScreen():AddInputCallback(ihatestickinginputcallbackseverywhere)
		self:xy(20, 235)
		self:zoom(0.5)
		self:halign(0)
		self:settext("Toggle Preview")
	end,
	RefreshChartInfoMessageCommand = function(self)
		if song then
			self:visible(true)
		else
			self:visible(false)
		end
	end,
	MouseLeftClickMessageCommand = function(self)
		if isOver(self) and (song or noteField) then
			 toggleNoteField()
		end
	end,
	CurrentStyleChangedMessageCommand=function(self)	-- need to regenerate the notefield when changing styles or crashman appears -mina
		if noteField then
			SCREENMAN:GetTopScreen():DeletePreviewNoteField(mcbootlarder)
			noteField = false
			toggleNoteField()
		end
	end
}

	t[#t + 1] = LoadActor("../_chartpreview.lua")
return t
