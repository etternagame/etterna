t = Def.ActorFrame{}

local frameX = THEME:GetMetric("ScreenTitleMenu","ScrollerX")-10
local frameY = THEME:GetMetric("ScreenTitleMenu","ScrollerY")

t[#t+1] = Def.Quad{
	InitCommand=cmd(draworder,-300;xy,frameX,frameY;zoomto,SCREEN_WIDTH,160;halign,0;diffuse,getMainColor('highlight');diffusealpha,0.15;diffusetopedge,color("0,0,0,0"))
}

t[#t+1] = Def.Quad{
	InitCommand=cmd(draworder,-300;xy,frameX,frameY-100;zoomto,SCREEN_WIDTH,160;halign,0;diffuse,getMainColor('highlight');diffusealpha,0.15;diffusebottomedge,color("0,0,0,0"))
}

t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=cmd(xy,10,frameY-180;zoom,0.65;valign,1;halign,0;diffuse,getDifficultyColor("Difficulty_Couple")),
	OnCommand=function(self)
		self:settext(getThemeName())
	end,
}

t[#t+1] = LoadFont("Common normal") .. {
	InitCommand=cmd(xy,SCREEN_WIDTH-10,frameY-180;zoom,0.5;valign,1;halign,1;diffuse,getMainColor('highlight')),
	OnCommand=function(self)
		self:settext(getThemeVersion())
	end,
}

t[#t+1] = LoadActor(THEME:GetPathG("","_ring")) .. {
	InitCommand=cmd(xy,capWideScale(get43size(SCREEN_WIDTH-10),SCREEN_WIDTH-256),frameY-180;diffuse,getDifficultyColor("Difficulty_Couple")diffusealpha,1;baserotationx,420)
}
return t