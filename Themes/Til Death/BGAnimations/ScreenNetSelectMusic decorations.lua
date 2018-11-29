local t = LoadActor("ScreenSelectMusic decorations/default")
	t[#t + 1] =
		Def.ActorFrame {
		BeginCommand = function(self)
			MESSAGEMAN:Broadcast("AddMPChatInput")
			SCREENMAN:GetTopScreen():AddInputCallback(input)-- not sure if we need this or fallback handles it -mina
		end
	}
	
return t