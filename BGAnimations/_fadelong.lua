-- meant to define the outward screen transition for a screen
-- this is a full screen fade for 2 seconds into black after waiting 3 seconds
return Def.Quad {
	InitCommand = function(self)
		self:FullScreen():diffuse(color("#00000000"))
	end,
	OnCommand = function(self)
		self:sleep(3):linear(2):diffusealpha(1)
	end
}
