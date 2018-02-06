--[[ Screen Filter ]]


local padding = 20 -- 10px on each side
local arrowWidth = 64 -- until noteskin metrics are implemented...

local filterColor = color("0,0,0,0")
local filterAlphas = {
	PlayerNumber_P1 = 1,
	PlayerNumber_P2 = 1,
	Default = 1,
}

--moving notefield shenanigans
local rPressed = false
local tPressed = false
local noteFieldWidth = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplaySizes.NotefieldWidth
local notefieldX = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).GameplayXYCoordinates.NotefieldX
local filter

local function input(event)
	if getAutoplay() ~= 0 then
		if event.DeviceInput.button == "DeviceButton_r" then
			rPressed = not (event.type == "InputEventType_Release")
		end
		if event.DeviceInput.button == "DeviceButton_t" then
			tPressed = not (event.type == "InputEventType_Release")
		end
		if rPressed and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_left" then
				filter:addx(-3)
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				filter:addx(3)
			end
		end
		if tPressed and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_left" then
				noteFieldWidth = noteFieldWidth - 0.01
				filter:playcommand("Update")
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				noteFieldWidth = noteFieldWidth + 0.01
				filter:playcommand("Update")
			end
		end
	end
	return false
end

local t = Def.ActorFrame{
	OnCommand=function()
		if(playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay) then
			SCREENMAN:GetTopScreen():AddInputCallback(input)
		end
	end
};

local style = GAMESTATE:GetCurrentStyle()
local cols = style:ColumnsPerPlayer()

local numPlayers = GAMESTATE:GetNumPlayersEnabled()
local center1P = ((cols >= 6) or PREFSMAN:GetPreference("Center1Player"))

local styleType = ToEnumShortString(style:GetStyleType())
local filterWidth = (arrowWidth * cols) + padding

if numPlayers == 1 then
	local player = GAMESTATE:GetMasterPlayerNumber()
	local pNum = (player == PLAYER_1) and 1 or 2

	filterAlphas[player] = playerConfig:get_data(pn_to_profile_slot(player)).ScreenFilter;
	if filterAlphas[player] == nil then
		filterAlphas[player] = 0
	else
		filterAlphas[player] = tonumber(filterAlphas[player])
	end;

	local pos;
	-- [ScreenGameplay] PlayerP#Player*Side(s)X
	if center1P then
		pos = SCREEN_CENTER_X
	else
		local metricName = string.format("PlayerP%i%sX",pNum,styleType)
		pos = THEME:GetMetric("ScreenGameplay",metricName)
	end
	t[#t+1] = Def.Quad{
		Name="SinglePlayerFilter";
		InitCommand=function(self)
			self:x(pos)
			self:CenterY()
			self:zoomto(filterWidth*getNoteFieldScale(player)*noteFieldWidth,SCREEN_HEIGHT)
			self:diffusecolor(filterColor)
			self:diffusealpha(filterAlphas[player])
			self:addx(notefieldX)
			filter = self
		end,
		UpdateCommand=function(self)
			local player = GAMESTATE:GetMasterPlayerNumber()
			self:zoomto(filterWidth*getNoteFieldScale(player)*noteFieldWidth,SCREEN_HEIGHT)
		end,
	};
else
	-- two players... a bit more complex.
	if styleType == "TwoPlayersSharedSides" then
		-- routine, just use one in the center.
		local player = GAMESTATE:GetMasterPlayerNumber()
		local pNum = player == PLAYER_1 and 1 or 2
		local metricName = "PlayerP".. pNum .."TwoPlayersSharedSidesX"
		t[#t+1] = Def.Quad{
			Name="RoutineFilter";
			InitCommand=function(self)
				self:x(THEME:GetMetric("ScreenGameplay",metricName)):CenterY():zoomto(filterWidth*getNoteFieldScale(player),SCREEN_HEIGHT):diffusecolor(filterColor):diffusealpha(filterAlphas[player])
			end;
		};
	else
		-- otherwise we need two separate ones. to the pairsmobile!
		for i, player in ipairs(PlayerNumber) do
			local pNum = (player == PLAYER_1) and 1 or 2
			filterAlphas[player] = playerConfig:get_data(pn_to_profile_slot(player)).ScreenFilter
			local metricName = string.format("PlayerP%i%sX",pNum,styleType)
			local pos = THEME:GetMetric("ScreenGameplay",metricName)
			t[#t+1] = Def.Quad{
				Name="Player"..pNum.."Filter";
				InitCommand=function(self)
					self:x(pos):CenterY():zoomto(filterWidth*getNoteFieldScale(player),SCREEN_HEIGHT):diffusecolor(filterColor):diffusealpha(filterAlphas[player])
				end;
			};
		end
	end
end

return t;