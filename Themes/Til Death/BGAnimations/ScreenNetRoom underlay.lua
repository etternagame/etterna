local t = Def.ActorFrame{}

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,SCREEN_WIDTH,0;halign,1;valign,0;zoomto,capWideScale(get43size(350),350),SCREEN_HEIGHT;diffuse,color("#33333399"));
};


-- what is this supposed to be? - mina
t[#t+1] = Def.Quad{
	InitCommand=cmd(draworder,-300;xy,10,300-100;zoomto,SCREEN_WIDTH,160;halign,0;diffuse,getMainColor('highlight');diffusealpha,0.15;diffusebottomedge,color("0,0,0,0"))
}

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,SCREEN_WIDTH-capWideScale(get43size(350),350),0;halign,0;valign,0;zoomto,4,SCREEN_HEIGHT;diffuse,getMainColor('highlight'));
};

return t
