local t = Def.ActorFrame {
	Def.Sprite {
		Texture = NOTESKIN:GetPath("", "lift")

		Frame0000 = 0,
		Delay0000 = 1,

		InitCommand = function(self)
			self:zoomx(0.5)
		end
	}
}

return t
