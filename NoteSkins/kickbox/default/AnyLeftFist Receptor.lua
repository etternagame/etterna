local t =
	LoadActor("AnyRightFist Receptor") ..
	{
		InitCommand = function(self)
			self:zoomx(-1)
		end
	}

return t
