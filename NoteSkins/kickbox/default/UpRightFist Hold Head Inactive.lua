return Def.Sprite {
	Texture=NOTESKIN:GetPath('UpRightFist','Hold Head Active');
	InitCommand=function(self)
		self:diffuse(color("0.5,0.5,0.5,1"))
	end;
};