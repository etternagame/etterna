local t = LoadActor("AnyRightFist Tap Lift") .. {
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
}

return t;
