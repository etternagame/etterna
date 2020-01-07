local translated_info = {
	ExitText = THEME:GetString("ScreenTestInput", "ExitText"),
	Start = THEME:GetString("GameButton", "Start")
}

return Def.ActorFrame {
	--Def.ControllerStateDisplay {
	--	InitCommand=function(self)
	--	self:LoadGameController()
	--end,
	--};
	Def.DeviceList {
		Font = "Common Normal",
		InitCommand = function(self)
			self:x(SCREEN_LEFT + 20):y(SCREEN_TOP + 80):zoom(0.8):halign(0)
		end
	},
	Def.InputList {
		Font = "Common Normal",
		InitCommand = function(self)
			self:x(SCREEN_CENTER_X - 250):y(SCREEN_CENTER_Y):zoom(1):halign(0):vertspacing(8)
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y+150):halign(0.5):valign(0.5):zoomto(240,40):diffuse(color("#000000")):diffusealpha(0.5)
		end
	},
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y+150):zoom(0.5):halign(0.5):maxwidth(240 / 0.5)
		end,
		OnCommand = function(self)
			local fmtstr = translated_info["ExitText"]
			self:settextf(fmtstr, translated_info["Start"])
		end
	}
}
