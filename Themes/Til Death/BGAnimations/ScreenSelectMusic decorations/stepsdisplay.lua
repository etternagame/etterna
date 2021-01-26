local itsOn = false
local stepsdisplayx = SCREEN_WIDTH * 0.56 - capWideScale(48, 56)
local thesteps

local rowwidth = 60
local rowheight = 17
local cursorwidth = 6
local cursorheight = 17

local numshown = 7
local currentindex = 1
local displayindexoffset = 0

local sd =
	Def.ActorFrame {
	Name = "StepsDisplay",
	InitCommand = function(self)
		self:xy(stepsdisplayx, 68):valign(0)
	end,
	OffCommand = function(self)
		self:visible(false)
	end,
	OnCommand = function(self)
		self:visible(true)
	end,
	TabChangedMessageCommand = function(self, params)
		self:finishtweening()
		if getTabIndex() < 3 and GAMESTATE:GetCurrentSong() then
			-- dont display this if the score nested tab is already online
			-- prevents repeat 3 presses to break the display
			-- (let page 2 handle this specifically)
			if params.to == 2 then
				return
			end
			self:playcommand("On")
		else
			self:playcommand("Off")
		end
	end,
	CurrentSongChangedMessageCommand = function(self, song)
		local song = song.ptr
		if song then 
			thesteps = song:GetChartsMatchingFilter()
			-- if in online scores tab it still pops up for 1 frame
			-- so the bug fixed in the above command makes a return
			-- how sad
			if getTabIndex() < 3 then
				self:playcommand("On")
			end
			self:playcommand("UpdateStepsRows")
		else 
			self:playcommand("Off")
		end
	end,
	DelayedChartUpdateMessageCommand = function(self)
		local leaderboardEnabled =
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).leaderboardEnabled and DLMAN:IsLoggedIn()
		if leaderboardEnabled and GAMESTATE:GetCurrentSteps() then
			local chartkey = GAMESTATE:GetCurrentSteps():GetChartKey()
			if SCREENMAN:GetTopScreen():GetMusicWheel():IsSettled() then
				DLMAN:RequestChartLeaderBoardFromOnline(
					chartkey,
					function(leaderboard)
					end
				)
			end
		end
	end,
	ChartPreviewOnMessageCommand = function(self)
		if not itsOn then
			self:addx(capWideScale(12, 0)):addy(capWideScale(18, 0))
			itsOn = true
		end
	end,
	ChartPreviewOffMessageCommand = function(self)
		if itsOn then
			self:addx(capWideScale(-12, 0)):addy(capWideScale(-18, 0))
			itsOn = false
		end
	end,
	CalcInfoOnMessageCommand = function(self)
		self:x(20)
	end,
	CalcInfoOffMessageCommand = function(self)
		self:x(stepsdisplayx)
	end
}

local function stepsRows(i)
	local o = Def.ActorFrame {
		InitCommand = function(self)
			self:y(rowheight * (i - 1))
		end,
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(rowwidth, rowheight):halign(0)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:visible(true)
					local diff = steps:GetDifficulty()
					self:diffuse(getDifficultyColor(diff))
					self:diffusealpha(0.4)
				else 
					self:visible(false)
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps and isOver(self) then
					SCREENMAN:GetTopScreen():ChangeSteps(i - currentindex)
					SCREENMAN:GetTopScreen():ChangeSteps(0)
				end
			end
		},
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(24, rowheight):halign(0)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:visible(true)
					local diff = steps:GetDifficulty()
					self:diffuse(byDifficulty(diff))					
				else 
					self:visible(false)
				end
			end
		},
		-- Chart defined "Meter" value, not msd (useful to have this for reference)
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(rowwidth - cursorwidth - 5):addy(-1):zoom(0.35):settext(""):halign(1)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:settext(steps:GetMeter())
				else
					self:settext("")
				end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(12):zoom(0.2):settext(""):halign(0.5):valign(0)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then
				local diff = steps:GetDifficulty()
					self:settext(getShortDifficulty(diff))
				else
					self:settext("")
				end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(12):addy(-9):zoom(0.2):settext(""):halign(0.5):valign(0):maxwidth(20 / 0.2)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then
				local st = THEME:GetString("StepsDisplay StepsType", ToEnumShortString(steps:GetStepsType()))
					self:settext(st)
				else
					self:settext("")
				end
			end
		}
	}

	return o
end

local sdr =
	Def.ActorFrame {
	Name = "StepsRows"
}

for i = 1, numshown do
	sdr[#sdr + 1] = stepsRows(i)
end
sd[#sd + 1] = sdr

local center = math.ceil(numshown / 2)
-- cursor goes on top
sd[#sd + 1] = Def.Quad {
	InitCommand = function(self)
		self:x(rowwidth):zoomto(cursorwidth, cursorheight):halign(1):valign(0.5):diffusealpha(0.6)
	end,
	CurrentStepsChangedMessageCommand = function(self, steps)
		for i = 1, 20 do
			if thesteps and thesteps[i] and thesteps[i] == steps.ptr then
				currentindex = i
				break
			end
		end

		if currentindex <= center then
			displayindexoffset = 0
		elseif #thesteps - displayindexoffset > numshown then
			displayindexoffset = currentindex - center
			currentindex = center
		else
			currentindex = currentindex - displayindexoffset
		end

		if #thesteps > numshown and #thesteps - displayindexoffset < numshown then
			displayindexoffset = #thesteps - numshown 
		end

		self:y(cursorheight * (currentindex - 1))
		self:GetParent():GetChild("StepsRows"):queuecommand("UpdateStepsRows")
	end
}

return sd