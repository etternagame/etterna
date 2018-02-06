local t = Def.ActorFrame{};

local frameWidth = 280
local frameHeight = 20
local frameX = SCREEN_WIDTH-5
local frameY = 15

local sortTable = {
	SortOrder_Preferred 			= 'Preferred',
	SortOrder_Group 				= 'Group',
	SortOrder_Title 				= 'Title',
	SortOrder_BPM 					= 'BPM',
	SortOrder_Popularity 			= 'Popular',
	SortOrder_TopGrades 			= 'Grade',
	SortOrder_Artist 				= 'Artist',
	SortOrder_Genre 				= 'Genre',
	SortOrder_ModeMenu 				= 'Mode Menu',
	SortOrder_Length 				= 'Song Length',
	SortOrder_Recent 				= 'Recently Played',
	SortOrder_Favorites				= 'Favorites',
	SortOrder_Overall				= 'Overall',
	SortOrder_Stream				= 'Stream',
	SortOrder_Jumpstream				= 'Jumpstream',
	SortOrder_Handstream				= 'Handstream',
	SortOrder_Stamina				= 'Stamina',
	SortOrder_JackSpeed				= 'JackSpeed',
	SortOrder_Chordjack				= 'Chordjack',
	SortOrder_Technical				= 'Technical'
}

t[#t+1] = Def.Quad{
	Name="CurrentSort";
	InitCommand=function(self)
		self:xy(frameX,frameY):halign(1):zoomto(frameWidth,frameHeight):diffuse(getMainColor('frames'))
	end;
};

t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(frameX,frameY+5):halign(1):zoom(0.55):maxwidth((frameWidth-40)/0.35)
	end;
	BeginCommand=function(self)
		self:queuecommand("Set")
	end;
	SetCommand=function(self)
		local sort = GAMESTATE:GetSortOrder()
		local song = GAMESTATE:GetCurrentSong()
		if sort == nil then
			self:settext("Sort: ")
		elseif sort == "SortOrder_Group" and song ~= nil then
			self:settext(song:GetGroupName()) 
		else
			self:settext("Sort: "..sortTable[sort])
		end

	end;
	SortOrderChangedMessageCommand=function(self)
		self:queuecommand("Set"):diffuse(getMainColor('positive'))
	end;
	CurrentSongChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end;
};

t[#t+1] = StandardDecorationFromFileOptional("BPMDisplay","BPMDisplay");
t[#t+1] = StandardDecorationFromFileOptional("BPMLabel","BPMLabel");

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