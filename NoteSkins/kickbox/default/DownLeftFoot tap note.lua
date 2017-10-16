local t = LoadActor("DownRightfoot tap Note") .. {
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
}

return t;
