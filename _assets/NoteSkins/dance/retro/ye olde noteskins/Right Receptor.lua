return LoadActor( NOTESKIN:GetPath("", "_Tap Receptor"), NOTESKIN:LoadActor( "Left", "Go Receptor" ) )..{
	InitCommand=function(self)
		self:zoomx(-1)
	end,
};