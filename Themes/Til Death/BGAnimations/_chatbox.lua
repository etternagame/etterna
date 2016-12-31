local t = Def.ActorFrame{}

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