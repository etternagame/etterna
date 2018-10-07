local t = Def.ActorFrame {}

t[#t + 1] = LoadActor("Mine_Base")

t[#t + 1] =
	LoadActor("Mine_Fill") ..
	{
		InitCommand = function(self)
			self:diffuseshift():effectcolor1(color("#FFFFFFFF")):effectcolor2(color("#FFFFFF22")):effectclock("bgm"):effectperiod(
				2
			)
		end
	}
t[#t + 1] =
	LoadActor("Mine_Fill") ..
	{
		InitCommand = function(self)
			self:blend(Blend.Add):diffuseshift():effectcolor1(color("#FFFFFFFF")):effectcolor2(color("#FFFFFF22")):effectclock(
				"bgm"
			):effectperiod(2)
		end
	}

t[#t + 1] =
	LoadActor("Mine_Border") ..
	{
		InitCommand = function(self)
			self:spin():effectmagnitude(0, 0, 36)
		end
	}

t[#t + 1] = LoadActor("Mine_Overlay")

t[#t + 1] =
	LoadActor("Mine_Light") ..
	{
		InitCommand = function(self)
			self:blend(Blend.Add):diffuseshift():effectcolor1(color("#FFFFFF55")):effectcolor2(color("#FFFFFF00")):effectclock(
				"bgm"
			):zoom(1.15):effectperiod(2)
		end
	}

return t
