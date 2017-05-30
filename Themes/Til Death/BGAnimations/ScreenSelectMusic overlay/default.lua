local t = Def.ActorFrame{}

t[#t+1] = Def.Actor{
	CodeMessageCommand=function(self,params)
		if params.Name == "AvatarShow" and getTabIndex() == 1 then
			SCREENMAN:AddNewScreenToTop("ScreenAvatarSwitch")
		end
	end
}

t[#t+1] = LoadActor("../_frame")
t[#t+1] = LoadActor("../_PlayerInfo")
t[#t+1] = LoadActor("currentsort")
t[#t+1] = LoadFont("Common Large")..{
	InitCommand=cmd(xy,5,32;halign,0;valign,1;zoom,0.55;diffuse,getMainColor('positive');settext,"Select Music:");
}
t[#t+1] = LoadActor("../_cursor")
t[#t+1] = LoadActor("../_mousewheelscroll")
t[#t+1] = LoadActor("../_mouseselect")
t[#t+1] = LoadActor("../_halppls")
t[#t+1] = LoadActor("currenttime")

return t
