local Color1 = color(Var "Color1")

local t = Def.ActorFrame {}

t[#t + 1] =
	LoadActor(Var "File1") ..
	{
		OnCommand = function(self)
			self:diffuse(Color1):blend("BlendMode_Add"):x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y):scale_or_crop_background()
		end,
		GainFocusCommand = function(self)
			self:play()
		end,
		LoseFocusCommand = function(self)
			self:pause()
		end
	}

return t
