return Def.Sprite {
	Texture = NOTESKIN:GetPath("_center", "explosion"),
	InitCommand = function(self)
		self:blend("BlendMode_Add"):finishtweening():diffusealpha(0.2):zoom(0.6):linear(0.1):diffusealpha(0):zoom(0.8)
	end
}
