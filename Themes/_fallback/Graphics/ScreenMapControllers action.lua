return Def.BitmapText {
	Font = "Common Normal",
	InitCommand = function(self)
		self:x(SCREEN_CENTER_X):zoom(.75, 0):diffuse(color("#808080"))
	end,
	OnCommand = function(self)
		self:diffusealpha(0)
		self:decelerate(0.5)
		self:diffusealpha(1)
		-- fancy effect:  Look at the name (which is set by the screen) to set text
		self:settext(THEME:GetString("ScreenMapControllers", "Action" .. self:GetName()))
	end,
	OffCommand = function(self)
		self:stoptweening():accelerate(0.3):diffusealpha(0):queuecommand("Hide")
	end,
	HideCommand = function(self)
		self:visible(false)
	end,
	GainFocusCommand = function(self)
		self:diffuseshift():effectcolor2(color("#808080")):effectcolor1(color("#FFFFFF"))
	end,
	LoseFocusCommand = function(self)
		self:stopeffect()
	end
}
