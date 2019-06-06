local function highlight(self)
	self:GetChild("rando"):queuecommand("Highlight")
end

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end

local t = Def.ActorFrame {
	InitCommand=function(self)
		self:SetUpdateFunction(highlight)
		self:SetUpdateFunctionInterval(0.025)
	end
}

local frameWidth = 280
local frameHeight = 20
local frameX = SCREEN_WIDTH - 5
local frameY = 15

local sortTable = {
	SortOrder_Preferred = "Preferred",
	SortOrder_Group = "Group",
	SortOrder_Title = "Title",
	SortOrder_BPM = "BPM",
	SortOrder_Popularity = "Popular",
	SortOrder_TopGrades = "Grade",
	SortOrder_Artist = "Artist",
	SortOrder_Genre = "Genre",
	SortOrder_ModeMenu = "Mode Menu",
	SortOrder_Length = "Song Length",
	SortOrder_Recent = "Recently Played",
	SortOrder_Favorites = "Favorites",
	SortOrder_Overall = "Overall",
	SortOrder_Stream = "Stream",
	SortOrder_Jumpstream = "Jumpstream",
	SortOrder_Handstream = "Handstream",
	SortOrder_Stamina = "Stamina",
	SortOrder_JackSpeed = "JackSpeed",
	SortOrder_Chordjack = "Chordjack",
	SortOrder_Technical = "Technical",
	SortOrder_Length = "Length"
}

t[#t + 1] =
	Def.Quad {
	Name = "CurrentSort",
	InitCommand = function(self)
		self:xy(frameX, frameY):halign(1):zoomto(frameWidth, frameHeight):diffuse(getMainColor("frames"))
	end
}

local group_rand = ""
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		Name="rando",
		InitCommand = function(self)
			self:xy(frameX, frameY + 5):halign(1):zoom(0.55):maxwidth((frameWidth - 40) / 0.35)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local sort = GAMESTATE:GetSortOrder()
			local song = GAMESTATE:GetCurrentSong()
			if sort == nil then
				self:settext("Sort: ")
			elseif sort == "SortOrder_Group" and song ~= nil then
				group_rand = song:GetGroupName()
				self:settext(group_rand)
			else
				self:settext("Sort: " .. sortTable[sort])
				group_rand = ""
			end
		end,
		SortOrderChangedMessageCommand = function(self)
			self:queuecommand("Set"):diffuse(getMainColor("positive"))
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		MouseLeftClickMessageCommand = function(self)
			if group_rand ~= "" and isOver(self) then
				math.randomseed(os.time())
				local t = SONGMAN:GetSongsInGroup(group_rand)
				local random_song = t[math.random(#t)]
				SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(random_song)
			end
		end,
		HighlightCommand=function(self)
			if group_rand ~= "" then
				highlightIfOver(self)
			end
		end
	}

t[#t + 1] = StandardDecorationFromFileOptional("BPMDisplay", "BPMDisplay")
t[#t + 1] = StandardDecorationFromFileOptional("BPMLabel", "BPMLabel")

--just a simple mouse rollover test.
--[[ 
local function Update(self)
	t.InitCommand=function(self)
		self:SetUpdateFunction(Update)
	end;
	if isOver(self:GetChild("CurrentSort")) then
    	self:GetChild("CurrentSort"):diffusealpha(0.5)
    else
    	self:GetChild("CurrentSort"):diffusealpha(1)
    end;
end; 

t.InitCommand=function(self)
	self:SetUpdateFunction(Update)
end;
--]]
return t
