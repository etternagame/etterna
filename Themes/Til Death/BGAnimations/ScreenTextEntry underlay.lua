local t = Def.ActorFrame{ }

t[#t+1] = Def.Quad{
	InitCommand = function(self)
		self:FullScreen():Center():diffuse(color("0,0,0,0.5")):diffusealpha(1)
		self:zoomto(SCREEN_WIDTH-250,90):Center():addy(-8)
		self:diffuse(0,0,0,0)
	end,
	OnCommand = function(self)
		self:decelerate(0.1):diffusealpha(0.8)
	end,
	OffCommand = function(self)
		self:decelerate(0.1):diffusealpha(0)
	end
}

return t
