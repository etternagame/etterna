local t = Def.ActorFrame {
	Def.Sprite {
		Texture=NOTESKIN:GetPath( '_upleftsolo', 'underlay' );
		Frame0000=0;
		Delay0000=1;
		InitCommand=function(self)
			self:zoomx(-1)
		end;
	};
	Def.Sprite {
		Texture=NOTESKIN:GetPath( '_down', 'tap note' );
		Frame0000=0;
		Delay0000=1;
		InitCommand=function(self)
			self:rotationz(-135)
		end;
	};
};
return t;