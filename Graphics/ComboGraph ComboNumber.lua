return LoadFont("Common normal") .. {
	Name = "Numbers",
	InitCommand = function(self)
		self:zoom(0.4):diffuse(color("#FFFFFF"))
	end
}
