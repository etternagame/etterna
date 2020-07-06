local moveUpP1 = false
local moveDownP1 = false
local lockSpeedP1 = false

local cover

local laneColor = getLaneCoverColor("cover")
local bpmColor = getLaneCoverColor("bpmText")
local heightColor = getLaneCoverColor("heightText")

local cols = GAMESTATE:GetCurrentStyle():ColumnsPerPlayer()
local evencols = cols - cols%2
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay

local isCentered = ((cols >= 6) or PREFSMAN:GetPreference("Center1Player")) and GAMESTATE:GetNumPlayersEnabled() == 1
-- load from prefs later
local nfspace = MovableValues.NotefieldSpacing and MovableValues.NotefieldSpacing or 0
local width = 64 * cols * MovableValues.NotefieldWidth + nfspace * (evencols)
local padding = 8
local styleType = ToEnumShortString(GAMESTATE:GetCurrentStyle():GetStyleType())

local prefsP1 = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCover
local enabledP1 = prefsP1 ~= 0 and GAMESTATE:IsPlayerEnabled(PLAYER_1)
local isReverseP1 = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
if prefsP1 == 2 then -- it's a Hidden LaneCover
	isReverseP1 = not isReverseP1
end

local heightP1 = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCoverHeight

local P1X =
	SCREEN_CENTER_X + MovableValues.NotefieldX + (cols % 2 == 0 and -nfspace / 2 or 0)

if not isCentered then
	P1X = THEME:GetMetric("ScreenGameplay", string.format("PlayerP1%sX", styleType))
end

local function getPlayerBPM(pn)
	local pn = GAMESTATE:GetMasterPlayerNumber()
	local songPosition = GAMESTATE:GetPlayerState(pn):GetSongPosition()
	local ts = SCREENMAN:GetTopScreen()
	local bpm = 0
	if ts:GetScreenType() == "ScreenType_Gameplay" then
		bpm = ts:GetTrueBPS(pn) * 60
	end
	return bpm
end

local function getMaxDisplayBPM(pn)
	local pn = GAMESTATE:GetMasterPlayerNumber()
	local song = GAMESTATE:GetCurrentSong()
	local steps = GAMESTATE:GetCurrentSteps(pn)
	if steps:GetDisplayBPMType() ~= "DisplayBPM_Random" then
		return steps:GetDisplayBpms()[2]
	else
		return steps:GetTimingData():GetActualBPM()[2]
	end
end

local function getSpeed(pn)
	local po = GAMESTATE:GetPlayerState(pn):GetPlayerOptions("ModsLevel_Preferred")
	if po:XMod() ~= nil then
		return po:XMod() * getPlayerBPM(pn)
	elseif po:CMod() ~= nil then
		return po:CMod()
	elseif po:MMod() ~= nil then
		return po:MMod() * (getPlayerBPM(pn) / getMaxDisplayBPM(pn))
	else
		return getPlayerBPM(pn)
	end
end

local yoffsetreverse = THEME:GetMetric("Player", "ReceptorArrowsYReverse")
local yoffsetstandard = THEME:GetMetric("Player", "ReceptorArrowsYStandard")

local function getNoteFieldHeight(pn)
	local usingreverse = GAMESTATE:GetPlayerState(pn):GetCurrentPlayerOptions():UsingReverse()
	if usingreverse then
		return SCREEN_CENTER_Y + yoffsetreverse
	else
		return SCREEN_CENTER_Y - yoffsetstandard
	end
end

local function getScrollSpeed(pn, LaneCoverHeight)
	local height = getNoteFieldHeight(pn)
	local speed = getSpeed(pn)

	if LaneCoverHeight < height then
		return speed * (height / (height - LaneCoverHeight))
	else
		return 0
	end
end

--inaccurate since the highspeed x1 speed definition is different between the games.
--iidx x1 is defined as the whole measure showing up on the notefield
--SM x1 is defined as 4th notes being next to each other with no gaps or overlaps.
--Also note that the number given by this function does not take into account the fact
-- the notefield may have been moved up or down. It also does not work for the Hidden mode.
local function getIIDXGreenNumber(pn, LaneCoverHeight)
	return (174 * ((getNoteFieldHeight(pn) - LaneCoverHeight) * (1000 / getNoteFieldHeight(pn)))) /
		((getSpeed(pn) / getPlayerBPM(pn)) * getPlayerBPM(pn))
end

