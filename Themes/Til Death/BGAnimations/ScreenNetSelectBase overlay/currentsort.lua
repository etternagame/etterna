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
	SortOrder_Favorites				= 'Favorites'
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

return t
