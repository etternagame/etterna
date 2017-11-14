return Def.Sprite {
	Texture=NOTESKIN:GetPath('UpRightFist','Roll Head Active');
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
};