local t = Def.ActorFrame{}

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,SCREEN_WIDTH,0;halign,1;valign,0;zoomto,capWideScale(get43size(350),350),SCREEN_HEIGHT;diffuse,color("#33333399"));
};

t[#t+1] = Def.Quad{
	InitCommand=cmd(xy,SCREEN_WIDTH-capWideScale(get43size(350),350),0;halign,0;valign,0;zoomto,4,SCREEN_HEIGHT;diffuse,color("#FFFFFF"));
};

return t