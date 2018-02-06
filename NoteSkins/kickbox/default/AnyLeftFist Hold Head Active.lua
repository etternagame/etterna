return Def.Sprite {
	Texture=NOTESKIN:GetPath('AnyRightFist','Hold Head Active');
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
};