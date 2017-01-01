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
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) SCREENMAN:GetTopScreen():UsersVisible(false) end
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
--t[#t+1] = LoadActor("wifesearchbar")

t[#t+1] = LoadFont("Common Normal") .. {
	InitCommand=cmd(xy,SCREEN_WIDTH/3,SCREEN_TOP+15;zoom,0.35;diffuse,getMainColor('positive');maxwidth,SCREEN_WIDTH),
	BeginCommand=cmd(queuecommand,"Set"),
	SetCommand=function(self)
			local str = ""
			local top = SCREENMAN:GetTopScreen()
			if top:GetUserQty() > 5 then
				for i=1,5 do
					str = str .. "      " .. (top:GetUser(i))
				end
			
			else
				for i=1,top:GetUserQty() do
					str = str .. "      " .. (top:GetUser(i))
				end
			end
			self:settext(str)
	end,
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
	PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	UsersUpdateMessageCommand=cmd(queuecommand,"Set"),
}
t[#t+1] = LoadFont("Common Normal") .. {
	InitCommand=cmd(xy,SCREEN_WIDTH/3,SCREEN_TOP+25;zoom,0.35;diffuse,getMainColor('positive');maxwidth,SCREEN_WIDTH),
	BeginCommand=cmd(queuecommand,"Set"),
	SetCommand=function(self)
			local str = ""
			local top = SCREENMAN:GetTopScreen()
			if top:GetUserQty() > 5 then
				for i=6,top:GetUserQty() do
					str = str .. "      " .. (top:GetUser(i))
				end
			end
			self:settext(str)
	end,
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
	PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	UsersUpdateMessageCommand=cmd(queuecommand,"Set"),
}

return t
