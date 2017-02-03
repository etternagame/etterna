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

local AvatarX = 0
local AvatarY = SCREEN_HEIGHT-50
local profileXP = 0


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
				--Since I'm awful at making level algorithms, I'm just going to use this algorithm straight coming from Prim's levels.lua.
				--Unfortunately, it's not a true exp formula, so rip. -Misterkister
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
				noteCount = 0
				level = 0
			end; 
		else
			profileName = "No Profile"
			playCount = 0
			playTime = 0
			noteCount = 0
			level = 0
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
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,AvatarX+53,AvatarY+7;halign,0;zoom,0.6;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settextf("%s: %5.2f",profileName,skillsets.Overall)
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
		InitCommand=cmd(xy,SCREEN_CENTER_X,AvatarY+10;halign,0.5;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			self:settext("Judge: "..GetTimingDifficulty())
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
	--Along with the level system, this need to be simplified. -Misterkister
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_CENTER_X,AvatarY+30;halign,0.5;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
		if profileXP < 4 then
			self:settext("EXP Earned: "..profileXP.."/4")
		elseif profileXP == 4 then
			self:settext("EXP Earned: "..profileXP.."/8")
		elseif profileXP > 4 and profileXP < 8 then
			self:settext("EXP Earned: "..profileXP.."/8")
		elseif profileXP == 8 then
			self:settext("EXP Earned: "..profileXP.."/16")
		elseif profileXP > 8 and profileXP < 16 then
			self:settext("EXP Earned: "..profileXP.."/16")
		elseif profileXP == 16 then
			self:settext("EXP Earned: "..profileXP.."/32")
		elseif profileXP > 16 and profileXP < 32 then
			self:settext("EXP Earned: "..profileXP.."/32")
		elseif profileXP == 32 then
			self:settext("EXP Earned: "..profileXP.."/64")
		elseif profileXP > 32 and profileXP < 64 then
			self:settext("EXP Earned: "..profileXP.."/64")
		elseif profileXP == 64 then
			self:settext("EXP Earned: "..profileXP.."/128")
		elseif profileXP > 64 and profileXP < 128 then
			self:settext("EXP Earned: "..profileXP.."/128")
		elseif profileXP == 128 then
			self:settext("EXP Earned: "..profileXP.."/256")
		elseif profileXP > 128 and profileXP < 256 then
			self:settext("EXP Earned: "..profileXP.."/256")
		elseif profileXP == 256 then
			self:settext("EXP Earned: "..profileXP.."/512")
		elseif profileXP > 256 and profileXP < 512 then
			self:settext("EXP Earned: "..profileXP.."/512")
		elseif profileXP == 512 then
			self:settext("EXP Earned: "..profileXP.."/1024")
		elseif profileXP > 512 and profileXP < 1024 then
			self:settext("EXP Earned: "..profileXP.."/1024")
		elseif profileXP == 1024 then
			self:settext("EXP Earned: "..profileXP.."/2048")
		elseif profileXP > 1024 and profileXP < 2048 then
			self:settext("EXP Earned: "..profileXP.."/2048")
		elseif profileXP == 2048 then
			self:settext("EXP Earned: "..profileXP.."/4096")
		elseif profileXP > 2048 and profileXP < 4096 then
			self:settext("EXP Earned: "..profileXP.."/4096")
		elseif profileXP == 4096 then
			self:settext("EXP Earned: "..profileXP.."/8192")
		elseif profileXP > 4096 and profileXP < 8192 then
			self:settext("EXP Earned: "..profileXP.."/8192")
		elseif profileXP == 8192 then
			self:settext("EXP Earned: "..profileXP.."/16384")
		elseif profileXP > 8192 and profileXP < 16384 then
			self:settext("EXP Earned: "..profileXP.."/16384")
		elseif profileXP == 16384 then
			self:settext("EXP Earned: "..profileXP.."/32768")
		elseif profileXP > 16384 and profileXP < 32768 then
			self:settext("EXP Earned: "..profileXP.."/32768")
		elseif profileXP == 32768 then
			self:settext("EXP Earned: "..profileXP.."/65536")
		elseif profileXP > 32768 and profileXP < 65536 then
			self:settext("EXP Earned: "..profileXP.."/65536")
		elseif profileXP == 65536 then
			self:settext("EXP Earned: "..profileXP.."/131072")
		elseif profileXP > 65536 and profileXP < 131072 then
			self:settext("EXP Earned: "..profileXP.."/131072")
		elseif profileXP == 131072 then
			self:settext("EXP Earned: "..profileXP.."/262144")
		elseif profileXP > 131072 and profileXP < 262144 then
			self:settext("EXP Earned: "..profileXP.."/262144")
		elseif profileXP == 262144 then
			self:settext("EXP Earned: "..profileXP.."/524288")
		elseif profileXP > 262144 and profileXP < 524288 then
			self:settext("EXP Earned: "..profileXP.."/524288")
		elseif profileXP == 524288 then
			self:settext("EXP Earned: "..profileXP.."/1048576")
		elseif profileXP > 524288 and profileXP < 1048576 then
			self:settext("EXP Earned: "..profileXP.."/1048576")
		elseif profileXP == 1048576 then
			self:settext("EXP Earned: "..profileXP.."/2097152")
		elseif profileXP > 1048576 and profileXP < 2097152 then
			self:settext("EXP Earned: "..profileXP.."/2097152")
		elseif profileXP == 2097152 then
			self:settext("EXP Earned: "..profileXP.."/4194304")
		elseif profileXP > 2097152 and profileXP < 4194304 then
			self:settext("EXP Earned: "..profileXP.."/4194304")
		elseif profileXP == 4194304 then
			self:settext("EXP Earned: "..profileXP.."/8388608")
		elseif profileXP > 4194304 and profileXP < 8388608 then
			self:settext("EXP Earned: "..profileXP.."/8388608")
		elseif profileXP == 8388608 then
			self:settext("EXP Earned: "..profileXP.."/16777216")
		elseif profileXP > 8388608 and profileXP < 16777216 then
			self:settext("EXP Earned: "..profileXP.."/16777216")
		elseif profileXP == 16777216 then
			self:settext("EXP Earned: "..profileXP.."/33554432")
		elseif profileXP > 16777216 and profileXP < 33554432 then
			self:settext("EXP Earned: "..profileXP.."/33554432")
		elseif profileXP == 33554432 then
			self:settext("EXP Earned: "..profileXP.."/67108864")
		elseif profileXP > 33554432 and profileXP < 67108864 then
			self:settext("EXP Earned: "..profileXP.."/67108864")
		elseif profileXP == 67108864 then
			self:settext("EXP Earned: "..profileXP.."/134217728")
		elseif profileXP > 67108864 and profileXP < 134217728 then
			self:settext("EXP Earned: "..profileXP.."/134217728")
		elseif profileXP == 134217728 then
			self:settext("EXP Earned: "..profileXP.."/268435456")
		elseif profileXP > 134217728 and profileXP < 268435456 then
			self:settext("EXP Earned: "..profileXP.."/268435456")
		elseif profileXP == 268435456 then
			self:settext("EXP Earned: "..profileXP.."/536870912")
		elseif profileXP > 268435456 and profileXP < 536870912 then
			self:settext("EXP Earned: "..profileXP.."/536870912")
		elseif profileXP == 536870912 then
			self:settext("EXP Earned: "..profileXP.."/1073741824")
		elseif profileXP > 536870912 and profileXP < 1073741824 then
			self:settext("EXP Earned: "..profileXP.."/1073741824")
		elseif profileXP == 1073741824 then
			self:settext("EXP Earned: "..profileXP.."/2147483648")
		elseif profileXP > 1073741824 and profileXP < 2147483648 then
			self:settext("EXP Earned: "..profileXP.."/2147483648")
		elseif profileXP == 2147483648 then
			self:settext("EXP Earned: "..profileXP.."/4294967296")
		elseif profileXP > 2147483648 and profileXP < 4294967296 then
			self:settext("EXP Earned: "..profileXP.."/4294967296")
		elseif profileXP == 4294967296 then
			self:settext("EXP Earned: "..profileXP.."/8589934592")
		elseif profileXP > 4294967296 and profileXP < 8589934592 then
			self:settext("EXP Earned: "..profileXP.."/8589934592")
		elseif profileXP == 8589934592 then
			self:settext("EXP Earned: "..profileXP.."/17179869184")
		elseif profileXP > 8589934592 and profileXP < 17179869184 then
			self:settext("EXP Earned: "..profileXP.."/17179869184")
		elseif profileXP == 17179869184 then
			self:settext("EXP Earned: "..profileXP.."/34359738368")
		elseif profileXP > 17179869184 and profileXP < 34359738368 then
			self:settext("EXP Earned: "..profileXP.."/34359738368")
		elseif profileXP == 34359738368 then
			self:settext("EXP Earned: "..profileXP.."/68719476736")
		elseif profileXP > 34359738368 and profileXP < 68719476736 then
			self:settext("EXP Earned: "..profileXP.."/68719476736")
		elseif profileXP == 68719476736 then
			self:settext("EXP Earned: "..profileXP.."/137438953472")
		elseif profileXP > 68719476736 and profileXP < 137438953472 then
			self:settext("EXP Earned: "..profileXP.."/137438953472")
		elseif profileXP == 137438953472 then
			self:settext("EXP Earned: "..profileXP.."/274877906944")
		elseif profileXP > 137438953472 and profileXP < 274877906944 then
			self:settext("EXP Earned: "..profileXP.."/274877906944")
		elseif profileXP == 274877906944 then
			self:settext("EXP Earned: "..profileXP.."/549755813888")
		elseif profileXP > 274877906944 and profileXP < 549755813888 then
			self:settext("EXP Earned: "..profileXP.."/549755813888")
		elseif profileXP == 549755813888 then
			self:settext("EXP Earned: "..profileXP.."/1.0995116e+12")
		--For someone that's been playing a lot in this game, this needs testing since these numbers are not in expanded form. -Misterkister
		elseif profileXP > 549755813888 and profileXP < 1.0995116e+12 then
			self:settext("EXP Earned: "..profileXP.."/1.0995116e+12")
		elseif profileXP == 1.0995116e+12 then
			self:settext("EXP Earned: "..profileXP.."/2.1990233e+12")
		elseif profileXP > 1.0995116e+12 and profileXP < 2.1990233e+12 then
			self:settext("EXP Earned: "..profileXP.."/2.1990233e+12")
		elseif profileXP == 2.1990233e+12 then
			self:settext("EXP Earned: "..profileXP.."/4.3980465e+12")
		elseif profileXP > 2.1990233e+12 and profileXP < 4.3980465e+12 then
			self:settext("EXP Earned: "..profileXP.."/4.3980465e+12")
		elseif profileXP == 4.3980465e+12 then
			self:settext("EXP Earned: "..profileXP.."/8.796093e+12")
		elseif profileXP > 4.3980465e+12 and profileXP < 8.796093e+12 then
			self:settext("EXP Earned: "..profileXP.."/8.796093e+12")
		elseif profileXP == 8.796093e+12 then
			self:settext("EXP Earned: "..profileXP.."/1.7592186e+13")
		elseif profileXP > 8.796093e+12 and profileXP < 1.7592186e+13 then
			self:settext("EXP Earned: "..profileXP.."/1.7592186e+13")
		elseif profileXP == 1.7592186e+13 then
			self:settext("EXP Earned: "..profileXP.."/3.5184372e+13")
		elseif profileXP > 1.7592186e+13 and profileXP < 3.5184372e+13 then
			self:settext("EXP Earned: "..profileXP.."/3.5184372e+13")
		elseif profileXP == 3.5184372e+13 then
			self:settext("EXP Earned: "..profileXP.."/7.0368744e+13")
		elseif profileXP > 3.5184372e+13 and profileXP < 7.0368744e+13 then
			self:settext("EXP Earned: "..profileXP.."/7.0368744e+13")
		elseif profileXP == 7.0368744e+13 then
			self:settext("EXP Earned: "..profileXP.."/1.4073749e+14")
		elseif profileXP > 7.0368744e+13 and profileXP < 1.4073749e+14 then
			self:settext("EXP Earned: "..profileXP.."/1.4073749e+14")
		elseif profileXP == 1.4073749e+14 then
			self:settext("EXP Earned: "..profileXP.."/2.8147498e+14")
		elseif profileXP > 1.4073749e+14 and profileXP < 2.8147498e+14 then
			self:settext("EXP Earned: "..profileXP.."/2.8147498e+14")
		elseif profileXP == 2.8147498e+14 then
			self:settext("EXP Earned: "..profileXP.."/5.6294995e+14")
		elseif profileXP > 2.8147498e+14 and profileXP < 5.6294995e+14 then
			self:settext("EXP Earned: "..profileXP.."/5.6294995e+14")
		elseif profileXP == 5.6294995e+14 then
			self:settext("EXP Earned: "..profileXP.."/1.1258999e+15")
		elseif profileXP > 5.6294995e+14 and profileXP < 1.1258999e+15 then
			self:settext("EXP Earned: "..profileXP.."/1.1258999e+15")
		--Maxed Out Level. -Misterkister
		elseif profileXP == 1.1258999e+15 then
			self:settext("EXP Earned: "..profileXP)
		elseif profileXP > 1.1258999e+15 then
			self:settext("EXP Earned: "..profileXP)
		else
			self:settext("EXP Earned: "..profileXP.."/4")
			end
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
	},
	--This is BY FAR the stupidest way to set the levels, but I'm going to put this here until someone find a proper efficient way to fix this.
	--Levels are up to 50. Testing is needed around level 39. -Misterkister
	--Algorithm is 2^x. -Misterkister
	LoadFont("Common Normal") .. {
		InitCommand=cmd(xy,SCREEN_CENTER_X,AvatarY+20;halign,0.5;zoom,0.35;diffuse,getMainColor('positive')),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
		if profileXP < 4 then
			self:settext("Overall Level: 1")
		elseif profileXP == 4 then
			self:settext("Overall Level: 2")
		elseif profileXP > 4 and profileXP < 8 then
			self:settext("Overall Level: 2")
		elseif profileXP == 8 then
			self:settext("Overall Level: 3")
		elseif profileXP > 8 and profileXP < 16 then
			self:settext("Overall Level: 3")
		elseif profileXP == 16 then
			self:settext("Overall Level: 4")
		elseif profileXP > 16 and profileXP < 32 then
			self:settext("Overall Level: 4")
		elseif profileXP == 32 then
			self:settext("Overall Level: 5")
		elseif profileXP > 32 and profileXP < 64 then
			self:settext("Overall Level: 5")
		elseif profileXP == 64 then
			self:settext("Overall Level: 6")
		elseif profileXP > 64 and profileXP < 128 then
			self:settext("Overall Level: 6")
		elseif profileXP == 128 then
			self:settext("Overall Level: 7")
		elseif profileXP > 128 and profileXP < 256 then
			self:settext("Overall Level: 7")
		elseif profileXP == 256 then
			self:settext("Overall Level: 8")
		elseif profileXP > 256 and profileXP < 512 then
			self:settext("Overall Level: 8")
		elseif profileXP == 512 then
			self:settext("Overall Level: 9")
		elseif profileXP > 512 and profileXP < 1024 then
			self:settext("Overall Level: 9")
		elseif profileXP == 1024 then
			self:settext("Overall Level: 10")
		elseif profileXP > 1024 and profileXP < 2048 then
			self:settext("Overall Level: 10")
		elseif profileXP == 2048 then
			self:settext("Overall Level: 11")
		elseif profileXP > 2048 and profileXP < 4096 then
			self:settext("Overall Level: 11")
		elseif profileXP == 4096 then
			self:settext("Overall Level: 12")
		elseif profileXP > 4096 and profileXP < 8192 then
			self:settext("Overall Level: 12")
		elseif profileXP == 8192 then
			self:settext("Overall Level: 13")
		elseif profileXP > 8192 and profileXP < 16384 then
			self:settext("Overall Level: 13")
		elseif profileXP == 16384 then
			self:settext("Overall Level: 14")
		elseif profileXP > 16384 and profileXP < 32768 then
			self:settext("Overall Level: 14")
		elseif profileXP == 32768 then
			self:settext("Overall Level: 15")
		elseif profileXP > 32768 and profileXP < 65536 then
			self:settext("Overall Level: 15")
		elseif profileXP == 65536 then
			self:settext("Overall Level: 16")
		elseif profileXP > 65536 and profileXP < 131072 then
			self:settext("Overall Level: 16")
		elseif profileXP == 131072 then
			self:settext("Overall Level: 17")
		elseif profileXP > 131072 and profileXP < 262144 then
			self:settext("Overall Level: 17")
		elseif profileXP == 262144 then
			self:settext("Overall Level: 18")
		elseif profileXP > 262144 and profileXP < 524288 then
			self:settext("Overall Level: 18")
		elseif profileXP == 524288 then
			self:settext("Overall Level: 19")
		elseif profileXP > 524288 and profileXP < 1048576 then
			self:settext("Overall Level: 19")
		elseif profileXP == 1048576 then
			self:settext("Overall Level: 20")
		elseif profileXP > 1048576 and profileXP < 2097152 then
			self:settext("Overall Level: 20")
		elseif profileXP == 2097152 then
			self:settext("Overall Level: 21")
		elseif profileXP > 2097152 and profileXP < 4194304 then
			self:settext("Overall Level: 21")
		elseif profileXP == 4194304 then
			self:settext("Overall Level: 22")
		elseif profileXP > 4194304 and profileXP < 8388608 then
			self:settext("Overall Level: 22")
		elseif profileXP == 8388608 then
			self:settext("Overall Level: 23")
		elseif profileXP > 8388608 and profileXP < 16777216 then
			self:settext("Overall Level: 23")
		elseif profileXP == 16777216 then
			self:settext("Overall Level: 24")
		elseif profileXP > 16777216 and profileXP < 33554432 then
			self:settext("Overall Level: 24")
		elseif profileXP == 33554432 then
			self:settext("Overall Level: 25")
		elseif profileXP > 33554432 and profileXP < 67108864 then
			self:settext("Overall Level: 25")
		elseif profileXP == 67108864 then
			self:settext("Overall Level: 26")
		elseif profileXP > 67108864 and profileXP < 134217728 then
			self:settext("Overall Level: 26")
		elseif profileXP == 134217728 then
			self:settext("Overall Level: 27")
		elseif profileXP > 134217728 and profileXP < 268435456 then
			self:settext("Overall Level: 27")
		elseif profileXP == 268435456 then
			self:settext("Overall Level: 28")
		elseif profileXP > 268435456 and profileXP < 536870912 then
			self:settext("Overall Level: 28")
		elseif profileXP == 536870912 then
			self:settext("Overall Level: 29")
		elseif profileXP > 536870912 and profileXP < 1073741824 then
			self:settext("Overall Level: 29")
		elseif profileXP == 1073741824 then
			self:settext("Overall Level: 30")
		elseif profileXP > 1073741824 and profileXP < 2147483648 then
			self:settext("Overall Level: 30")
		elseif profileXP == 2147483648 then
			self:settext("Overall Level: 31")
		elseif profileXP > 2147483648 and profileXP < 4294967296 then
			self:settext("Overall Level: 31")
		elseif profileXP == 4294967296 then
			self:settext("Overall Level: 32")
		elseif profileXP > 4294967296 and profileXP < 8589934592 then
			self:settext("Overall Level: 32")
		elseif profileXP == 8589934592 then
			self:settext("Overall Level: 33")
		elseif profileXP > 8589934592 and profileXP < 17179869184 then
			self:settext("Overall Level: 33")
		elseif profileXP == 17179869184 then
			self:settext("Overall Level: 34")
		elseif profileXP > 17179869184 and profileXP < 34359738368 then
			self:settext("Overall Level: 34")
		elseif profileXP == 34359738368 then
			self:settext("Overall Level: 35")
		elseif profileXP > 34359738368 and profileXP < 68719476736 then
			self:settext("Overall Level: 35")
		elseif profileXP == 68719476736 then
			self:settext("Overall Level: 36")
		elseif profileXP > 68719476736 and profileXP < 137438953472 then
			self:settext("Overall Level: 36")
		elseif profileXP == 137438953472 then
			self:settext("Overall Level: 37")
		elseif profileXP > 137438953472 and profileXP < 274877906944 then
			self:settext("Overall Level: 37")
		elseif profileXP == 274877906944 then
			self:settext("Overall Level: 38")
		elseif profileXP > 274877906944 and profileXP < 549755813888 then
			self:settext("Overall Level: 38")
		elseif profileXP == 549755813888 then
			self:settext("Overall Level: 39")
		--For someone that's been playing a lot in this game, this needs testing since these numbers are not in expanded form. -Misterkister
		elseif profileXP > 549755813888 and profileXP < 1.0995116e+12 then
			self:settext("Overall Level: 39")
		elseif profileXP == 1.0995116e+12 then
			self:settext("Overall Level: 40")
		elseif profileXP > 1.0995116e+12 and profileXP < 2.1990233e+12 then
			self:settext("Overall Level: 40")
		elseif profileXP == 2.1990233e+12 then
			self:settext("Overall Level: 41")
		elseif profileXP > 2.1990233e+12 and profileXP < 4.3980465e+12 then
			self:settext("Overall Level: 41")
		elseif profileXP == 4.3980465e+12 then
			self:settext("Overall Level: 42")
		elseif profileXP > 4.3980465e+12 and profileXP < 8.796093e+12 then
			self:settext("Overall Level: 42")
		elseif profileXP == 8.796093e+12 then
			self:settext("Overall Level: 43")
		elseif profileXP > 8.796093e+12 and profileXP < 1.7592186e+13 then
			self:settext("Overall Level: 43")
		elseif profileXP == 1.7592186e+13 then
			self:settext("Overall Level: 44")
		elseif profileXP > 1.7592186e+13 and profileXP < 3.5184372e+13 then
			self:settext("Overall Level: 44")
		elseif profileXP == 3.5184372e+13 then
			self:settext("Overall Level: 45")
		elseif profileXP > 3.5184372e+13 and profileXP < 7.0368744e+13 then
			self:settext("Overall Level: 45")
		elseif profileXP == 7.0368744e+13 then
			self:settext("Overall Level: 46")
		elseif profileXP > 7.0368744e+13 and profileXP < 1.4073749e+14 then
			self:settext("Overall Level: 46")
		elseif profileXP == 1.4073749e+14 then
			self:settext("Overall Level: 47")
		elseif profileXP > 1.4073749e+14 and profileXP < 2.8147498e+14 then
			self:settext("Overall Level: 47")
		elseif profileXP == 2.8147498e+14 then
			self:settext("Overall Level: 48")
		elseif profileXP > 2.8147498e+14 and profileXP < 5.6294995e+14 then
			self:settext("Overall Level: 48")
		elseif profileXP == 5.6294995e+14 then
			self:settext("Overall Level: 49")
		elseif profileXP > 5.6294995e+14 and profileXP < 1.1258999e+15 then
			self:settext("Overall Level: 49")
		--Maxed Out Level. -Misterkister
		elseif profileXP == 1.1258999e+15 then
			self:settext("Overall Level: 50")
		elseif profileXP > 1.1258999e+15 then
			self:settext("Overall Level: 50")
		else
			self:settext("Overall Level: 1")
			end
		end,
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
		PlayerUnjoinedMessageCommand=cmd(queuecommand,"Set"),
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
