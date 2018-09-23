local t =
	LoadActor("AnyRightFist Explosion") ..
	{
		InitCommand = function(self)
			self:basezoomx(-1)
		end
	}

return t
