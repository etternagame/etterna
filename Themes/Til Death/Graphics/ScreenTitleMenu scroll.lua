local gc = Var("GameCommand")

return Def.ActorFrame {
	LoadFont("Common Normal") .. {
		Text=THEME:GetString("ScreenTitleMenu",gc:GetText()),
		OnCommand=function(self)
			self:halign(0)
		end,
		GainFocusCommand=function(self)
			self:zoom(0.6):diffusealpha(1):diffuse(getMainColor('positive'))
		end,
		LoseFocusCommand=function(self)
			self:diffuse(getMainColor('positive')):diffusealpha(0.7):zoom(0.5)
		end,
 	}
}

