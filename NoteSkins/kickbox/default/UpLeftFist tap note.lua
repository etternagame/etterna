local t = LoadActor("UpRightFist tap Note") .. {
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
}

return t;
