return Def.ActorFrame {
	--note graphic
	NOTESKIN:LoadActor(Var "Button", "Tap Note") ..
		{
			InitCommand = function(self)
				self:blend("BlendMode_Add"):playcommand("Glow")
			end,
			W1Command = function(self)
				self:playcommand("Glow")
			end,
			W2Command = function(self)
				self:playcommand("Glow")
			end,
			W3Command = function(self)
				self:playcommand("Glow")
			end,
			HitMineCommand = function(self)
				self:playcommand("Glow")
			end,
			GlowCommand = function(self)
				self:setstate(0):finishtweening():diffusealpha(1.0):zoom(1.0):linear(0.15):diffusealpha(0.9):zoom(1.15):linear(0.15):diffusealpha(
					0.0
				):zoom(1.3)
			end,
			HeldCommand = function(self)
				self:playcommand("Glow")
			end
		},
	--tap
	NOTESKIN:LoadActor(Var "Button", "Ready Receptor") ..
		{
			Name = "Tap",
			--Frames = { { Frame = 2 ; Delay = 1 } };
			TapCommand = function(self)
				self:finishtweening():diffusealpha(1):zoom(1):linear(0.2):diffusealpha(0):zoom(1.2)
			end,
			InitCommand = function(self)
				self:pause():setstate(2):playcommand("Tap")
			end,
			HeldCommand = function(self)
				self:playcommand("Tap")
			end,
			ColumnJudgmentMessageCommand = function(self)
				self:playcommand("Tap")
			end
			--TapNoneCommand=cmd(playcommand,"Tap");
		},
	--explosion
	LoadActor("_flash") ..
		{
			InitCommand = function(self)
				self:blend("BlendMode_Add"):playcommand("Glow")
			end,
			W1Command = function(self)
				self:playcommand("Glow")
			end,
			W2Command = function(self)
				self:playcommand("Glow")
			end,
			W3Command = function(self)
				self:playcommand("Glow")
			end,
			--HoldingOnCommand=cmd(playcommand,"Glow");
			HitMineCommand = function(self)
				self:playcommand("Glow")
			end,
			HeldCommand = function(self)
				self:playcommand("Glow")
			end,
			GlowCommand = function(self)
				self:setstate(0):finishtweening():diffusealpha(1):zoom(1):linear(0.2):diffusealpha(0):zoom(1.2)
			end
		},
	--thing...
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(50, 5000):diffusealpha(0)
		end,
		HitMineCommand = function(self)
			self:finishtweening():diffusealpha(1):linear(0.3):diffusealpha(0)
		end
	}
}
