return LoadActor("_CenterHit") ..
	{
		InitCommand = function(self)
			self:x(2):y(2)
		end
	}
