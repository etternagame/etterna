-- represents the quad drawn for all combos that are not the max combo on the combo graph

return Def.Quad {
	InitCommand = function(self)
		self:diffusealpha(0.7)
		registerActorToColorConfigElement(self, "evaluation", "NormalComboText")
	end
}
