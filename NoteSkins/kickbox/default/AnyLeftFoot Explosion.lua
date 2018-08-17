local t = LoadActor("AnyRightFoot Explosion") .. {
	InitCommand=function(self)
		self:basezoomx(-1)
	end;
}

return t;
