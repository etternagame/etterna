local t = LoadActor("AnyRightFoot tap Note") .. {
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
}

return t;
