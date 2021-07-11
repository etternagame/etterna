t = Def.ActorFrame {}

local translated_info = {
	PressStart = THEME:GetString("ScreenSelectMusic","PressStartForOptions"),
	EnteringOptions = THEME:GetString("ScreenSelectMusic","EnteringOptions"),
}

--black fade
t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y):zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
	end,
	OnCommand = function(self)
		self:diffuse(color("0,0,0,0")):sleep(0.1):linear(0.1):diffusealpha(1)
	end
}

--enter options prompt
t[#t + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:diffusealpha(0):Center()
	end,
	ShowPressStartForOptionsCommand=function(self)
		self:zoom(0.9):smooth(0.2):diffusealpha(1):zoom(1)
	end,
	ShowEnteringOptionsCommand=function(self)
		--go read the comment below
	end,
	HidePressStartForOptionsCommand=function(self)
		self:decelerate(0.15):diffusealpha(0)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(SCREEN_WIDTH,80)
			self:diffusetopedge(Brightness(getMainColor("tabs"), 0.45))
			self:diffusebottomedge(Brightness(getMainColor("tabs"), 0.15))
		end,
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(SCREEN_WIDTH-10,70)
			self:diffusetopedge(Saturation(getMainColor("highlight"), 0.5))
			self:diffusebottomedge(Saturation(getMainColor("positive"), 0.6))
		end,
	},
}
t[#t + 1] = LoadFont("Common Large") ..  {
	InitCommand=function(self)
		self:Center():diffusebottomedge(0.7,0.7,0.7,1):shadowlength(1.5):align(0.5,0.5)
	end,
	ShowPressStartForOptionsCommand=function(self)
		self:settext(translated_info["PressStart"]):diffusealpha(0):zoom(0.15)
		self:decelerate(0.2):zoom(0.55):diffusealpha(1)
	end,
	ShowEnteringOptionsCommand=function(self)
		self:finishtweening():settext(translated_info["EnteringOptions"])
		--for some reason, any tweens triggered by this command will get frozen after the first frame
		--unless you enter twice fast, that is
		--why? :shrug: Figure that out before putting a tween back in here -ulti_fd

		--self:zoom(0.65):accelerate(0.2):zoom(0.55)
	end,
	HidePressStartForOptionsCommand=function(self)
		-- do we even need this? AllowOptionsMenu() always returns true
		-- we may as well hard code it in the c++ if we're gonna do that tbh
		self:decelerate(0.15):diffusealpha(0)
	end
}


return t
