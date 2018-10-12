local t =
	LoadActor("AnyRightFoot Receptor") ..
	{
		InitCommand = function(self)
			self:zoomx(-1)
		end
	}

return t
