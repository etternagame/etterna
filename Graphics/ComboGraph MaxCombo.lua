-- represents the quad drawn for the largest combo on the combo graph

return Def.Quad {
	Name = "MaxCombo",
	InitCommand = function(self)
		self:diffusealpha(0.8)
		registerActorToColorConfigElement(self, "evaluation", "MaxComboText")
	end
}
