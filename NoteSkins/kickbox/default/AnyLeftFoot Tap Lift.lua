local t = LoadActor("AnyRightFoot Tap Lift") .. {
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
}

return t;
