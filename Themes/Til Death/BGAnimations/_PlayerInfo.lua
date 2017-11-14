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
local AvatarX = 0
local AvatarY = SCREEN_HEIGHT-50
local playerRating = 0

t[#t+1] = Def.Actor{
	BeginCommand=function(self)
		self:queuecommand("Set")
	end,
	SetCommand=function(self)
		profile = GetPlayerOrMachineProfile(PLAYER_1)
		profileName = profile:GetDisplayName()
		playCount = profile:GetTotalNumSongsPlayed()
		playTime = profile:GetTotalSessionSeconds()
		noteCount = profile:GetTotalTapsAndHolds()
		playerRating = profile:GetPlayerRating()
	end,
	PlayerJoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	PlayerUnjoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end	
}

t[#t+1] = Def.ActorFrame{
	Name="Avatar"..PLAYER_1,
	BeginCommand=function(self)
		self:queuecommand("Set")
	end,
	SetCommand=function(self)
		if profile == nil then
			self:visible(false)
		else
			self:visible(true)
		end
	end,
	PlayerJoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	PlayerUnjoinedMessageCommand=function(self)
		self:queuecommand("Set")
	end,

	Def.Sprite {
		Name="Image",
		InitCommand=function(self)
			self:visible(true):halign(0):valign(0):xy(AvatarX,AvatarY)
		end,
		BeginCommand=function(self)
			self:queuecommand("ModifyAvatar")
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("ModifyAvatar")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("ModifyAvatar")
		end,
		ModifyAvatarCommand=function(self)
			self:finishtweening()
			self:Load(THEME:GetPathG("","../"..getAvatarPath(PLAYER_1)))
			self:zoomto(50,50)
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(AvatarX+53,AvatarY+20):halign(0):zoom(0.35):diffuse(getMainColor('positive'))
		end,
		BeginCommand=function(self)
			self:queuecommand("Set")
		end,
		SetCommand=function(self)
			self:settext(playCount.." Plays")
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(AvatarX+53,AvatarY+30):halign(0):zoom(0.35):diffuse(getMainColor('positive'))
		end,
		BeginCommand=function(self)
			self:queuecommand("Set")
		end,
		SetCommand=function(self)
			self:settext(noteCount.." Arrows Smashed")
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(AvatarX+53,AvatarY+40):halign(0):zoom(0.35):diffuse(getMainColor('positive'))
		end,
		BeginCommand=function(self)
			self:queuecommand("Set")
		end,
		SetCommand=function(self)
			local time = SecondsToHHMMSS(playTime)
			self:settextf(time.." PlayTime")
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(SCREEN_CENTER_X-125,AvatarY+40):halign(0.5):zoom(0.35):diffuse(getMainColor('positive'))
		end,
		BeginCommand=function(self)
			self:queuecommand("Set")
		end,
		SetCommand=function(self)
			self:settext("Judge: "..GetTimingDifficulty())
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(SCREEN_WIDTH-5,AvatarY+10):halign(1):zoom(0.35):diffuse(getMainColor('positive'))
		end,
		BeginCommand=function(self)
			self:queuecommand("Set")
		end,
		SetCommand=function(self)
			self:settext(GAMESTATE:GetEtternaVersion())
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(SCREEN_WIDTH-5,AvatarY+20):halign(1):zoom(0.35):diffuse(getMainColor('positive'))
		end,
		BeginCommand=function(self)
			self:queuecommand("Set")
		end,
		SetCommand=function(self)
			self:settextf("Songs Loaded: %i", SONGMAN:GetNumSongs())
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	LoadFont("Common Normal") .. {
		InitCommand=function(self)
			self:xy(SCREEN_WIDTH-5,AvatarY+30):halign(1):zoom(0.35):diffuse(getMainColor('positive'))
		end,
		BeginCommand=function(self)
			self:queuecommand("Set")
		end,
		SetCommand=function(self)
			self:settextf("Songs Favorited: %i",  profile:GetNumFaves())
		end,
		PlayerJoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		PlayerUnjoinedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
		FavoritesUpdatedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
}

local function Update(self)
	t.InitCommand=function(self)
		self:SetUpdateFunction(Update)
	end;
	if getAvatarUpdateStatus(PLAYER_1) then
    	self:GetChild("Avatar"..PLAYER_1):GetChild("Image"):queuecommand("ModifyAvatar")
    	setAvatarUpdateStatus(PLAYER_1,false)
    end;
end
t.InitCommand=function(self)
	self:SetUpdateFunction(Update)
end	

return t
