return NOTESKIN:LoadActor("DownLeft", "NoteHit") ..
	{
		InitCommand = function(self)
			self:rotationy(180):rotationz(180):y(-6):x(-2)
		end
	}
