return Def.Sprite {
	Texture=NOTESKIN:GetPath('AnyRightFist','Roll Head Active');
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
};