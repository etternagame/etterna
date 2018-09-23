return Def.Sprite {
	Texture = NOTESKIN:GetPath("AnyRightFoot", "Hold Head Active"),
	InitCommand = function(self)
		self:basezoomx(-1):diffuse(color("0.5,0.5,0.5,1"))
	end
}
