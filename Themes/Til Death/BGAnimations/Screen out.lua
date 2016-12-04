return Def.Quad {
	InitCommand=cmd(xy,SCREEN_CENTER_X,SCREEN_CENTER_Y;zoomto,SCREEN_WIDTH,SCREEN_HEIGHT);
	OnCommand=cmd(diffuse,color("0,0,0,0");sleep,0.1;linear,0.1;diffusealpha,1);
};