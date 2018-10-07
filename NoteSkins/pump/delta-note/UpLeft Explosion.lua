return Def.ActorFrame {
	NOTESKIN:LoadActor(Var "Button", "NoteHit") ..
		{
			InitCommand = function(self)
				self:animate(0):blend(Blend.Add):diffusealpha(0)
			end,
			NoneCommand = function(self)
				self:playcommand("Glow")
			end,
			PressCommand = function(self)
				self:playcommand("Glow")
			end,
			W1Command = function(self)
				self:setstate(0):playcommand("W2")
			end,
			W2Command = function(self)
				self:setstate(0):playcommand("Glow")
			end,
			W3Command = function(self)
				self:setstate(1):playcommand("Glow")
			end,
			W4Command = function(self)
				self:setstate(2):playcommand("Glow")
			end,
			HitMineCommand = function(self)
				self:playcommand("Glow")
			end,
			HeldCommand = function(self)
				self:setstate(0):playcommand("Glow")
			end,
			GlowCommand = function(self)
				self:stoptweening(zoom, 1.05):diffusealpha(1):linear(0.25):zoom(1.1):diffusealpha(0)
			end
		},
	NOTESKIN:LoadActor(Var "Button", "NoteHit") ..
		{
			InitCommand = function(self)
				self:animate(0):blend(Blend.Add):diffusealpha(0)
			end,
			NoneCommand = function(self)
				self:playcommand("Glow")
			end,
			PressCommand = function(self)
				self:playcommand("Glow")
			end,
			W1Command = function(self)
				self:setstate(0):playcommand("W2")
			end,
			W2Command = function(self)
				self:setstate(0):playcommand("Glow")
			end,
			W3Command = function(self)
				self:setstate(1):playcommand("Glow")
			end,
			W4Command = function(self)
				self:setstate(2):playcommand("Glow")
			end,
			HitMineCommand = function(self)
				self:playcommand("Glow")
			end,
			HeldCommand = function(self)
				self:setstate(0):playcommand("Glow")
			end,
			GlowCommand = function(self)
				self:stoptweening(zoom, 1):diffusealpha(0.4):linear(0.3):zoom(1.2):diffusealpha(0)
			end
		},
	NOTESKIN:LoadActor(Var "Button", "NoteHit") ..
		{
			InitCommand = function(self)
				self:animate(0):zoom(1.1):blend(Blend.Add):visible(false)
			end,
			HoldingOnCommand = function(self)
				self:visible(true)
			end,
			HoldingOffCommand = function(self)
				self:visible(false)
			end
		}
}
