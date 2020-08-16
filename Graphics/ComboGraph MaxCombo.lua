-- represents the quad drawn for the largest combo on the combo graph

return Def.Quad {
	Name = "MaxCombo",
	InitCommand = function(self)
		self:diffuse(color("1,1,1,1"))
		self:diffusetopedge(color("0,0,1,1"))
	end
}
