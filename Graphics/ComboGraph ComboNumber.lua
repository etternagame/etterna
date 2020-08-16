-- represents the numbers on the combo graph if they are present
-- usually is placed on the largest combo

return LoadFont("Common normal") .. {
	Name = "Numbers",
	InitCommand = function(self)
		self:zoom(0.55)
		self:diffuse(color("#FFFFFF"))
		self:strokecolor(color("#00000077"))
	end
}
