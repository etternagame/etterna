-- represents the quad drawn for all combos that are not the max combo on the combo graph

return Def.Quad {
	InitCommand = function(self)
		self:diffuse(color("1,1,1,0.7"))
	end
}
