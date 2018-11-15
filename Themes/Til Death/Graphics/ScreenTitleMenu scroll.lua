local gc = Var("GameCommand")

return Def.ActorFrame {
	LoadFont("Common Normal") ..
		{
			Text = THEME:GetString("ScreenTitleMenu", gc:GetText()),
			OnCommand = function(self)
				self:xy(280, -78):halign(0):valign(0)
			end,
			GainFocusCommand = function(self)
				self:zoom(0.57):diffuse(getMainColor('positive'))
			end,
			LoseFocusCommand = function(self)
				self:zoom(0.55):diffuse(getTitleColor('Line_Left'))
			end
		}
}
