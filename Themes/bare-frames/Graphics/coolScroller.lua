local gc = Var("GameCommand")
return Def.ActorFrame {
	LoadFont("Common Normal") ..
		{
			OnCommand = function(self)
				self:settext(THEME:GetString(SCREENMAN:GetTopScreen():GetName(), gc:GetText()))
			end,
			GainFocusCommand = function(self)
				self:zoom(1)
			end,
			LoseFocusCommand = function(self)
				self:zoom(0.3)
			end
		}
}
