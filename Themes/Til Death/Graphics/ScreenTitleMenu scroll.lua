local gc = Var("GameCommand")

return Def.ActorFrame {
	LoadFont("Common Normal") .. {
		Text=THEME:GetString("ScreenTitleMenu",gc:GetText()),
		OnCommand=function(self)
			self:xy(280,-78):halign(0):valign(0)
		end,
		GainFocusCommand=function(self)
			self:zoom(0.57):diffuse(color("#59307f"))
		end,
		LoseFocusCommand=function(self)
			self:zoom(0.55):diffuse(color("#b87cf0"))
		end,
 	}
}

