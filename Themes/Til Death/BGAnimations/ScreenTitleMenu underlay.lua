t = Def.ActorFrame{}

local frameX = THEME:GetMetric("ScreenTitleMenu","ScrollerX")-10
local frameY = THEME:GetMetric("ScreenTitleMenu","ScrollerY")

t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:draworder(-300):xy(frameX,frameY):zoomto(SCREEN_WIDTH,160):halign(0):diffuse(getMainColor('highlight')):diffusealpha(0.15):diffusetopedge(color("0,0,0,0"))
	end	
}

t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:draworder(-300):xy(frameX,frameY-100):zoomto(SCREEN_WIDTH,160):halign(0):diffuse(getMainColor('highlight')):diffusealpha(0.15):diffusebottomedge(color("0,0,0,0"))
	end	
}

t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(10,frameY-180):zoom(0.65):valign(1):halign(0):diffuse(getDifficultyColor("Difficulty_Couple"))
	end,
	OnCommand=function(self)
		self:settext(getThemeName())
	end,
}

t[#t+1] = LoadActor(THEME:GetPathG("","_ring")) .. {
	InitCommand=function(self)
		self:xy(capWideScale(get43size(SCREEN_WIDTH-10),SCREEN_WIDTH-530),frameY-130):diffuse(getDifficultyColor("Difficulty_Couple")):diffusealpha(1):baserotationx(420)
	end	
}
return t