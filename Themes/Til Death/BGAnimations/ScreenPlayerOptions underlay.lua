t = Def.ActorFrame {};

local frameX = THEME:GetMetric("ScreenTitleMenu","ScrollerX")-10
local frameY = THEME:GetMetric("ScreenTitleMenu","ScrollerY")

t[#t+1] = LoadActor(THEME:GetPathG("","_OptionsScreen")) .. {
	InitCommand=function(self)
		self:xy(capWideScale(get43size(SCREEN_WIDTH-185),SCREEN_WIDTH-425),frameY-70):zoom(0.8):diffusealpha(1):baserotationx(0)
	end	
}
return t