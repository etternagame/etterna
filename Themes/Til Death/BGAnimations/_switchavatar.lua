

--Parameters.
local imgTypes = {".jpg",".png",".gif",".jpeg"}
local rawList = FILEMAN:GetDirListing("Themes/"..THEME:GetCurThemeName().."/Graphics/Player avatar/")
local avatars = filterFileList(rawList,imgTypes)

local maxItems = 7--math.min(7,#avatars)
local itemHeight = 30
local itemWidth = 30
local border = 5
local frameX = 0
local frameY = SCREEN_HEIGHT-55
local height = itemHeight+(border*2)
local width = maxItems*(itemWidth+border)+border

--search for avatar currently being used. if none are found, revert to _fallback.png which is assumed to be on index 1.
local function getInitAvatarIndex(pn)
	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()
	local avatar = avatarConfig:get_data().avatar[GUID]
	for i=1,#avatars do
		if avatar == avatars[i] then
			return i
		end;
	end;

	return 1
end

--place cursor on center unless it's on the edge.
local function getInitCursorIndex(pn)
	local avatarIndex = getInitAvatarIndex(pn)

	if avatarIndex < math.ceil(maxItems/2) then
		return avatarIndex
	elseif avatarIndex > #avatars-math.ceil(maxItems/2) then
		return maxItems-(#avatars-avatarIndex)
	end
	return math.ceil(maxItems/2)
end

local data ={
	PlayerNumber_P1 = {
		cursorIndex = getInitCursorIndex(PLAYER_1),
		avatarIndex = getInitAvatarIndex(PLAYER_1),
	},
	PlayerNumber_P2 = {
		cursorIndex = getInitCursorIndex(PLAYER_2),
		avatarIndex = getInitAvatarIndex(PLAYER_2),
	},
}

local t = Def.ActorFrame{
	Name="AvatarSwitch";
}

--Shifts an actor by "1 index"
local function shift(actor,amount)
	actor:finishtweening()
	actor:smooth(0.1)
	actor:addx((itemWidth+border)*amount)
end

--Grabs the currently selected avatar.
local function getSelectedAvatar(pn)
	return avatars[data[pn]["avatarIndex"]]
end

--Save preferences and sends a systemmessage at the end.
local function saveAvatar(pn)
	local avatar = getSelectedAvatar(pn)
	local profile = PROFILEMAN:GetProfile(pn)
	local GUID = profile:GetGUID()
	avatarConfig:get_data().avatar[GUID] = avatar
	avatarConfig:set_dirty()
	avatarConfig:save()
	SCREENMAN:SystemMessage(string.format("%s Avatar set to: '%s'",pn,avatar))
end

-- The main function that contains errything
local function avatarSwitch(pn)
	local t = Def.ActorFrame{
		Name="AvatarSwitch"..pn;
		BeginCommand=function(self)
			if pn == PLAYER_1 then
				self:x(-width);
				self:sleep(0.3)
				self:smooth(0.2)
				self:x(0)
			end;
			if pn == PLAYER_2 then
				self:x(SCREEN_WIDTH)
				self:sleep(0.3)
				self:smooth(0.2)
				self:x(SCREEN_WIDTH-width)
			end;
		end;
		CodeMessageCommand=function(self,params)
			if params.Name == "AvatarCancel" or params.Name == "AvatarExit" then
				if pn == PLAYER_1 then
					self:smooth(0.2)
					self:x(-width)
				end
				if pn == PLAYER_2 then
					self:smooth(0.2)
					self:x(SCREEN_WIDTH)
				end
			end;
		end;
	}

	t[#t+1] = Def.ActorFrame{
	CodeMessageCommand=function(self,params)
		--grab table/cursor and shift them by 1 to left/right everytime someone presses code for avatarleft/right
		local table = SCREENMAN:GetTopScreen():GetChildren().Overlay:GetChildren().AvatarSwitch:GetChildren()["AvatarSwitch"..pn]:GetChildren().AvatarTable
		local cursor = SCREENMAN:GetTopScreen():GetChildren().Overlay:GetChildren().AvatarSwitch:GetChildren()["AvatarSwitch"..pn]:GetChildren().AvatarCursor
		if params.PlayerNumber == pn then
			if params.Name == "AvatarLeft" then
				if data[pn]["avatarIndex"] > 1 and data[pn]["cursorIndex"] > 1 then
					shift(cursor,-1)
					data[pn]["avatarIndex"] = data[pn]["avatarIndex"] - 1
					data[pn]["cursorIndex"] = data[pn]["cursorIndex"] - 1 
				elseif data[pn]["avatarIndex"] > 1 and data[pn]["cursorIndex"] == 1 then
					shift(table,1)
					data[pn]["avatarIndex"] = data[pn]["avatarIndex"] - 1
				end;
			end;
			if params.Name == "AvatarRight" then
				if data[pn]["avatarIndex"] < #avatars and data[pn]["cursorIndex"] < maxItems then
					shift(cursor,1)
					data[pn]["avatarIndex"] = data[pn]["avatarIndex"] + 1
					data[pn]["cursorIndex"] = data[pn]["cursorIndex"] + 1 
				elseif data[pn]["avatarIndex"] < #avatars and data[pn]["cursorIndex"] == maxItems then
					shift(table,-1)
					data[pn]["avatarIndex"] = data[pn]["avatarIndex"] + 1
				end;
			end;
		end;
		--rq out of the screen if just canceling.
		if params.Name == "AvatarCancel" then
			SCREENMAN:GetTopScreen():Cancel()
		end;
		--save and exit if exiting. forcefully save both players when 2p as only the changes for the person who pressed exit will be applied.
		if params.Name == "AvatarExit" then
			if GAMESTATE:GetNumPlayersEnabled() == 1 then
				saveAvatar(params.PlayerNumber)
				setAvatarUpdateStatus(pn,true)
			else
				saveAvatar(PLAYER_1)
				setAvatarUpdateStatus(PLAYER_1,true)
				saveAvatar(PLAYER_2)
				setAvatarUpdateStatus(PLAYER_2,true)
			end;
			SCREENMAN:GetTopScreen():Cancel()
		end;
	end;
	}

	--Background Quad
	t[#t+1] = Def.Quad{
		InitCommand=cmd(xy,frameX,frameY;zoomto,width,height;halign,0;valign,1;diffuse,color("#00000066"));
	}

	--MASKING SCKS
	t[#t+1] = Def.Quad{
		InitCommand=cmd(xy,width,0;zoomto,SCREEN_WIDTH-width,SCREEN_HEIGHT;halign,0;valign,0;zwrite,true;clearzbuffer,true;blend,'BlendMode_NoEffect';);
		BeginCommand=function(self)
			if pn == PLAYER_2 then
				self:x(0)
				self:halign(1)
			end;
		end;
	}

	--Cursor
	t[#t+1] = Def.Quad{
		Name="AvatarCursor";
		InitCommand=cmd(xy,frameX-2+border,frameY+2-border;zoomto,itemHeight+4,itemWidth+4;halign,0;valign,1;diffuse,color("#FFFFFF"));
		BeginCommand=function(self)
			shift(self,(data[pn]["cursorIndex"]-1))
		end;
	}

	--List of avatars
	local avatarTable = Def.ActorFrame{
		Name="AvatarTable";
		BeginCommand=function(self)
			shift(self,-(data[pn]["avatarIndex"]-1))
			shift(self,(data[pn]["cursorIndex"]-1))
		end;
	}
	t[#t+1] = avatarTable
	for k,v in pairs(avatars) do
		avatarTable[#avatarTable+1] = Def.Sprite {
			InitCommand=cmd(visible,true;halign,0;valign,1;xy,frameX+border+((border+itemWidth)*(k-1)),frameY-border;ztest,true;);
			BeginCommand=cmd(queuecommand,"ModifyAvatar");
			ModifyAvatarCommand=function(self)
				self:finishtweening();
				self:LoadBackground(THEME:GetPathG("","Player avatar/"..v));
				self:zoomto(itemWidth,itemHeight)
			end;
		};
	end;

	--Text
	t[#t+1] = LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,frameX,frameY-height;halign,0;valign,1;zoom,0.35;);
		BeginCommand=cmd(queuecommand,"Set");
		SetCommand=function(self,params)
			--self:settextf("Player 1 avatar: ci%d ai%d",cursorIndex,avatarIndex)
			if pn == PLAYER_1 then
				self:settextf("Player 1 Avatar: %s",avatars[data[pn]["avatarIndex"]])
			end;
			if pn == PLAYER_2 then
				self:settextf("Player 2 Avatar: %s",avatars[data[pn]["avatarIndex"]])
			end;
		end;
		CodeMessageCommand=cmd(queuecommand,"Set");
	};
	return t
end

if GAMESTATE:IsHumanPlayer(PLAYER_1) then
	t[#t+1] = avatarSwitch(PLAYER_1)
end
if GAMESTATE:IsHumanPlayer(PLAYER_2) then
	t[#t+1] = avatarSwitch(PLAYER_2)
end

return t