-- represents the background for the entire life graph

return Def.Quad {
	Name = "Backing",
	InitCommand = function(self)
		self:diffuse(color("0.3,0.3,0.3,1"))
	end
}
