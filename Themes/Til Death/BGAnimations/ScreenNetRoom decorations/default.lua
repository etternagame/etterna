local t = Def.ActorFrame{}

t[#t+1] = LoadActor("../_chatbox")
t[#t+1] = LoadActor("profile")
t[#t+1] = LoadActor("roomsearch")
t[#t+1] = LoadActor("tabs")

local g = Def.ActorFrame{
	TabChangedMessageCommand=function(self)
		local top= SCREENMAN:GetTopScreen()
		if getTabIndex() == 0 then
			top:ChatboxVisible(true)
			top:ChatboxInput(true)
			top:InfoSetVisible(true)
		else 
			top:ChatboxVisible(false)
			top:ChatboxInput(false)
			top:InfoSetVisible(false)
		end
	end,
}

	
t[#t+1] = g

return t