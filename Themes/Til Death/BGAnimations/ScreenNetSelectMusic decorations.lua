local t = LoadActor("ScreenSelectMusic decorations/default")
	t[#t + 1] =
		Def.ActorFrame {
		BeginCommand = function(self)
			SCREENMAN:GetTopScreen():AddInputCallback(MPinput)
		end
	}
	
return t