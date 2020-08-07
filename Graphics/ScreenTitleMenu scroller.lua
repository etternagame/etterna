local gc = Var("GameCommand")

local choiceTextZoom = 0.6

return Def.ActorFrame {
	LoadFont("Common Large") ..
		{
			OnCommand = function(self)
				self:halign(0)
				self:zoom(choiceTextZoom)
				self:settext(THEME:GetString(SCREENMAN:GetTopScreen():GetName(), gc:GetText()))
			end
		}
}
