return LoadFont("Common Normal") .. {
	InitCommand=cmd(xy,0,10;halign,0;zoom,0.4;);
	BeginCommand=function(self)
		self:settext("AG")
	end;
};