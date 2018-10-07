--taken from custom speedmods so I don't have to define my own incrment size.
local default_speed_increment = 25
local function get_speed_increment()
	local increment = default_speed_increment
	if ReadGamePrefFromFile("SpeedIncrement") then
		increment = tonumber(GetGamePref("SpeedIncrement")) or default_speed_increment
	else
		WriteGamePrefToFile("SpeedIncrement", increment)
	end
	return increment
end

local increment = get_speed_increment()

-- Stuff for using device inputs instead of mapped keys.
--[[
local po = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptions("ModsLevel_Preferred");
local modstring = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString("ModsLevel_Preferred");
local speedType = 1 -- 1 = x, 2 = c, 3 = m
local xSpeed
local cSpeed

if po:XMod() ~= nil then
	speedType = 1
	xSpeed = po:XMod()
elseif po:CMod() ~= nil then
	speedType = 2
	cSpeed = po:CMod()
elseif po:MMod() ~= nil then
	speedType = 3
	cSpeed = po:MMod()
else
	speedType = 1
	xSpeed = 1
end;

local function inputP1(event)
	local topScreen = SCREENMAN:GetTopScreen();
	local temp = topScreen:GetChildren().Underlay:GetChildren().SpeedChangeP1;
	local c = temp:GetChildren();

	local avatarOption = topScreen:GetChildren().Overlay:GetChildren().Avatars:GetChildren().P1Avatar:GetChildren().P1AvatarOption
	
	if event.DeviceInput.button == 'DeviceButton_=' then
		if event.type == "InputEventType_Release" then
			--c.Test:settext("hoooo")

			if speedType == 2 then
				po:CMod(math.max(10,po:CMod()+increment))
				--c.Test:settext("hooooC")
			elseif speedType == 3 then
				po:MMod(math.max(10,po:MMod()+increment))
				--c.Test:settext("hooooM")
			else
				po:XMod(math.max(0.1,po:XMod()+(increment/10)))
				--c.Test:settext("hooooX")
			end;
			GAMESTATE:GetPlayerState(PLAYER_1):SetPlayerOptions("ModsLevel_Preferred",GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString("ModsLevel_Preferred"));
			avatarOption:settext(GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString('ModsLevel_Current'))
		end;
	end;
	
	if event.DeviceInput.button == 'DeviceButton_-' then
		if event.type == "InputEventType_Release" then
			--c.Test:settext("woooo")
			if speedType == 2 then
				po:CMod(math.max(10,po:CMod()-increment))
			elseif speedType == 3 then
				po:MMod(math.max(10,po:MMod()-increment))
			else
				po:XMod(math.max(0.1,po:XMod()-(increment/10)))
			end;
			GAMESTATE:GetPlayerState(PLAYER_1):SetPlayerOptions("ModsLevel_Preferred",GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString("ModsLevel_Preferred"));
			avatarOption:settext(GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString('ModsLevel_Current'))
		end;
	end;
	
	return true;
end
--]]
local t =
	Def.ActorFrame {
	Name = "SpeedChange",
	CodeMessageCommand = function(self, params)
		local pn = params.PlayerNumber
		local po = GAMESTATE:GetPlayerState(pn):GetPlayerOptions("ModsLevel_Preferred")
		local os = GAMESTATE:GetPlayerState(pn):GetPlayerOptionsString("ModsLevel_Preferred")
		local speedType = 1 -- 1 = x, 2 = c, 3 = m
		local xSpeed
		local cSpeed
		local avatarOption
		local topScreen = SCREENMAN:GetTopScreen()

		--Grab actors for the optionlines beside the profile avatar
		if pn == PLAYER_1 and GAMESTATE:IsPlayerEnabled(PLAYER_1) then
			avatarOption =
				topScreen:GetChildren().Overlay:GetChildren().Avatars:GetChildren().P1Avatar:GetChildren().P1AvatarOption
		elseif pn == PLAYER_2 and GAMESTATE:IsPlayerEnabled(PLAYER_2) then
			avatarOption =
				topScreen:GetChildren().Overlay:GetChildren().Avatars:GetChildren().P2Avatar:GetChildren().P2AvatarOption
		end

		--Check type of speedmod used. if something goes wrong, default to xmod.
		if po:XMod() ~= nil then
			speedType = 1
			xSpeed = po:XMod()
		elseif po:CMod() ~= nil then
			speedType = 2
			cSpeed = po:CMod()
		elseif po:MMod() ~= nil then
			speedType = 3
			cSpeed = po:MMod()
		else
			speedType = 1
			xSpeed = 1
		end

		--increment speedmods when a certain key is pressed (EffectUp/EffectDown)
		if params.Name == "SpeedUp" then
			if speedType == 2 then
				po:CMod(math.max(10, po:CMod() + increment))
			elseif speedType == 3 then
				po:MMod(math.max(10, po:MMod() + increment))
			else
				po:XMod(math.max(0.1, po:XMod() + (increment / 100)))
			end
		elseif params.Name == "SpeedDown" then
			if speedType == 2 then
				po:CMod(math.max(10, po:CMod() - increment))
			elseif speedType == 3 then
				po:MMod(math.max(10, po:MMod() - increment))
			else
				po:XMod(math.max(0.1, po:XMod() - (increment / 100)))
			end
		end

		--Set the speedmod and set the player's option text.
		if GAMESTATE:IsPlayerEnabled(pn) then
			GAMESTATE:GetPlayerState(pn):SetPlayerOptions(
				"ModsLevel_Preferred",
				GAMESTATE:GetPlayerState(pn):GetPlayerOptionsString("ModsLevel_Preferred")
			)
			avatarOption:settext(GAMESTATE:GetPlayerState(pn):GetPlayerOptionsString("ModsLevel_Current"))
		end
	end
	--OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(inputP1) end
}

return t
