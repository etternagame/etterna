return Def.ActorFrame {
	Def.Quad {
		Name = "Horizontal",
		InitCommand = function(self)
			self:xy(0, -2):zoomto(854, 34):halign(0)
		end,
		SetCommand = function(self)
			self:diffuseramp()
			self:effectcolor1(color("#FFFFFF33"))
			self:effectcolor2(color("#FFFFFF33"))
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		OffCommand = function(self)
			self:visible(false)
		end
	}
}
