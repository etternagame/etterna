t = Def.ActorFrame{}


t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:zoomto(22,22):diffuse(color("#ffffff")):diffusealpha(0.7)
	end;
};

t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:x(22):zoomto(66,22):diffuse(color("#ffffff")):diffusealpha(0.5)
	end;
};



return t