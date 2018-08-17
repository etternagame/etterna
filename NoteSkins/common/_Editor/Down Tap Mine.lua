local t = Def.ActorFrame {
	Def.Sprite {
		Texture=NOTESKIN:GetPath( '_down', 'tap mine underlay' );
		Frames = Sprite.LinearFrames( 1, 1 );
		InitCommand=function(self)
			self:diffuseshift():effectcolor1(0.4,0,0,1):effectcolor2(1,0,0,1):effectclock('beat')
		end;
	};
		Def.Sprite {
		Texture=NOTESKIN:GetPath( '_down', 'tap mine base' );
		Frames = Sprite.LinearFrames( 1, 1 );
		InitCommand=function(self)
			self:spin():effectclock('beat'):effectmagnitude(0,0,80)
		end;
	};
		Def.Sprite {
		Texture=NOTESKIN:GetPath( '_down', 'tap mine overlay' );
		Frames = Sprite.LinearFrames( 1, 1 );
		InitCommand=function(self)
			self:spin():effectclock('beat'):effectmagnitude(0,0,-40)
		end;
	};
};
return t;
