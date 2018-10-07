return Def.Quad {
	InitCommand = function(self)
		self:FullScreen():diffuse(color("#00000000"))
	end,
	OnCommand = function(self)
		self:linear(2):diffusealpha(1)
	end
}
