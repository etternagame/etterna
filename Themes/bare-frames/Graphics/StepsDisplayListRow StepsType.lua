-- Controls the StepsType actor for the StepsDisplayListRow

local t = LoadFont("Common normal") .. {
	InitCommand = function(self)
		self:zoom(0.3, maxwidth, 5 / 0.3)
	end
}

return t
