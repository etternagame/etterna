local moveUpP1 = false
local moveDownP1 = false

local cover

if MovableValues == nil or MovableValues.NoteFieldWidth == nil then
	return Def.Actor {}
end

local laneColor = COLORS:getGameplayColor("LaneCover")
local bpmColor = COLORS:getGameplayColor("LaneCoverBPM")
local heightColor = COLORS:getGameplayColor("LaneCoverHeight")

local cols = GAMESTATE:GetCurrentStyle():ColumnsPerPlayer()
local evencols = cols - cols%2

-- load from prefs later
local nfspace = MovableValues.NoteFieldSpacing and MovableValues.NoteFieldSpacing or 0
local width = 64 * cols * MovableValues.NoteFieldWidth + nfspace * (evencols)
local padding = 8

local prefsP1 = playerConfig:get_data().LaneCover
local isReverseP1 = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse()
if prefsP1 == 2 then -- it's a Hidden LaneCover
	isReverseP1 = not isReverseP1
end

local heightP1 = playerConfig:get_data().LaneCoverHeight

if prefsP1 == 0 then
	return Def.Actor {Name = "Cover"}
end

local function getPlayerBPM()
	local songPosition = GAMESTATE:GetPlayerState():GetSongPosition()
	local ts = SCREENMAN:GetTopScreen()
	local bpm = 0
	if ts:GetScreenType() == "ScreenType_Gameplay" then
		bpm = ts:GetTrueBPS() * 60
	end
	return bpm
end

local function getMaxDisplayBPM()
	local steps = GAMESTATE:GetCurrentSteps()
	if steps:GetDisplayBPMType() ~= "DisplayBPM_Random" then
		return steps:GetDisplayBpms()[2]
	else
		return steps:GetTimingData():GetActualBPM()[2]
	end
end

local function getSpeed()
	local po = GAMESTATE:GetPlayerState():GetPlayerOptions("ModsLevel_Preferred")
	if po:XMod() ~= nil then
		return po:XMod() * getPlayerBPM()
	elseif po:CMod() ~= nil then
		return po:CMod()
	elseif po:MMod() ~= nil then
		return po:MMod() * (getPlayerBPM() / getMaxDisplayBPM())
	else
		return getPlayerBPM()
	end
end

local yoffsetreverse = THEME:GetMetric("Player", "ReceptorArrowsYReverse")
local yoffsetstandard = THEME:GetMetric("Player", "ReceptorArrowsYStandard")

local function getNoteFieldHeight()
	local usingreverse = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse()
	if usingreverse then
		return SCREEN_CENTER_Y + yoffsetreverse
	else
		return SCREEN_CENTER_Y - yoffsetstandard
	end
end

local function getScrollSpeed(LaneCoverHeight)
	local height = getNoteFieldHeight()
	local speed = getSpeed()

	if LaneCoverHeight < height then
		return speed * (height / (height - LaneCoverHeight))
	else
		return 0
	end
end

local selectPressed = false
local skibby = nil
local function input(event)
	if getAutoplay() ~= 0 and playerConfig:get_data().LaneCover ~= 0 then
		if Movable.current == "DeviceButton_r" and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_left" then
				cover:addx(-3)
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				cover:addx(3)
			end
		end
		if Movable.current == "DeviceButton_t" and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_left" then
				width = 64 * cols * MovableValues.NoteFieldWidth - 0.01 + MovableValues.NoteFieldSpacing * (cols-1)
				cover:playcommand("Update")
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				width = 64 * cols * MovableValues.NoteFieldWidth + 0.01 + MovableValues.NoteFieldSpacing * (cols-1)
				cover:playcommand("Update")
			end
		end
		if Movable.current == "DeviceButton_n" and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" or event.DeviceInput.button == "DeviceButton_down" then
				local dir = event.DeviceInput.button
				local inc = Movable.DeviceButton_n[dir].inc
				width = width + inc * evencols
				cover:playcommand("Update")
			end
		end
	end
	if event.type == "InputEventType_Release" then
		moveDownP1 = false
		moveUpP1 = false
		if event.button == "Select" then
			selectPressed = false
		end
	end
	if event.type == "InputEventType_FirstPress" then
		if event.button == "EffectUp" and selectPressed then
			moveDownP1 = false
			moveUpP1 = true
			skibby:playcommand("SavePrefs")
		elseif event.button == "EffectDown" and selectPressed then
			moveDownP1 = true
			moveUpP1 = false
			skibby:playcommand("SavePrefs")
		elseif event.button == "Select" then
			selectPressed = true
		end
	end
	return false
end

local t = Def.ActorFrame {
	Name = "LaneCover",
	InitCommand = function(self)
		skibby = self
	end,
	SavePrefsCommand = function(self)
		playerConfig:get_data().LaneCoverHeight = heightP1
		playerConfig:set_dirty()
		playerConfig:save()
	end,
}

