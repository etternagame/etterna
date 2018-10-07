--Similar to dummy.lua and I'm just commenting again because I don't know what I am exactly doing. -Misterkister

local update = false
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0) -- visible(false()
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

local stepstuffX = frameX + frameWidth - offsetX - 300
local stepstuffY = frameY + offsetY - 23

if not IsUsingWideScreen() == true then
	stepstuffX = frameX + frameWidth - offsetX + 30
	stepstuffY = frameY + offsetY - 13 + 110
end

local radarValues = {
	{"RadarCategory_Notes", "Notes"},
	{"RadarCategory_TapsAndHolds", "Taps"},
	{"RadarCategory_Jumps", "Jumps"},
	{"RadarCategory_Hands", "Hands"},
	{"RadarCategory_Holds", "Holds"},
	{"RadarCategory_Rolls", "Rolls"},
	{"RadarCategory_Mines", "Mines"}
}

local radarX = frameX + offsetX + 450
local stuffstuffstuffX = frameX + offsetX + 400

if not IsUsingWideScreen() == true then
	radarX = frameX + offsetX + 450 - 120
	stuffstuffstuffX = frameX + offsetX + 400 - 105
end

--Hacky way of fixing these ratios outside of 16:9 and 4:3. -Misterkister

--16:10 ratio. -Misterkister
if round(GetScreenAspectRatio(), 5) == 1.6 then
	radarX = frameX + offsetX + 450 - 45
	stuffstuffstuffX = frameX + offsetX + 400 - 40
end

--5:4 ratio. -Misterkister
if round(GetScreenAspectRatio(), 5) == 1.25 then
	radarX = frameX + offsetX + 310
	stuffstuffstuffX = frameX + offsetX + 270
end

--8:3 ratio targeted. -Misterkister
if round(GetScreenAspectRatio(), 5) > 1.77778 then
	radarX = SCREEN_CENTER_X + 50
	stuffstuffstuffX = SCREEN_CENTER_X
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
