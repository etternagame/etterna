local active = true

local CtrlPressed = false
local function input(event)
	if event.type ~= "InputEventType_Release" and active then
		for i = 1, (NSMAN:IsETTP() and 2 or 3) do
			if event.DeviceInput.button == "DeviceButton_" .. i and CtrlPressed == true then
				local tind = getTabIndex()
				setTabIndex(i - 1)
				MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = i-1})
			end
		end
	end
	if event.DeviceInput.button == "DeviceButton_left ctrl" then
		if event.type == "InputEventType_Release" then
			CtrlPressed = false
		else
			CtrlPressed = true
		end
	end
	if event.DeviceInput.button == "DeviceButton_right ctrl" then
		if event.type == "InputEventType_Release" then
			CtrlPressed = false
		else
			CtrlPressed = true
		end
	end
	return false
end

local t = Def.ActorFrame {
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	BeginCommand = function(self)
		resetTabIndex()
	end,
	PlayerJoinedMessageCommand = function(self)
		resetTabIndex()
	end,
	BeginningSearchMessageCommand = function(self)
		active = true
	end,
	EndingSearchMessageCommand = function(self)
		active = true
	end
}

-- Just for debug
--[[
t[#t+1] = LoadFont("Common Normal") .. {
	InitCommand=function(self)
		self:xy(300,300):halign(0):zoom(2):diffuse(getMainColor(2))
	end;
	BeginCommand=function(self)
		self:queuecommand("Set")
	end;
	SetCommand=function(self)
		self:settext(getTabIndex())
	end;
	CodeMessageCommand=function(self)
		self:queuecommand("Set")
	end;
};
--]]
--======================================================================================

local tabNames
if NSMAN:IsETTP() then
	tabNames = {"Search", "Profile"}
else
	tabNames = {"Chatbox", "Search", "Profile"}
end

local frameWidth = (SCREEN_WIDTH * (7 / 24)) / (#tabNames - 1)
local frameX = frameWidth / 2
local frameY = SCREEN_HEIGHT - 70

local function tabs(index)
	local t = Def.ActorFrame {
		Name = "Tab" .. index,
		InitCommand = function(self)
			self:xy(frameX + ((index - 1) * frameWidth), frameY)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:finishtweening()
			self:smooth(0.1)
			--show tab if it's the currently selected one
			if getTabIndex() == index - 1 then
				self:diffusealpha(1):y(frameY - 1)
				self:GetChild("TabBG"):diffusecolor(Brightness(getMainColor("positive"),0.3)):diffusealpha(0.5)
			else -- otherwise "Hide" them
				self:diffusealpha(0.7):y(frameY)
				self:GetChild("TabBG"):diffusecolor(getMainColor("frames")):diffusealpha(0.7)
			end
		end,
		TabChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		PlayerJoinedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

	t[#t + 1] = UIElements.QuadButton(1, 1) .. {
		Name = "TabBG",
		InitCommand = function(self)
			self:y(2):valign(0):zoomto(frameWidth, 20):diffusecolor(getMainColor("frames")):diffusealpha(0.7)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				local tind = getTabIndex()
				setTabIndex(index - 1)
				MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = index-1})
			end
		end
	}

	t[#t + 1] = LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:y(4):valign(0):zoom(0.45):diffuse(getMainColor("positive"))
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settext(THEME:GetString("TabNames", tabNames[index]))
			if isTabEnabled(index) then
				self:diffuse(getMainColor("positive"))
			else
				self:diffuse(color("#666666"))
			end
		end,
		PlayerJoinedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}
	return t
end

--Make tabs
for i = 1, #tabNames do
	t[#t + 1] = tabs(i)
end

return t
