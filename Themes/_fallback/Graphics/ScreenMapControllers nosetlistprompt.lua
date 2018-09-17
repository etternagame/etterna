return Def.ActorFrame {
	InitCommand = function(self)
		self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):diffuse(Color.Black):diffusealpha(0)
		end,
		TweenOnCommand = function(self)
			self:stoptweening():diffusealpha(1):linear(0.5):diffusealpha(0.8)
		end,
		TweenOffCommand = function(self)
			self:stoptweening():linear(0.5):diffusealpha(0)
		end
	},
	Def.ActorFrame {
		Def.BitmapText {
			Font = "Common Normal",
			Text = ScreenString("NoSetListPrompt"),
			InitCommand = function(self)
				self:y(10):wrapwidthpixels(SCREEN_WIDTH - 48):diffusealpha(0)
			end,
			TweenOnCommand = function(self)
				self:stoptweening():diffusealpha(0):sleep(0.5125):linear(0.125):diffusealpha(1)
			end,
			TweenOffCommand = function(self)
				self:stoptweening():linear(0.5):diffusealpha(0)
			end
		}
	}
}
