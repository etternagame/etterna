local selected = getTableKeys()
local themeColor = colorConfig:get_data()[selected[1]][selected[2]]
local colorTable = {}
for i = 2, #themeColor do -- First char in string is a "#", ignore.
	colorTable[i - 1] = themeColor:sub(i, i)
end

local translated_info = {
	Title = THEME:GetString("ScreenColorEdit", "Title"),
	Description = THEME:GetString("ScreenColorEdit", "Description")
}

local function getRotationZ(self)
	local parent = self:GetParent()
	if parent == nil then
		return self:GetRotationZ()
	else
		return self:GetRotationZ() + getRotationZ(parent)
	end
end

local function localMousePos(self, mx, my)
	local rz = math.rad(-getRotationZ(self))
	local x = mx - self:GetTrueX()
	local y = my - self:GetTrueY()
	return x * math.cos(rz) - y * math.sin(rz), x * math.sin(rz) + y * math.cos(rz)
end

local function posToColor(x, y)
end

local function inputeater(event)
	if event.type == "InputEventType_FirstPress" then
		if event.DeviceInput.button == "DeviceButton_left mouse button" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		end
	end
end

local t = Def.ActorFrame {
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(inputeater)
	end,
	CodeMessageCommand = function(self, params)
		if params.Name == "ColorCancel" then
			SCREENMAN:GetTopScreen():Cancel()
		end
		if params.Name == "ColorStart" then
			SCREENMAN:GetTopScreen():Cancel()
		end
	end,
	Def.Quad {
		Name = "MainBG",
		InitCommand = function(self)
			self:xy(0, 0):halign(0):valign(0):zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):diffuse(color("#000000"))
			self:diffusealpha(0.9)
		end
	}
}

local colorBoxHeight = 250
local saturationSliderWidth = 25
local saturationOverlay = nil
local saturationSliderPos = nil
local colorPickPosition = nil
local colorPreview = nil

local satNum = 0 -- saturation percent
local hueNum = 0 -- degrees 0-360 exclusive
local valNum = 0 -- brightness percent

local function applyHSV()
	local newColor = HSV(hueNum, 1 - satNum, 1 - valNum)

	colorPreview:diffuse(newColor)
end

local function updateSaturation(percent)
	if percent < 0 then percent = 0 elseif percent > 1 then percent = 1 end

	saturationOverlay:diffusealpha(percent)
	satNum = percent
	-- update numbers and stuff
	applyHSV()
end

local function updateColor(percentX, percentY)
	if percentY < 0 then percentY = 0 elseif percentY > 1 then percentY = 1 end
	if percentX < 0 then percentX = 0 elseif percentX > 1 then percentX = 1 end

	colorPickPosition:xy(saturationSliderWidth + (colorBoxHeight * percentX), colorBoxHeight * percentY)

	hueNum = 360 * percentX
	valNum = percentY
	applyHSV()
end

t[#t+1] = Def.ActorFrame {
	Name = "ColorPickEquipment",
	InitCommand = function(self)
		self:xy(SCREEN_WIDTH / 12, SCREEN_HEIGHT / 8)
	end,

	Def.Sprite {
		Name = "HSVImage",
		Texture = THEME:GetPathG("", "color_hsv"),
		InitCommand = function(self)
			self:zoomto(colorBoxHeight, colorBoxHeight)
			self:x(saturationSliderWidth)
			self:valign(0):halign(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				local y = INPUTFILTER:GetMouseY()
				local x = INPUTFILTER:GetMouseX()
				local relX, relY = localMousePos(self, x, y)
				updateColor(relX / colorBoxHeight, relY / colorBoxHeight)
			end
		end
	},
	Def.Sprite {
		Name = "SaturationOverlay",
		Texture = THEME:GetPathG("", "color_sat_overlay"),
		InitCommand = function(self)
			self:zoomto(colorBoxHeight, colorBoxHeight)
			self:x(saturationSliderWidth)
			self:diffusealpha(0.0)
			self:valign(0):halign(0)
			saturationOverlay = self
		end,
	},
	Def.Quad {
		Name = "SaturationSlider",
		InitCommand = function(self)
			self:zoomto(saturationSliderWidth, colorBoxHeight)
			self:valign(0):halign(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				local y = INPUTFILTER:GetMouseY()
				local x = INPUTFILTER:GetMouseX()
				local relX, relY = localMousePos(self, x, y)
				updateSaturation(relY / colorBoxHeight)
				saturationSliderPos:y(relY)
			end
		end
	},
	Def.Quad {
		Name = "SaturationSliderPos",
		InitCommand = function(self)
			self:diffuse(color("0,0,0"))
			self:zoomto(saturationSliderWidth, 2)
			self:xy(0,0)
			self:valign(0):halign(0)
			saturationSliderPos = self
		end,
	},
	Def.Sprite {
		Name = "ColorPickPosition",
		Texture = THEME:GetPathG("", "_thick circle"),
		InitCommand = function(self)
			self:diffuse(color("1,1,1,.8"))
			self:zoomto(7,7)
			self:x(saturationSliderWidth)
			colorPickPosition = self
		end
	},
	Def.Quad {
		Name = "PickedColorPreview",
		InitCommand = function(self)
			self:zoomto(colorBoxHeight/4, colorBoxHeight/4)
			self:x(colorBoxHeight + saturationSliderWidth)
			self:valign(0):halign(0)
			colorPreview = self
		end,
	}
}

t[#t + 1] = LoadActor("_frame")

t[#t + 1] = LoadFont("Common Large") .. {
	Name = "ScreenTitleText",
	InitCommand = function(self)
		self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("highlight")):settext(translated_info["Title"])
	end
}

t[#t+1] = LoadActor("_cursor")

return t