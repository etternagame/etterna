return NOTESKIN:LoadActor("UpLeft", "Tap Note") ..
	{
		InitCommand = function(self)
			self:effectclock("beat"):effectmagnitude(0.5, 1, 0)
		end
	}
