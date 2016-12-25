-- forcibly set the game style to single so we dont crash when loading songs
GAMESTATE:SetCurrentStyle("single")
GAMESTATE:SetCurrentPlayMode('PlayMode_Regular')

--Input event for mouse clicks
local function input(event)
	local top = SCREENMAN:GetTopScreen()
	if event.DeviceInput.button == 'DeviceButton_left mouse button' then
		if event.type == "InputEventType_Release" then
			if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
				if isOver(top:GetChild("Overlay"):GetChild("PlayerAvatar"):GetChild("Avatar"..PLAYER_1):GetChild("Image")) then
					SCREENMAN:AddNewScreenToTop("ScreenAvatarSwitch")
				end
			end
			if GAMESTATE:IsPlayerEnabled(PLAYER_2) then
				if isOver(top:GetChild("Overlay"):GetChild("PlayerAvatar"):GetChild("Avatar"..PLAYER_2):GetChild("Image")) then
					SCREENMAN:AddNewScreenToTop("ScreenAvatarSwitch")
				end
			end
		end
	end
return false
end

local t = Def.ActorFrame{
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end
}

t[#t+1] = Def.Actor{
	CodeMessageCommand=function(self,params)
		if params.Name == "AvatarShow" then
			SCREENMAN:AddNewScreenToTop("ScreenAvatarSwitch")
		end
	end
}
t[#t+1] = LoadActor("../_frame")
t[#t+1] = LoadActor("../_PlayerInfo")
t[#t+1] = LoadActor("currentsort")
t[#t+1] = LoadActor("currenttime")
t[#t+1] = LoadFont("Common Large")..{InitCommand=cmd(xy,5,32;halign,0;valign,1;zoom,0.55;diffuse,getMainColor("positive");settext,"Select Music:")}
t[#t+1] = LoadActor("../_cursor")
t[#t+1] = LoadActor("../_halppls")
t[#t+1] = LoadActor("textentrysearchbar")

local skillsets = {
	Overall = 0,
	Speed 	= 0,
	Stam  	= 0,
	Jack  	= 0,
}


if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	profile = GetPlayerOrMachineProfile(PLAYER_1)
	if profile ~= nil then
		skillsets.Overall = profile:GetPlayerRating()
		skillsets.Speed = profile:GetPlayerSpeedRating()
		skillsets.Stam = profile:GetPlayerStamRating()
		skillsets.Jack = profile:GetPlayerJackRating()
	end
end



local function littlebits(i)
	local t = Def.ActorFrame{
		LoadFont("Common Normal") .. {
			InitCommand=cmd(xy,SCREEN_WIDTH/3,SCREEN_HEIGHT-50+i*10;halign,1;zoom,0.35;diffuse,getMainColor('positive')),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
					self:settext(ms.SkillSets[i]..":")
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Normal") .. {
			InitCommand=cmd(xy,SCREEN_WIDTH/3+30,SCREEN_HEIGHT-50+i*10;halign,1;zoom,0.35;diffuse,getMainColor('positive')),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
					self:settextf("%5.2f",skillsets[ms.SkillSets[i]])
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		}
	}
	return t
end


for i=1,#ms.SkillSets do 
	t[#t+1] = littlebits(i)
end


return t
