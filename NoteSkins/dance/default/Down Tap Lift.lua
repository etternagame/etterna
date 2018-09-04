local t = Def.ActorFrame {
	LoadActor("lift") .. {
		--[[InitCommand=function(self)
			self:animate(false):pulse():effectclock("beat"):effectmagnitude(0.9,1,1)
		end]]
		-- who even wants a pulsing lift anyways
	}
}
return t
