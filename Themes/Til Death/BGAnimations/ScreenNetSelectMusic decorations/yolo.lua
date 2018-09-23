--Similar to dummy.lua and I'm just commenting again because I don't know what I am exactly doing. -Misterkister

local update = false
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set"):visible(false)
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
			self:queuecommand("On")
			self:visible(true)
			update = true
		else
			self:queuecommand("Off")
			update = false
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	PlayerJoinedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320, 400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX = 10
local offsetY = 20
local pn = GAMESTATE:GetEnabledPlayers()[1]

local cdtitleX = frameX + 440
local cdtitleY = frameY + 225

if not IsUsingWideScreen() == true then
	cdtitleX = frameX + 440 - 105
	cdtitleY = frameY + 225
end

t[#t + 1] =
	Def.Sprite {
	InitCommand = function(self)
		self:xy(cdtitleX, cdtitleY + 10):zoomy(0):valign(1)
	end,
	Name = "CDTitle",
	SetCommand = function(self)
		if update then
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

			if height >= 80 and width >= 100 then
				if height * (100 / 80) >= width then
					self:zoom(80 / height)
				else
					self:zoom(100 / width)
				end
			elseif height >= 80 then
				self:zoom(80 / height)
			elseif width >= 100 then
				self:zoom(100 / width)
			else
				self:zoom(1)
			end
			self:diffusealpha(1)
		end
	end,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

local stepstuffX = frameX + frameWidth - offsetX - 300
local stepstuffY = frameY + offsetY - 23

if not IsUsingWideScreen() == true then
	stepstuffX = frameX + frameWidth - offsetX + 30
	stepstuffY = frameY + offsetY - 13 + 110
end

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "StepsAndMeter",
		InitCommand = function(self)
			self:xy(stepstuffX, stepstuffY):zoom(0.4):halign(1)
		end,
		SetCommand = function(self)
			local steps = GAMESTATE:GetCurrentSteps(pn)
			local song = GAMESTATE:GetCurrentSong()
			local notecount = 0
			local length = 1
			if steps ~= nil and song ~= nil and update then
				length = song:GetStepsSeconds()
				notecount = steps:GetRadarValues(pn):GetValue("RadarCategory_Notes")
				self:settext(string.format("%0.2f Average NPS", notecount / length))
				self:diffuse(Saturation(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(), steps:GetDifficulty())), 0.3))
			else
				self:settext("0.00 Average NPS")
			end
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentStepsP1ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentStepsP2ChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

local radarValues = {
	{"RadarCategory_Notes", "Notes"},
	{"RadarCategory_TapsAndHolds", "Taps"},
	{"RadarCategory_Jumps", "Jumps"},
	{"RadarCategory_Hands", "Hands"},
	{"RadarCategory_Holds", "Holds"},
	{"RadarCategory_Rolls", "Rolls"},
	{"RadarCategory_Mines", "Mines"},
	{"RadarCategory_Lifts", "Lifts"},
	{"RadarCategory_Fakes", "Fakes"}
}

local radarX = frameX + offsetX + 450
local stuffstuffstuffX = frameX + offsetX + 400

if not IsUsingWideScreen() == true then
	radarX = frameX + offsetX + 450 - 120
	stuffstuffstuffX = frameX + offsetX + 400 - 110
end

for k, v in ipairs(radarValues) do
	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(stuffstuffstuffX, frameY + offsetY + 230 + (15 * (k - 1))):zoom(0.4):halign(0):maxwidth(
					(frameWidth - offsetX * 2 - 150) / 0.4
				)
			end,
			OnCommand = function(self)
				self:settext(v[2] .. ": ")
			end
		}
	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			Name = "RadarValue" .. v[1],
			InitCommand = function(self)
				self:xy(radarX, frameY + offsetY + 230 + (15 * (k - 1))):zoom(0.4):halign(0):maxwidth(
					(frameWidth - offsetX * 2 - 150) / 0.4
				)
			end,
			SetCommand = function(self)
				local song = GAMESTATE:GetCurrentSong()
				local steps = GAMESTATE:GetCurrentSteps(pn)
				local count = 0
				if song ~= nil and steps ~= nil and update then
					count = steps:GetRadarValues(pn):GetValue(v[1])
					self:settext(count)
					self:diffuse(color("#FFFFFF"))
				else
					self:settext(0)
					self:diffuse(getMainColor("disabled"))
				end
			end,
			CurrentSongChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CurrentStepsP1ChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CurrentStepsP2ChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
end

return t
