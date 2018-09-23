return Def.Quad {
	InitCommand = function(self)
		self:setsize(1, 12):diffuse(getMainColor("highlight")):diffusetopedge(
			Saturation(Brightness(getMainColor("highlight"), 1), 0.5)
		)
	end,
	BeginCommand = function(self)
		self:glowshift():effectcolor1(color("1,1,1,0.325")):effectcolor2(color("1,1,1,0"))
	end
}
