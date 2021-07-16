local prevZoom = 0.65
local musicratio = 1

-- hurrrrr nps quadzapalooza -mina
local wodth = capWideScale(get43size(240), 280)
local hidth = 40
local cd
local loopStartPos
local loopEndPos

local function handleRegionSetting(positionGiven)
	-- don't allow a negative region 
	-- internally it is limited to -2
	-- the start delay is 2 seconds, so limit this to 0
	if positionGiven < 0 then return end

	-- first time starting a region
	if not loopStartPos and not loopEndPos then
		loopStartPos = positionGiven
		MESSAGEMAN:Broadcast("RegionSet")
		return
	end

	-- reset region to bookmark only if double right click
	if positionGiven == loopStartPos or positionGiven == loopEndPos then
		loopEndPos = nil
		loopStartPos = positionGiven
		MESSAGEMAN:Broadcast("RegionSet")
		SCREENMAN:GetTopScreen():ResetLoopRegion()
		return
	end

	-- measure the difference of the new pos from each end
	local startDiff = math.abs(positionGiven - loopStartPos)
	local endDiff = startDiff + 0.1
	if loopEndPos then
		endDiff = math.abs(positionGiven - loopEndPos)
	end

	-- use the diff to figure out which end to move

	-- if there is no end, then you place the end
	if not loopEndPos then
		if loopStartPos < positionGiven then
			loopEndPos = positionGiven
		elseif loopStartPos > positionGiven then
			loopEndPos = loopStartPos
			loopStartPos = positionGiven
		else
			-- this should never happen
			-- but if it does, reset to bookmark
			loopEndPos = nil
			loopStartPos = positionGiven
			MESSAGEMAN:Broadcast("RegionSet")
			SCREENMAN:GetTopScreen():ResetLoopRegion()
			return
		end
	else
		-- closer to the start, move the start
		if startDiff < endDiff then
			loopStartPos = positionGiven
		else
			loopEndPos = positionGiven
		end
	end
	SCREENMAN:GetTopScreen():SetLoopRegion(loopStartPos, loopEndPos)
	MESSAGEMAN:Broadcast("RegionSet", {loopLength = loopEndPos-loopStartPos})
end

local function duminput(event)
	if event.type == "InputEventType_Release" then
		if event.DeviceInput.button == "DeviceButton_left mouse button" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
			MESSAGEMAN:Broadcast("MouseRightClick")
		end
	elseif event.type == "InputEventType_FirstPress" then
		if event.DeviceInput.button == "DeviceButton_backspace" then
			if loopStartPos ~= nil then
				SCREENMAN:GetTopScreen():SetSongPositionAndUnpause(loopStartPos, 1, true)
			end
		elseif event.button == "EffectUp" then
			SCREENMAN:GetTopScreen():AddToRate(0.05)
		elseif event.button == "EffectDown" then
			SCREENMAN:GetTopScreen():AddToRate(-0.05)
		elseif event.button == "Coin" then
			handleRegionSetting(SCREENMAN:GetTopScreen():GetSongPosition())
		elseif event.DeviceInput.button == "DeviceButton_mousewheel up" then
			if GAMESTATE:IsPaused() then
				local pos = SCREENMAN:GetTopScreen():GetSongPosition()
				local dir = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse() and 1 or -1
				local nextpos = pos + dir * 0.05
				if loopEndPos ~= nil and nextpos >= loopEndPos then
					handleRegionSetting(nextpos + 1)
				end
				SCREENMAN:GetTopScreen():SetSongPosition(nextpos, 0, false)
			end
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
			if GAMESTATE:IsPaused() then
				local pos = SCREENMAN:GetTopScreen():GetSongPosition()
				local dir = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse() and 1 or -1
				local nextpos = pos - dir * 0.05
				if loopEndPos ~= nil and nextpos >= loopEndPos then
					handleRegionSetting(nextpos + 1)
				end
				SCREENMAN:GetTopScreen():SetSongPosition(nextpos, 0, false)
			end
		end
	end
	
	return false
end

local function UpdatePreviewPos(self)
	local pos = SCREENMAN:GetTopScreen():GetSongPosition() / musicratio
	self:GetChild("Pos"):zoomto(math.min(math.max(0, pos), wodth), hidth)
	self:queuecommand("Highlight")
end

