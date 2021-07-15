local t = Def.ActorFrame{ }

t[#t + 1] = LoadActor(THEME:GetPathG("", "_OptionsScreen")) .. {
	OnCommand = function(self)
		self:FullScreen():zoom(1):visible(0):diffusealpha(0)
		if not inScreenSelectMusic then
			self:visible(1):decelerate(0.1):diffusealpha(1)
		end
	end
}

t[#t+1] = Def.Quad {
	InitCommand = function(self)
		self:zoomto(SCREEN_WIDTH-250,90):Center():addy(-8)
		self:diffuse(0,0,0,0)
	end,
	OnCommand = function(self)
		if inScreenSelectMusic then
			self:decelerate(0.1):diffusealpha(0.8)
		end
	end,
	OffCommand = function(self)
		self:decelerate(0.1):diffusealpha(0)
	end
}

return t
