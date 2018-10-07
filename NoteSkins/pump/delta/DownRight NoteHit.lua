return NOTESKIN:LoadActor("DownLeft", "NoteHit") ..
	{
		OnCommand = function(self)
			self:x(-2):zoomx(-1)
		end
	}
