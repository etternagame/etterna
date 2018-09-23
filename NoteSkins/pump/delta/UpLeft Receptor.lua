return Def.ActorFrame {
	NOTESKIN:LoadActor(Var "Button", "Ready Receptor") ..
		{
			Name = "Base",
			InitCommand = function(self)
				self:animate(0):setstate(0)
			end
		},
	NOTESKIN:LoadActor(Var "Button", "Ready Receptor") ..
		{
			Name = "Glow",
			InitCommand = function(self)
				self:animate(0):setstate(1)
			end,
			OnCommand = function(self)
				self:effectclock("bgm"):diffuseshift():effectcolor1(color("#FFFFFFFF")):effectcolor2(color("#FFFFFF00")):effecttiming(
					1,
					0,
					0,
					0
				)
			end
		},
	NOTESKIN:LoadActor(Var "Button", "Ready Receptor") ..
		{
			Name = "Tap",
			InitCommand = function(self)
				self:animate(0):setstate(2):zoom(1):diffusealpha(0):blend("BlendMode_Add")
			end,
			PressCommand = function(self)
				self:diffuse(color("#FFFFAA")):stoptweening():zoom(1.1):linear(0.1):diffusealpha(0.6):zoom(1)
			end,
			LiftCommand = function(self)
				self:diffuse(color("#FFFFAA")):stoptweening():diffusealpha(0.6):zoom(1):linear(0.15):zoom(1.2):diffusealpha(0)
			end
		}
}
