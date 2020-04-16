--[[ Screen Filter ]]
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local padding = 20 -- 10px on each side
local arrowWidth = 64 -- until noteskin metrics are implemented...

--moving notefield shenanigans
local rPressed = false
local tPressed = false
local noteFieldWidth = MovableValues.NotefieldWidth
local notefieldX = MovableValues.NotefieldX
local oldWidth = noteFieldWidth
local filter
local cbContainer

-- this happens when not in gameplay
if noteFieldWidth == nil then
	return Def.ActorFrame {}
end

local function input(event)
	if getAutoplay() ~= 0 then -- not touching this currently, its fully bound with the notefield ones and doesnt have a message
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
	local width = style:GetWidth(pn)
	local colWidth = width/cols
	local border = 4
	
	if not enabled then
		return r
	end
	
	for i=1,cols do
		r[#r+1] = Def.Quad{
			InitCommand = function(self)
				self:zoomto(getNoteFieldScale(pn) * (arrowWidth - 4) * noteFieldWidth, SCREEN_HEIGHT * 2)
				
				local reverse = GAMESTATE:GetPlayerState(pn):GetCurrentPlayerOptions():UsingReverse()
				local receptor = reverse and THEME:GetMetric("Player", "ReceptorArrowsYStandard") or THEME:GetMetric("Player", "ReceptorArrowsYReverse")

				self:diffusealpha(alpha)
				local thewidth
				if noteFieldWidth >= 1 then
					thewidth = math.abs(1-noteFieldWidth)
				else
					thewidth = noteFieldWidth - 1
				end
				self:xy((-(arrowWidth * (cols / 2) * getNoteFieldScale(pn)) + ((i - 1) * arrowWidth * getNoteFieldScale(pn)) + (getNoteFieldScale(pn) * arrowWidth / 2)) + (i-(cols/2)-(1/2))*colWidth*(thewidth),-receptor)
				self:fadebottom(0.6):fadetop(0.6)
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
			-- this isnt really necessary but it stops lua errors
			-- basically this just triggers if we exit playeroptions into gameplay
			-- and i dont want to investigate what causes it
			-- but it doesnt break anything when it happens
			-- so this just stops the lua error and breaks nothing else
			if SCREENMAN:GetTopScreen() == nil then return end
			SCREENMAN:GetTopScreen():AddInputCallback(input)
		end
	end
}

local filterColor = color("0,0,0,0")
local filterAlphas = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ScreenFilter
if filterAlphas == nil then
	filterAlphas = 0
else
	filterAlphas = tonumber(filterAlphas)
end

t[#t + 1] =
	Def.Quad {
	Name = "SinglePlayerFilter",
	InitCommand = function(self)
		self:zoomto(filterWidth * getNoteFieldScale(PLAYER_1) * noteFieldWidth, SCREEN_HEIGHT * 2)
		self:diffusecolor(filterColor)
		self:diffusealpha(filterAlphas)
		filter = self
	end,
	UpdateCommand = function(self)
		noteFieldWidth = MovableValues.NotefieldWidth
		self:zoomto(filterWidth * getNoteFieldScale(PLAYER_1) * noteFieldWidth, SCREEN_HEIGHT * 2)
	end
}

t[#t+1] = laneHighlight()

return t
