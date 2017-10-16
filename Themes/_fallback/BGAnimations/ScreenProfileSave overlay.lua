local x = Def.ActorFrame{
	Def.Quad{
		InitCommand=function(self)
			self:Center():zoomto(SCREEN_WIDTH,80):diffuse(color("0,0,0,0.5"))
		end;
	};
	LoadFont("Common Normal")..{
		Text=ScreenString("Saving Profiles");
		InitCommand=function(self)
			self:Center():diffuse(color("1,1,1,1")):shadowlength(1)
		end;
		OffCommand=function(self)
			self:linear(0.15):diffusealpha(0)
		end;
	};
};

x[#x+1] = Def.Actor {
	BeginCommand=function(self)
		if SCREENMAN:GetTopScreen():HaveProfileToSave() then self:sleep(1); end;
		self:queuecommand("Load");
	end;
	LoadCommand=function() SCREENMAN:GetTopScreen():Continue(); end;
};

return x;