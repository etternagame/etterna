--Input event for mouse clicks
local function input(event)
	local top = SCREENMAN:GetTopScreen()
	if event.DeviceInput.button == 'DeviceButton_left mouse button' then
		if event.type == "InputEventType_Release" then
			if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
				if isOver(top:GetChild("Overlay"):GetChild("PlayerAvatar"):GetChild("Avatar"..PLAYER_1):GetChild("Image")) then
					SCREENMAN:AddNewScreenToTop("ScreenAvatarSwitch");
				end;
			end;
			if GAMESTATE:IsPlayerEnabled(PLAYER_2) then
				if isOver(top:GetChild("Overlay"):GetChild("PlayerAvatar"):GetChild("Avatar"..PLAYER_2):GetChild("Image")) then
					SCREENMAN:AddNewScreenToTop("ScreenAvatarSwitch");
				end;
			end;
		end;
	end
return false;
end

local t = Def.ActorFrame{
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end;
}

t[#t+1] = Def.Actor{
	CodeMessageCommand=function(self,params)
		if params.Name == "AvatarShow" then
			SCREENMAN:AddNewScreenToTop("ScreenAvatarSwitch");
		end;
	end;
};
t[#t+1] = LoadActor("../_frame");
t[#t+1] = LoadActor("../_avatar");
t[#t+1] = LoadActor("currentsort");
t[#t+1] = LoadFont("Common Large")..{
	InitCommand=cmd(xy,5,32;halign,0;valign,1;zoom,0.55;diffuse,getMainColor(1);settext,"Select Music:";);
}

t[#t+1] = LoadActor("../_cursor");
t[#t+1] = LoadActor("../_halppls");

return t