local function input(event)
	if getAutoplay() ~= 0 and playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCover ~= 0 then
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
				width = 64 * cols * MovableValues.NotefieldWidth - 0.01 + MovableValues.NotefieldSpacing * (cols-1)
				local dir = event.DeviceInput.button
				local inc = Movable.DeviceButton_t[dir].inc
				P1X = P1X + inc
				cover:playcommand("Update")
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				width = 64 * cols * MovableValues.NotefieldWidth + 0.01 + MovableValues.NotefieldSpacing * (cols-1)
				local dir = event.DeviceInput.button
				local inc = Movable.DeviceButton_t[dir].inc
				P1X = P1X + inc
				cover:playcommand("Update")
			end
		end
		if Movable.current == "DeviceButton_n" and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_up" or event.DeviceInput.button == "DeviceButton_down" then
				local dir = event.DeviceInput.button
				local inc = Movable.DeviceButton_n[dir].inc
				width = width + inc * evencols
				P1X = P1X - inc / evencols
				cover:playcommand("Update")
			end
		end
	end
	return false
end

local t =
	Def.ActorFrame {
	CodeMessageCommand = function(self, params)
		moveDownP1 = false
		moveUpP1 = false
		local doot = heightP1
		if params.PlayerNumber == PLAYER_1 and allowedCustomization then
			if params.Name == "LaneUp" then
				moveUpP1 = true
			elseif params.Name == "LaneDown" then
				moveDownP1 = true
			else
				moveDownP1 = false
				moveUpP1 = false
			end
			self:playcommand("SavePrefs")
		end
	end,
	SavePrefsCommand = function(self)
		if enabledP1 then
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCoverHeight = heightP1
			playerConfig:set_dirty(pn_to_profile_slot(PLAYER_1))
			playerConfig:save(pn_to_profile_slot(PLAYER_1))
		end
	end,
	OnCommand = function()
		if (allowedCustomization) then
			SCREENMAN:GetTopScreen():AddInputCallback(input)
		end
	end
}

if enabledP1 then
	t[#t + 1] =
		Def.Quad {
		Name = "CoverP1",
		InitCommand = function(self)
			self:xy(P1X, SCREEN_TOP):zoomto((width + padding) * getNoteFieldScale(PLAYER_1), heightP1):valign(0):diffuse(
				laneColor
			)
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
				self:xy(P1X, SCREEN_TOP):zoomto((width + padding) * getNoteFieldScale(PLAYER_1), heightP1):valign(0):diffuse(
					laneColor
				)
				self:y(SCREEN_TOP)
				self:valign(0)
			else
				self:xy(P1X, SCREEN_TOP):zoomto((width + padding) * getNoteFieldScale(PLAYER_1), heightP1):valign(0):diffuse(
					laneColor
				)
				self:y(SCREEN_BOTTOM)
				self:valign(1)
			end
			cover = self
		end
	}

	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			Name = "CoverTextP1White",
			InitCommand = function(self)
				self:x(P1X - (width * getNoteFieldScale(PLAYER_1) / 8)):settext(0):valign(1):zoom(0.5):diffuse(heightColor)
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
	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			Name = "CoverTextP1Green",
			InitCommand = function(self)
				self:x(P1X + (width * getNoteFieldScale(PLAYER_1) / 8)):settext(0):valign(1):zoom(0.5):diffuse(bpmColor)
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
end

local function Update(self)
	t.InitCommand = function(self)
		self:SetUpdateFunction(Update)
	end
	self:SetUpdateRate(5)
	if enabledP1 then

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
		self:GetChild("CoverTextP1White"):settext(math.floor(heightP1))
		if prefsP1 == 1 then -- don't update greennumber for hidden lanecovers
			self:GetChild("CoverTextP1Green"):settext(math.floor(getScrollSpeed(PLAYER_1, heightP1)))
		end
		if isReverseP1 then
			self:GetChild("CoverTextP1White"):y(heightP1 - 5)
			self:GetChild("CoverTextP1Green"):y(heightP1 - 5)
		else
			self:GetChild("CoverTextP1White"):y(SCREEN_BOTTOM - heightP1 + 5)
			self:GetChild("CoverTextP1Green"):y(SCREEN_BOTTOM - heightP1 + 5)
		end

		if moveDownP1 or moveUpP1 then
			self:GetChild("CoverTextP1White"):finishtweening()
			self:GetChild("CoverTextP1White"):diffusealpha(1)
			self:GetChild("CoverTextP1White"):sleep(0.25)
			self:GetChild("CoverTextP1White"):smooth(0.75)
			self:GetChild("CoverTextP1White"):diffusealpha(0)
			self:GetChild("CoverTextP1Green"):finishtweening()
			self:GetChild("CoverTextP1Green"):diffusealpha(1)
			self:GetChild("CoverTextP1Green"):sleep(0.25)
			self:GetChild("CoverTextP1Green"):smooth(0.75)
			self:GetChild("CoverTextP1Green"):diffusealpha(0)
			self:GetChild("CoverTextP1White"):x(P1X - (width * getNoteFieldScale(PLAYER_1) / 8))
			self:GetChild("CoverTextP1Green"):x(P1X + (width * getNoteFieldScale(PLAYER_1) / 8))
		end
	end
end
if allowedCustomization then
	t.InitCommand = function(self)
		self:SetUpdateFunction(Update)
	end
end

return t
