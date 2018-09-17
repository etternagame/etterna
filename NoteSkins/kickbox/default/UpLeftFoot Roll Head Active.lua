return Def.Sprite {
	Texture = NOTESKIN:GetPath("UpRightFoot", "Roll Head Active"),
	InitCommand = function(self)
		self:basezoomx(-1)
	end
}
