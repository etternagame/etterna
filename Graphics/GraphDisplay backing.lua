-- represents the background for the entire life graph

return Def.Quad {
	Name = "Backing",
	InitCommand = function(self)
		self:diffuse(color("#999999"))
		self:diffusealpha(0.7)
		self:diffusebottomedge(color("#00000033"))
	end
}
