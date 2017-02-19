-- Old avatar actor frame.. renamed since much more will be placed here (hopefully?)
local t = Def.ActorFrame{
	Name="PlayerAvatar"
}

local profile

local profileName = "No Profile"
local playCount = 0
local playTime = 0
local noteCount = 0
local numfaves = 0
local skillsets = {
	Overall = 0,
	Speed 	= 0,
	Stam  	= 0,
	Jack  	= 0,
}
local profileXP = 0
local AvatarX = 0
local AvatarY = SCREEN_HEIGHT-50

t[#t+1] = Def.Actor{
	BeginCommand=cmd(queuecommand,"Set");
	SetCommand=function(self)
		if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
			profile = GetPlayerOrMachineProfile(PLAYER_1)
			if profile ~= nil then
				if profile == PROFILEMAN:GetMachineProfile() then
					profileName = "Player 1"
				else
					profileName = profile:GetDisplayName()
				end
				playCount = profile:GetTotalNumSongsPlayed()
				playTime = profile:GetTotalSessionSeconds()
				noteCount = profile:GetTotalTapsAndHolds()
				profileXP = math.floor(profile:GetTotalDancePoints()/10 + profile:GetTotalNumSongsPlayed()*50)

				-- oook i need to handle this differently
				skillsets.Overall = profile:GetPlayerRating()
				skillsets.Speed = profile:GetPlayerSkillsetRating(2)
				skillsets.Stam = profile:GetPlayerSkillsetRating(3)
				skillsets.Jack = profile:GetPlayerSkillsetRating(4)
				
			else 
				profileName = "No Profile"
				playCount = 0
				playTime = 0
				profileXP = 0
			end; 
		else
			profileName = "No Profile"
			playCount = 0
			playTime = 0
			noteCount = 0
			profileXP = 0
		end;
	end;
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set");
	PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set");
}

t[#t+1] = Def.ActorFrame{
	Name="Avatar"..PLAYER_1,
	BeginCommand=cmd(queuecommand,"Set"),
	SetCommand=function(self)
		if profile == nil then
			self:visible(false)
		else
			self:visible(true)
		end
	end,
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
	PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),

	Def.Sprite {
		Name="Image",
		InitCommand=cmd(visible,true;halign,0;valign,0;xy,AvatarX,AvatarY),
		BeginCommand=cmd(queuecommand,"ModifyAvatar"),
		PlayerJoinedMessageCommand=cmd(queuecommand,"ModifyAvatar"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"ModifyAvatar"),
		ModifyAvatarCommand=function(self)
			self:finishtweening()
			self:Load(THEME:GetPathG("","../"..getAvatarPath(PLAYER_1)))
			self:zoomto(50,50)
		end,
	},
	--Revamped. SMO stuff for now. -Misterkister
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+7;halign,0;zoom,0.6;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			local tiers = {[0] = "1: Novice", [7] = "2: Basic", [13] = "3: Intermediate", [17] = "4: Advanced", [21] = "5: Expert", [25] = "6: Master", [29] = "7: Veteran", [35] = "8: Legendary", [40] = "9: Vibro Legend"}
			local index = math.floor(skillsets.Overall)
				while tiers[index] == nil do
				index = index - 1
				end
			if IsNetSMOnline() then
			self:settextf("%s: %5.2f (Tier %s)",profileName,skillsets.Overall,tiers[index])
			else
			self:settextf("%s: %5.2f",profileName,skillsets.Overall)
			end
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+20;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext(playCount.." Plays")
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+30;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext(noteCount.." Arrows Smashed")
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+40;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			local time = SecondsToHHMMSS(playTime)
			self:settextf(time.." PlayTime")
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_CENTER_X-125,AvatarY+40;halign,0.5;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext("Judge: "..GetTimingDifficulty())
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	--Level system revamped. -Misterkister
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_CENTER_X,AvatarY+25;halign,0.5;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
		local level = 1
			if profileXP > 0 then
				level = math.floor(math.log(profileXP) / math.log(2))
			end
			self:settext("Overall Level: " .. level .. "\nEXP Earned: " .. profileXP .. "/" .. 2^(level+1))
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_WIDTH-5,AvatarY+10;halign,1;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext(GAMESTATE:GetEtternaVersion())
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_WIDTH-5,AvatarY+20;halign,1;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Songs Loaded: %i", SONGMAN:GetNumSongs())
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_WIDTH-5,AvatarY+30;halign,1;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("Songs Favorited: %i",  profile:GetNumFaves())
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		FavoritesUpdatedMessageCommand=cmd(queuecommand,"Set"),
	},
}

local function Update(self)
	t.InitCommand=cmd(SetUpdateFunction,Update);
	if getAvatarUpdateStatus(PLAYER_1) then
    	self:GetChild("Avatar"..PLAYER_1):GetChild("Image"):queuecommand("ModifyAvatar")
    	setAvatarUpdateStatus(PLAYER_1,false)
    end;
end
t.InitCommand=cmd(SetUpdateFunction,Update)

local function littlebits(i)
	local t = Def.ActorFrame{
		LoadFont("Common Normal") .. {
			InitCommand=cmd(xy,AvatarX+200,AvatarY+10*i;halign,0;zoom,0.35;diffuse,getMainColor('positive')),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				self:settext(ms.SkillSets[i]..":")
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Normal") .. {
			InitCommand=cmd(xy,AvatarX+300,AvatarY+10*i;halign,1;zoom,0.35),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self)
				self:settextf("%5.2f",skillsets[ms.SkillSets[i]])
				self:diffuse(ByMSD(skillsets[ms.SkillSets[i]]))
			end,
			PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
			PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
		}
	}
	return t
end

for i=2,#ms.SkillSets do 
	--t[#t+1] = littlebits(i)
end

return t
