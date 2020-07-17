-- Controls the dot next to the MusicWheel

return Def.ActorFrame {
	Def.Quad {
		Name = "Horizontal",
		InitCommand = function(self)
			self:xy(8, -2):zoomto(3, 3):halign(0)
		end,
		SetCommand = function(self)
			self:diffuse(color("#FFFFFF"))
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		OffCommand = function(self)
			self:visible(false)
		end
	}
}
