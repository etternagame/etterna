local t =
	Def.ActorFrame {
	Def.Sprite {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH + 100, SCREEN_CENTER_Y)
			self:Load(getAssetPath("toasty") .. ".png")
		end,
		StartTransitioningCommand = function(self)
			self:decelerate(0.25):x(SCREEN_WIDTH - 100):sleep(1.75):accelerate(0.5):x(SCREEN_WIDTH + 100)
		end
	},
	Def.Sound {
		InitCommand = function(self)
			self:load(getAssetPath("toasty") .. ".ogg")
		end,
		StartTransitioningCommand = function(self)
			self:play()
		end
	}
}
return t
