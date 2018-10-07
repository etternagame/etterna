local top
local whee

local border = 5

local inputX = THEME:GetMetric("ScreenNetSelectBase", "ChatInputX")
local inputY = THEME:GetMetric("ScreenNetSelectBase", "ChatInputY")
local inputWidth = THEME:GetMetric("ScreenNetSelectBase", "ChatTextInputWidth") * 0.4
local inputHeight = 25

local outputX = THEME:GetMetric("ScreenNetSelectBase", "ChatOutputX")
local outputY = THEME:GetMetric("ScreenNetSelectBase", "ChatOutputY")
local outputWidth = THEME:GetMetric("ScreenNetSelectBase", "ChatTextOutputWidth") * 0.3153
local outputHeight = THEME:GetMetric("ScreenNetSelectBase", "ChatOutputLines") * 9.25

if IsUsingWideScreen() == true then
	local border = 5

	local inputX = THEME:GetMetric("ScreenNetSelectBase", "ChatInputX")
	local inputY = THEME:GetMetric("ScreenNetSelectBase", "ChatInputY")
	local inputWidth = THEME:GetMetric("ScreenNetSelectBase", "ChatTextInputWidth") * 0.4
	local inputHeight = 25

	local outputX = THEME:GetMetric("ScreenNetSelectBase", "ChatOutputX")
	local outputY = THEME:GetMetric("ScreenNetSelectBase", "ChatOutputY")
	local outputWidth = THEME:GetMetric("ScreenNetSelectBase", "ChatTextOutputWidth") * 0.3153
	local outputHeight = THEME:GetMetric("ScreenNetSelectBase", "ChatOutputLines") * 9.25
end

local function isOverChatbox()
	if
		INPUTFILTER:GetMouseX() > outputX - border and INPUTFILTER:GetMouseX() < outputX + outputWidth + border * 2 and
			INPUTFILTER:GetMouseY() > outputY - border and
			INPUTFILTER:GetMouseY() < outputY + outputHeight + border * 2
	 then
		return true
	end
	return false
end

local function scrollInput(event)
	if event.DeviceInput.button == "DeviceButton_tab" then
		if event.type == "InputEventType_FirstPress" then
			local pressingtab = true
		elseif event.type == "InputEventType_Release" then
			local pressingtab = false
		end
	elseif event.DeviceInput.button == "DeviceButton_enter" then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("Scroll")
		end
	elseif event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" then
		if isOverChatbox() then
			top:ScrollChatUp()
			MESSAGEMAN:Broadcast("Scroll")
		else
			moving = true
			if pressingtab == true then
				whee:Move(-2)
			else
				whee:Move(-1)
			end
		end
	elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" then
		if isOverChatbox() then
			top:ScrollChatDown()
			MESSAGEMAN:Broadcast("Scroll")
		else
			moving = true
			if pressingtab == true then
				whee:Move(2)
			else
				whee:Move(1)
			end
		end
	elseif moving == true then
		whee:Move(0)
		moving = false
	end
end

local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		top = SCREENMAN:GetTopScreen()
		whee = top:GetMusicWheel()
		top:AddInputCallback(scrollInput)
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(inputX - border, inputY - border):zoomto(outputWidth + border * 2, inputHeight + border * 2):halign(0):valign(
			0
		):diffuse(color("#00000099"))
	end,
	TabChangedMessageCommand = function(self)
		local top = SCREENMAN:GetTopScreen()
		if getTabIndex() == 0 then
			self:visible(true)
		else
			self:visible(false)
		end
	end
}
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(outputX - border, outputY - border):zoomto(outputWidth + border * 2, outputHeight + border * 2):halign(0):valign(
			0
		):diffuse(color("#00000099"))
	end,
	TabChangedMessageCommand = function(self)
		local top = SCREENMAN:GetTopScreen()
		if getTabIndex() == 0 then
			self:visible(true)
		else
			self:visible(false)
		end
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(outputX + outputWidth - 1, outputHeight):zoomto(border, outputHeight):halign(0):valign(0):diffuse(
			getMainColor("highlight")
		):queuecommand("Set")
	end,
	SetCommand = function(self)
		if getTabIndex() == 0 then
			local lineqty = top:GetChatLines()
			local scroll = top:GetChatScroll()
			self:visible(true)
			if lineqty >= THEME:GetMetric("ScreenNetSelectBase", "ChatOutputLines") then
				local newheight = outputHeight / (lineqty / THEME:GetMetric("ScreenNetSelectBase", "ChatOutputLines"))
				self:zoomto(border, newheight)
				self:y(outputY + outputHeight - newheight - scroll * outputHeight / lineqty)
			else
				self:zoomto(border, outputHeight)
				self:y(outputY)
			end
		else
			self:visible(false)
		end
	end,
	ScrollMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

return t