t[#t + 1] = Def.Quad {
	Name = "CoverP1",
	InitCommand = function(self)
		self:valign(0)
		self:y(SCREEN_TOP)
		self:zoomto((width + padding) * getNoteFieldScale(PLAYER_1), heightP1)
		self:diffuse(laneColor)
		self:diffusealpha(1)
		cover = self
	end,
	BeginCommand = function(self)
		if isReverseP1 then
			self:y(SCREEN_TOP)
			self:valign(0)
		else
			self:y(SCREEN_BOTTOM)
			self:valign(1)
		end
	end,
	UpdateCommand = function(self)
		if isReverseP1 then
			self:valign(0)
			self:zoomto((width + padding) * getNoteFieldScale(PLAYER_1), heightP1)
			self:y(SCREEN_TOP)
			self:diffuse(laneColor)
			self:diffusealpha(1)
		else
			self:valign(1)
			self:zoomto((width + padding) * getNoteFieldScale(PLAYER_1), heightP1)
			self:y(SCREEN_BOTTOM)
			self:diffuse(laneColor)
			self:diffusealpha(1)
		end
		cover = self
	end
}

t[#t + 1] = LoadFont("Common Normal") .. {
	Name = "CoverTextP1White",
	InitCommand = function(self)
		self:valign(1)
		self:x(-(width * getNoteFieldScale(PLAYER_1) / 8))
		self:zoom(0.5)
		self:diffuse(heightColor)
		self:diffusealpha(1)
		self:settext(0)
	end,
	BeginCommand = function(self)
		self:settext(0)
		if isReverseP1 then
			self:y(heightP1 - 5)
			self:valign(1)
		else
			self:y(SCREEN_BOTTOM - heightP1 + 5)
			self:valign(0)
		end
		self:finishtweening()
		self:diffusealpha(1)
		self:sleep(0.25)
		self:smooth(0.75)
		self:diffusealpha(0)
	end
}
t[#t + 1] = LoadFont("Common Normal") .. {
	Name = "CoverTextP1Green",
	InitCommand = function(self)
		self:valign(1)
		self:x((width * getNoteFieldScale(PLAYER_1) / 8))
		self:zoom(0.5)
		self:diffuse(bpmColor)
		self:diffusealpha(1)
		self:settext(0)
	end,
	BeginCommand = function(self)
		self:settext(math.floor(getSpeed(PLAYER_1)))
		if isReverseP1 then
			self:y(heightP1 - 5)
			self:valign(1)
		else
			self:y(SCREEN_BOTTOM - heightP1 + 5)
			self:valign(0)
		end
		self:finishtweening()
		self:diffusealpha(1)
		self:sleep(0.25)
		self:smooth(0.75)
		self:diffusealpha(0)
	end
}

local function Update(self)
	t.InitCommand = function(self)
		self:SetUpdateFunction(Update)
	end
	self:SetUpdateRate(5)
	local whitetext = self:GetChild("CoverTextP1White")
	local greentext = self:GetChild("CoverTextP1Green")

	if moveDownP1 then
		if isReverseP1 then
			heightP1 = math.min(SCREEN_BOTTOM, math.max(0, heightP1 + 0.1))
		else
			heightP1 = math.min(SCREEN_BOTTOM, math.max(0, heightP1 - 0.1))
		end
	end
	if moveUpP1 then
		if isReverseP1 then
			heightP1 = math.min(SCREEN_BOTTOM, math.max(0, heightP1 - 0.1))
		else
			heightP1 = math.min(SCREEN_BOTTOM, math.max(0, heightP1 + 0.1))
		end
	end

	self:GetChild("CoverP1"):zoomy(heightP1)
	whitetext:settext(math.floor(heightP1))
	if prefsP1 == 1 then -- don't update greennumber for hidden lanecovers
		greentext:settext(math.floor(getScrollSpeed(heightP1)))
	end

	if isReverseP1 then
		whitetext:y(heightP1 - 5)
		greentext:y(heightP1 - 5)
	else
		whitetext:y(SCREEN_BOTTOM - heightP1 + 5)
		greentext:y(SCREEN_BOTTOM - heightP1 + 5)
	end

	if moveDownP1 or moveUpP1 then
		whitetext:finishtweening()
		whitetext:diffusealpha(1)
		whitetext:sleep(0.25)
		whitetext:smooth(0.75)
		whitetext:diffusealpha(0)

		greentext:finishtweening()
		greentext:diffusealpha(1)
		greentext:sleep(0.25)
		greentext:smooth(0.75)
		greentext:diffusealpha(0)

		whitetext:x(-(width * getNoteFieldScale(PLAYER_1) / 8))
		greentext:x((width * getNoteFieldScale(PLAYER_1) / 8))
	end
end

return t