local t = Def.ActorFrame {
	Name = "ChartPreview",
	InitCommand = function(self)
		self:xy(MovableValues.PracticeCDGraphX, MovableValues.PracticeCDGraphY)
		self:SetUpdateFunction(UpdatePreviewPos)
		cd = self:GetChild("ChordDensityGraph"):visible(true):draworder(1000):y(20)
		--self:zoomto(MovableValues.PracticeCDGraphWidth, MovableValues.PracticeCDGraphHeight)
	end,
	BeginCommand = function(self)
		musicratio = GAMESTATE:GetCurrentSteps():GetLastSecond() / (wodth)
		SCREENMAN:GetTopScreen():AddInputCallback(duminput)
		cd:GetChild("cdbg"):diffusealpha(0)
		self:SortByDrawOrder()
		self:queuecommand("GraphUpdate")
	end,
	PracticeModeReloadMessageCommand = function(self)
		musicratio = GAMESTATE:GetCurrentSteps():GetLastSecond() / wodth
	end,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:x(wodth / 2)
			self:diffuse(color("0.05,0.05,0.05,1"))
		end
	},
	Def.Quad {
		Name = "PosBG",
		InitCommand = function(self)
			self:zoomto(wodth, hidth):halign(0):diffuse(color("1,1,1,1")):draworder(900)
		end,
		HighlightCommand = function(self) -- use the bg for detection but move the seek pointer -mina
			if isOver(self) then
				self:GetParent():GetChild("Seek"):visible(true)
				self:GetParent():GetChild("Seektext"):visible(true)
				self:GetParent():GetChild("Seek"):x(INPUTFILTER:GetMouseX() - self:GetParent():GetX())
				self:GetParent():GetChild("Seektext"):x(INPUTFILTER:GetMouseX() - self:GetParent():GetX() - 4) -- todo: refactor this lmao -mina
				self:GetParent():GetChild("Seektext"):y(INPUTFILTER:GetMouseY() - self:GetParent():GetY())
				self:GetParent():GetChild("Seektext"):settextf(
					"%0.2f",
					self:GetParent():GetChild("Seek"):GetX() * musicratio / getCurRateValue()
				)
			else
				self:GetParent():GetChild("Seektext"):visible(false)
				self:GetParent():GetChild("Seek"):visible(false)
			end
		end
	},
	Def.Quad {
		Name = "Pos",
		InitCommand = function(self)
			self:zoomto(0, hidth):diffuse(color("0,1,0,.5")):halign(0):draworder(900)
		end
	}
}

-- Load the CDGraph with a forced width parameter.
t[#t + 1] = LoadActorWithParams("../../chorddensitygraph.lua", {
	Width = wodth,
    Height = 53 / 555 * SCREEN_HEIGHT,
    NPSThickness = 1.5,
    TextSize = 0.45,
})

-- more draw order shenanigans
t[#t + 1] = LoadFont("Common Normal") .. {
	Name = "Seektext",
	InitCommand = function(self)
		self:y(8):valign(1):halign(1):draworder(1100):diffuse(color("0.8,0,0")):zoom(0.4)
	end
}

t[#t + 1] = Def.Quad {
	Name = "Seek",
	InitCommand = function(self)
		self:zoomto(2, hidth):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
	end,
	MouseLeftClickMessageCommand = function(self)
		if isOver(self) then
			local withCtrl = INPUTFILTER:IsControlPressed()
			if withCtrl then
				handleRegionSetting(self:GetX() * musicratio)
			else
				SCREENMAN:GetTopScreen():SetSongPosition(self:GetX() * musicratio, 0, false)
			end
		end
	end,
	MouseRightClickMessageCommand = function(self)
		if isOver(self) then
			handleRegionSetting(self:GetX() * musicratio)
		else
			if not (allowedCustomization) then
				SCREENMAN:GetTopScreen():TogglePause()
			end
		end
	end
}

t[#t + 1] = Def.Quad {
	Name = "BookmarkPos",
	InitCommand = function(self)
		self:zoomto(2, hidth):diffuse(color(".2,.5,1,1")):halign(0.5):draworder(1100)
		self:visible(false)
	end,
	SetCommand = function(self)
		self:visible(true)
		self:zoomto(2, hidth):diffuse(color(".2,.5,1,1")):halign(0.5)
		self:x(loopStartPos / musicratio)
	end,
	RegionSetMessageCommand = function(self, params)
		if not params or not params.loopLength then
			self:playcommand("Set")
		else
			self:visible(true)
			self:x(loopStartPos / musicratio):halign(0)
			self:zoomto(params.loopLength / musicratio, hidth):diffuse(color(".7,.2,.7,0.5"))
		end
	end,
	CurrentRateChangedMessageCommand = function(self)
		if not loopEndPos and loopStartPos then
			self:playcommand("Set")
		elseif loopEndPos and loopStartPos then
			self:playcommand("RegionSet", {loopLength = (loopEndPos - loopStartPos)})
		end
	end,
	PracticeModeReloadMessageCommand = function(self)
		self:playcommand("CurrentRateChanged")
	end
}

return t