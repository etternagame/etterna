local flashColor = color(Var "Color1")
return Def.Quad {
	InitCommand = function(self)
		self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y):scaletoclipped(SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2):diffuse(flashColor)
	end,
	GainFocusCommand = function(self)
		self:finishtweening():diffusealpha(1):accelerate(0.6):diffusealpha(0)
	end
}
