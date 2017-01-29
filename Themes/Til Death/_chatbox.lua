
local top
local whee

local border = 5

local inputX = THEME:GetMetric("ScreenNetSelectBase","ChatInputX")
local inputY = THEME:GetMetric("ScreenNetSelectBase","ChatInputY")
local inputWidth = THEME:GetMetric("ScreenNetSelectBase","ChatTextInputWidth")*0.4
local inputHeight = 25


local outputX = THEME:GetMetric("ScreenNetSelectBase","ChatOutputX")
local outputY = THEME:GetMetric("ScreenNetSelectBase","ChatOutputY")
local outputWidth = THEME:GetMetric("ScreenNetSelectBase","ChatTextOutputWidth")*0.3153
local outputHeight = THEME:GetMetric("ScreenNetSelectBase","ChatOutputLines")*9.25

if IsUsingWideScreen() == true then

local border = 5

local inputX = THEME:GetMetric("ScreenNetSelectBase","ChatInputX")
local inputY = THEME:GetMetric("ScreenNetSelectBase","ChatInputY")
local inputWidth = THEME:GetMetric("ScreenNetSelectBase","ChatTextInputWidth")*0.4
local inputHeight = 25


local outputX = THEME:GetMetric("ScreenNetSelectBase","ChatOutputX")
local outputY = THEME:GetMetric("ScreenNetSelectBase","ChatOutputY")
local outputWidth = THEME:GetMetric("ScreenNetSelectBase","ChatTextOutputWidth")*0.3153
local outputHeight = THEME:GetMetric("ScreenNetSelectBase","ChatOutputLines")*9.25

end

local function isOverChatbox()
	if INPUTFILTER:GetMouseX() > outputX-border and INPUTFILTER:GetMouseX() < outputX + outputWidth+border*2 and INPUTFILTER:GetMouseY() > outputY-border and INPUTFILTER:GetMouseY() < outputY + outputHeight+border*2 then
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
	elseif event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" then
		if isOverChatbox() then
			top:ScrollChatUp()
		else
			moving = true
			if pressingtab == true then
				whee:Move(-3)	
			else
				whee:Move(-1)	
			end
		end
	elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" then
		if isOverChatbox() then
			top:ScrollChatDown()
		else
			moving = true
			if pressingtab == true then
				whee:Move(3)	
			else
				whee:Move(1)	
			end
		end
	elseif moving == true then
		whee:Move(0)
		moving = false
	end
end


local t = Def.ActorFrame{
	BeginCommand=function(self)
		top = SCREENMAN:GetTopScreen()
		whee = top:GetMusicWheel()
		top:AddInputCallback(scrollInput)
	end,
}

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,inputX-border,inputY-border;zoomto,outputWidth+border*2,inputHeight+border*2;halign,0;valign,0;diffuse,color("#00000099");),
	TabChangedMessageCommand=function(self)
		local top= SCREENMAN:GetTopScreen()
		if getTabIndex() == 0 then
			self:visible(true)
		else 
			self:visible(false)
		end
	end,
	}
t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,outputX-border,outputY-border;zoomto,outputWidth+border*2,outputHeight+border*2;halign,0;valign,0;diffuse,color("#00000099");),
	TabChangedMessageCommand=function(self)
		local top= SCREENMAN:GetTopScreen()
		if getTabIndex() == 0 then
			self:visible(true)
		else 
			self:visible(false)
		end
	end,
}

return t