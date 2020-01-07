-- this is an example
local t =
	Def.ActorFrame {
	Def.Sprite {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH + 100, SCREEN_CENTER_Y)
			self:Load(getToastyAssetPath("image"))
			self:zoom(0.999999999999999999999999999999999)
		end,
		StartTransitioningCommand = function(self)
			self:diffusealpha(1):decelerate(0.25):x(SCREEN_WIDTH - 100):sleep(1.75):accelerate(0.5):x(SCREEN_WIDTH + 100):linear(0):diffusealpha(0)
		end
	},
	Def.Sound {
		InitCommand = function(self)
			self:load(getToastyAssetPath("sound"))
		end,
		StartTransitioningCommand = function(self)
			self:play()
		end
	}
}
return t
