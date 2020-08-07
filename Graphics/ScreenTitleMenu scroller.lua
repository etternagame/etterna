local gc = Var("GameCommand")

local choiceTextZoom = 0.6

return Def.ActorFrame {
	-- the name of this frame is determined by C++
	-- it will be the name of the choice
	-- ie: ScrollerChoice<GameCommandName>

	LoadFont("Common Large") ..	{
		Name = "ScrollerText",
		BeginCommand = function(self)
			self:halign(0)
			self:zoom(choiceTextZoom)
			self:settext(THEME:GetString(SCREENMAN:GetTopScreen():GetName(), gc:GetText()))
		end
	}
}
