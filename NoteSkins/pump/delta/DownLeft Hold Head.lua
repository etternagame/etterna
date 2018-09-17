local t = Def.ActorFrame {}

t[#t + 1] =
	LoadActor("DownLeft_blend") ..
	{
		InitCommand = function(self)
			self:diffuseshift():effectcolor1(color("#376f9bFF")):effectcolor2(color("#376f9bFF")):fadetop(0.5)
		end
	}

t[#t + 1] =
	LoadActor("DownLeft_blend") ..
	{
		InitCommand = function(self)
			self:blend(Blend.Add):diffuseshift():effectcolor1(color("#5899ccFF")):effectcolor2(color("#5899cc33")):effectclock(
				"bgm"
			):effecttiming(1, 0, 0, 0)
		end
	}

t[#t + 1] =
	LoadActor("DownLeft_fill") ..
	{
		InitCommand = function(self)
			self:blend(Blend.Add):diffuseshift():effectcolor1(color("#5899ccFF")):effectcolor2(color("#5899ccFF"))
		end
	}

t[#t + 1] = LoadActor("DownLeft border")

return t
