return Def.Sprite {
	Texture = NOTESKIN:GetPath("UpRightFoot", "Hold Head Active"),
	InitCommand = function(self)
		self:basezoomx(-1)
	end
}
