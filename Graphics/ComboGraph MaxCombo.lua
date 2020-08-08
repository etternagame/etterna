return Def.Quad {
	Name = "MaxCombo",
	InitCommand = function(self)
		self:setsize(1, 12):diffuse(color("1,1,1,1")):diffusetopedge(
			Saturation(Brightness(color("1,1,1,1"), 1), 0.5)
		)
	end
}
