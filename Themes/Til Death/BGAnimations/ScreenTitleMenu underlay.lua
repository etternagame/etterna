Req.using(func)


t = Def.ActorFrame{}
local frameX = THEME:GetMetric("ScreenTitleMenu","ScrollerX")-10
local frameY = THEME:GetMetric("ScreenTitleMenu","ScrollerY")

t[#t+1] = Def.Quad{
	BeginCommand=_1:draworder(-300):xy(frameX,frameY):zoomto(SCREEN_WIDTH,160):halign(0):diffuse(getMainColor('highlight')):diffusealpha(0.15):diffusetopedge(color("0,0,0,0"))
}

t[#t+1] = Def.Quad{
	BeginCommand=_1:draworder(-300):xy(frameX,frameY-100):zoomto(SCREEN_WIDTH,160):halign(0):diffuse(getMainColor('highlight')):diffusealpha(0.15):diffusebottomedge(color("0,0,0,0"))
}

t[#t+1] = LoadFont("Common Large") .. {
	BeginCommand=_1:xy(10,frameY-180):zoom(0.65):valign(1):halign(0):diffuse(getDifficultyColor("Difficulty_Couple")),
	InitCommand=_1:settext(getThemeName()),
}

t[#t+1] = LoadActor(THEME:GetPathG("","_ring")) .. {
	BeginCommand=_1:xy(capWideScale(get43size(SCREEN_WIDTH-10),SCREEN_WIDTH-530),frameY-130):diffuse(getDifficultyColor("Difficulty_Couple")):diffusealpha(1):baserotationx(0)
}
return t