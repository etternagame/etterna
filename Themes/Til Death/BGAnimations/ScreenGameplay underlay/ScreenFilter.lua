--[[ Screen Filter ]]
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local padding = 20 -- 10px on each side
local arrowWidth = 64 -- until noteskin metrics are implemented...

local filterColor = color("0,0,0,0")
local filterAlphas = {
	PlayerNumber_P1 = 1,
	PlayerNumber_P2 = 1,
	Default = 1
}

--moving notefield shenanigans
local rPressed = false
local tPressed = false
local noteFieldWidth = MovableValues.NotefieldWidth
local notefieldX = MovableValues.NotefieldX
local oldWidth = noteFieldWidth
local filter
local cbContainer

local function input(event)
	if getAutoplay() ~= 0 then -- not touching this currently, its fully bound with the notefield ones and doesnt have a message
		if Movable.current == "DeviceButton_r" and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_left" then
				filter:addx(-3)
				cbContainer:addx(-3)
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				filter:addx(3)
				cbContainer:addx(3)
			end
		end
		if Movable.current == "DeviceButton_t" and event.type ~= "InputEventType_Release" then
			if event.DeviceInput.button == "DeviceButton_left" then
				oldWidth = noteFieldWidth
				noteFieldWidth = noteFieldWidth - 0.01
				filter:playcommand("Update")
				cbContainer:playcommand("Update")
			end
			if event.DeviceInput.button == "DeviceButton_right" then
				oldWidth = noteFieldWidth
				noteFieldWidth = noteFieldWidth + 0.01
				filter:playcommand("Update")
				cbContainer:playcommand("Update")
			end
		end
	end
	return false
end

local style = GAMESTATE:GetCurrentStyle()
local cols = style:ColumnsPerPlayer()

local numPlayers = GAMESTATE:GetNumPlayersEnabled()
local center1P = ((cols >= 6) or PREFSMAN:GetPreference("Center1Player"))

local styleType = ToEnumShortString(style:GetStyleType())
local filterWidth = (arrowWidth * cols) + padding

local judgeThreshold = Enum.Reverse(TapNoteScore)[ComboContinue()]
local enabled = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CBHighlight

local alpha = 0.4


local function laneHighlight()
	local pn = PLAYER_1
	local r = Def.ActorFrame {
		InitCommand = function(self)
			cbContainer = self
		end
	}
	local xpos = getNoteFieldPos(pn)
	local width = style:GetWidth(pn)
	local colWidth = width/cols
	local border = 4
	
	if not enabled then
		return r
	end
	
	for i=1,cols do
		r[#r+1] = Def.Quad{
			InitCommand = function(self)
				self:zoomto(getNoteFieldScale(pn) * (arrowWidth - 4) * noteFieldWidth, SCREEN_HEIGHT)
				self:valign(0)
				
				local reverse = GAMESTATE:GetPlayerState(pn):GetCurrentPlayerOptions():UsingReverse()
				local receptor = reverse and THEME:GetMetric("Player", "ReceptorArrowsYStandard") or THEME:GetMetric("Player", "ReceptorArrowsYReverse")

				self:diffusealpha(alpha)
				local thewidth
				if noteFieldWidth >= 1 then
					thewidth = math.abs(1-noteFieldWidth)
				else
					thewidth = noteFieldWidth - 1
				end
				self:xy((xpos - (arrowWidth * (cols / 2) * getNoteFieldScale(pn)) + ((i - 1) * arrowWidth * getNoteFieldScale(pn)) +(getNoteFieldScale(pn) * arrowWidth / 2)) + (i-(cols/2)-(1/2))*colWidth*(thewidth),-receptor)
				self:fadebottom(0.6):fadetop(0.6)
				self:addx(notefieldX)
				self:visible(false)
			end,
			JudgmentMessageCommand=function(self,params)
				local notes = params.Notes
				local firstTrack = params.FirstTrack+1
				if params.HoldNoteScore then return end
				if params.Player == pn and params.TapNoteScore then
					local enum  = Enum.Reverse(TapNoteScore)[params.TapNoteScore]
					if enum < judgeThreshold and enum > 3 and i == firstTrack then
						self:stoptweening()
						self:visible(true)
						self:diffuse(byJudgment(params.TapNoteScore))
						self:diffusealpha(alpha)
						self:tween(0.25, "TweenType_Bezier",{0,0,0.5,0,1,1,1,1})
						self:diffusealpha(0)
					end
				end
			end,
			UpdateCommand = function(self)
				noteFieldWidth = MovableValues.NotefieldWidth
				self:zoomtowidth((colWidth-border) * noteFieldWidth)
				self:addx((i-(cols/2)-(1/2))*colWidth * (noteFieldWidth - oldWidth))
			end
		}
	end

	return r
end

local t =
	Def.ActorFrame {
	OnCommand = function()
		if (allowedCustomization) then
			SCREENMAN:GetTopScreen():AddInputCallback(input)
		end
	end
}

if numPlayers == 1 then
	local player = GAMESTATE:GetMasterPlayerNumber()
	local pNum = (player == PLAYER_1) and 1 or 2

	filterAlphas[player] = playerConfig:get_data(pn_to_profile_slot(player)).ScreenFilter
	if filterAlphas[player] == nil then
		filterAlphas[player] = 0
	else
		filterAlphas[player] = tonumber(filterAlphas[player])
	end

	local pos
	-- [ScreenGameplay] PlayerP#Player*Side(s)X
	if center1P then
		pos = SCREEN_CENTER_X
	else
		local metricName = string.format("PlayerP%i%sX", pNum, styleType)
		pos = THEME:GetMetric("ScreenGameplay", metricName)
	end
	t[#t + 1] =
		Def.Quad {
		Name = "SinglePlayerFilter",
		InitCommand = function(self)
			self:x(pos)
			self:CenterY()
			self:zoomto(filterWidth * getNoteFieldScale(player) * noteFieldWidth, SCREEN_HEIGHT)
			self:diffusecolor(filterColor)
			self:diffusealpha(filterAlphas[player])
			self:addx(notefieldX)
			filter = self
		end,
		UpdateCommand = function(self)
			local player = GAMESTATE:GetMasterPlayerNumber()
			noteFieldWidth = MovableValues.NotefieldWidth
			self:zoomto(filterWidth * getNoteFieldScale(player) * noteFieldWidth, SCREEN_HEIGHT)
		end
	}
	t[#t+1] = laneHighlight()
end

return t
