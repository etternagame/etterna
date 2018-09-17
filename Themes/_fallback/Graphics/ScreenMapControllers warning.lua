return Def.ActorFrame {
	InitCommand = function(self)
		self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):diffuse(Color.Black)
		end,
		TweenOnCommand = function(self)
			self:diffusealpha(1):linear(0.5):diffusealpha(0.8)
		end,
		TweenOffCommand = function(self)
			self:linear(0.5):diffusealpha(0)
		end
	},
	Def.ActorFrame {
		Def.BitmapText {
			Font = "Common Normal",
			Text = ScreenString("WarningHeader"),
			InitCommand = function(self)
				self:y(-80):diffuse(Color.Red)
			end,
			TweenOnCommand = function(self)
				self:diffusealpha(0):zoomx(2):zoomy(0):sleep(0.5):smooth(0.25):zoom(1):diffusealpha(1)
			end,
			TweenOffCommand = function(self)
				self:linear(0.5):diffusealpha(0)
			end
		},
		Def.BitmapText {
			Font = "Common Normal",
			Text = ScreenString("WarningText"),
			InitCommand = function(self)
				self:y(10):wrapwidthpixels(SCREEN_WIDTH - 48)
			end,
			TweenOnCommand = function(self)
				self:diffusealpha(0):sleep(0.5125):linear(0.125):diffusealpha(1)
			end,
			TweenOffCommand = function(self)
				self:linear(0.5):diffusealpha(0)
			end
		}
	}
}
