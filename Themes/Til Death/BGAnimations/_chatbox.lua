local t = Def.ActorFrame{}

local border = 5

local inputX = THEME:GetMetric("ScreenNetSelectBase","ChatInputX")
local inputY = THEME:GetMetric("ScreenNetSelectBase","ChatInputY")
local inputWidth = THEME:GetMetric("ScreenNetSelectBase","ChatTextInputWidth")*0.5
local inputHeight = 10


local outputX = THEME:GetMetric("ScreenNetSelectBase","ChatOutputX")
local outputY = THEME:GetMetric("ScreenNetSelectBase","ChatOutputY")
local outputWidth = THEME:GetMetric("ScreenNetSelectBase","ChatTextOutputWidth")*0.5
local outputHeight = THEME:GetMetric("ScreenNetSelectBase","ChatOutputLines")*16


t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,inputX-border,inputY-border;zoomto,inputWidth+border*2,inputHeight+border*2;halign,0;valign,0;diffuse,color("#00000099"););	
}

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,outputX-border,outputY-border;zoomto,outputWidth+border*2,outputHeight+border*2;halign,0;valign,0;diffuse,color("#00000099"););	
}

return t