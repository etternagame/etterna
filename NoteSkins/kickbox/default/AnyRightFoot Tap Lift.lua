local t = Def.ActorFrame {
	InitCommand=function(self)
		self:pulse():effectclock('beat'):effectmagnitude(1,1.2,1)
	end;
	Def.Sprite {
		Texture=NOTESKIN:GetPath( '_AnyRightFoot', 'tap lift' );
		Frame0000=0;
		Delay0000=1;
		InitCommand=function(self)
			self:diffuseramp():effectclock('beat'):effectcolor1(color("1,1,1,1")):effectcolor2(color("1,1,1,0.5"))
		end;
	};
};
return t;
