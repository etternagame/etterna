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
	}
}
