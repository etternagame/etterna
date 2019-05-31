local t = Def.ActorFrame {}

--Left gray rectangle
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(0, 0):halign(0):valign(0):zoomto(250, 900):diffuse(getTitleColor("BG_Left")):diffusealpha(1)
	end
}

--Right gray rectangle
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(250, 0):halign(0):valign(0):zoomto(1000, 900):diffuse(getTitleColor("BG_Right")):diffusealpha(1)
	end
}

--Light purple line
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(250, 0):halign(0):valign(0):zoomto(10, 900):diffuse(getTitleColor("Line_Left")):diffusealpha(1)
	end
}

--Dark purple line
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(260, 0):halign(0):valign(0):zoomto(10, 900):diffuse(getTitleColor("Line_Right")):diffusealpha(1)
	end
}

return t
