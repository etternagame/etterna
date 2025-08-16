-- the funniest move around.
local button = Var "Button"

-- making sure the player isn't on downscroll
-- then doing a flip.
local flip = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():Reverse() == 1

local t = Def.ActorFrame {
	Def.Sprite {
		Texture = "_receptor",
		Frame0000 = 0,
		Delay0000 = 1,
		InitCommand = function(self)
			-- i love scaling textures
			self:diffuse(color(colors[button])):zoomx(0.5)
			if not flip then
				self:rotationx(180)
			end
		end
	},
	Def.Sprite {
		Texture = "_flash",
		Frame0000 = 0,
		Delay0000 = 1,
		InitCommand = function(self)
			self:diffuse(color(colors[button])):diffusealpha(0):zoomx(0.5)
			if not flip then
				self:rotationx(180)
			end
		end,
		PressCommand = function(self)
			self:diffusealpha(0.25)
		end,
		LiftCommand = function(self)
			self:accelerate(0.25):diffusealpha(0)
		end
	}
}
return t
