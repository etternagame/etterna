-- Controls transitions into every screen unless overridden by another 'ScreenName in'
-- This just creates a black box that covers the screen and then fades it to invisible after 0.1 seconds
return Def.Quad {
	InitCommand = function(self)
		self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y):zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
	end,
	OnCommand = function(self)
		self:diffuse(color("0,0,0,1")):sleep(0.1):linear(0.1):diffusealpha(0)
	end
}
