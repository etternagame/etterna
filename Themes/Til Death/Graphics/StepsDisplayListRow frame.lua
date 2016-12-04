t = Def.ActorFrame{}


t[#t+1] = Def.Quad{
	InitCommand=cmd(zoomto,22,22;diffuse,color("#ffffff");diffusealpha,0.7);
};

t[#t+1] = Def.Quad{
	InitCommand=cmd(x,22;zoomto,66,22;diffuse,color("#ffffff");diffusealpha,0.5);
};



return t