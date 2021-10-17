-- meant to define the outward screen transition for a screen
-- this is a full screen fade into black (2 seconds long)
return Def.Quad {
	InitCommand = function(self)
		self:FullScreen():diffuse(color("#00000000"))
	end,
	OnCommand = function(self)
		self:linear(2):diffusealpha(1)
	end
}
