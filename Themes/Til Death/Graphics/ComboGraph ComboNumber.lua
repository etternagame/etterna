return LoadFont("Common normal") .. {
	InitCommand=function(self)
		self:zoom(0.4):diffuse(color("#FFFFFF"))
	end;
};