local t = Def.ActorFrame {}
-- Controls the children of the StepsDisplayListRow if loaded anywhere

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:zoomto(22, 20):diffuse(color("#ffffff")):diffusealpha(0.7)
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:x(14):zoomto(40, 20):diffuse(color("#ffffff")):diffusealpha(0.5):halign(0)
	end
}

return t
