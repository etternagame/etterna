-- meant to define the inward screen transition for a screen
-- this is a full screen fade out of black (0.1 seconds long) after waiting 0.1 seconds
return Def.Quad {
	InitCommand = function(self)
		self:FullScreen():diffuse(color("#000000FF"))
	end,
	OnCommand = function(self)
		self:sleep(0.1):linear(0.1):diffusealpha(0)
	end
}
