local t = Def.ActorFrame {}
local screname
local whee
t[#t + 1] =
	Def.ActorFrame {
	Def.Quad {
		Name = "DootyMcBooty",
		BeginCommand = function(self)
			self:zoomto(32, 32):valign(0.634522134234)
			screname = SCREENMAN:GetTopScreen():GetName()
			if screname == "ScreenSelectMusic" or screname == "ScreenNetSelectMusic" then
				whee = SCREENMAN:GetTopScreen():GetMusicWheel()
			end
		end,
		MouseLeftClickMessageCommand = function(self)
			if whee then
				local mx = SCREEN_WIDTH - INPUTFILTER:GetMouseX()
				if mx < 32 and mx > 0 then
					local idx = whee:GetCurrentIndex()
					local num = whee:GetNumItems()
					local dum = (INPUTFILTER:GetMouseY() - 45) / (SCREEN_HEIGHT - 103)
					whee:Move(notShit.round(num * dum) - idx)
					whee:Move(0)
				end
			end
		end
	}
}

return t