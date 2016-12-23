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
	SortOrder_BeginnerMeter 		= 'Beginner Meter',
	SortOrder_EasyMeter 			= 'Easy Meter',
	SortOrder_DoubleMediumMeter 	= 'Normal Meter',
	SortOrder_HardMeter 			= 'Hard Meter',
	SortOrder_ChallengeMeter 		= 'Insane Meter',
	SortOrder_DoubleEasyMeter 		= 'Double Easy Meter',
	SortOrder_DoubleMediumMeter 	= 'Double Normal Meter',
	SortOrder_DoubleHardMeter 		= 'Double Hard Meter',
	SortOrder_DoubleChallengeMeter 	= 'Double Insane Meter',
	SortOrder_ModeMenu 				= 'Mode Menu',
	SortOrder_AllCourses 			= 'All Courses',
	SortOrder_Nonstop 				= 'Nonstop',
	SortOrder_Oni 					= 'Oni',
	SortOrder_Endless 				= 'Endless',
	SortOrder_Length 				= 'Song Length',
	SortOrder_Roulette 				= 'Roulette',
	SortOrder_Recent 				= 'Recently Played'
};

-- t[#t+1] = Def.Quad{
	-- Name="CurrentSort";
	-- InitCommand=cmd(xy,frameX,frameY;halign,1;zoomto,frameWidth,frameHeight;diffuse,getMainColor(1););
-- };

-- t[#t+1] = LoadFont("Common Normal") .. {
	-- InitCommand=cmd(xy,frameX-frameWidth+5,frameY;halign,0;zoom,0.45;maxwidth,(frameWidth-40)/0.45);
	-- BeginCommand=cmd(queuecommand,"Set");
	-- SetCommand=function(self)
		-- local sort = GAMESTATE:GetSortOrder()
		-- local song = GAMESTATE:GetCurrentSong()
		-- if sort == nil then
			-- self:settext("Sort: ")
		-- elseif sort == "SortOrder_Group" and song ~= nil then
			-- self:settext(song:GetGroupName())
		-- else
			-- self:settext("Sort: "..sortTable[sort])
		-- end

	-- end;
	-- SortOrderChangedMessageCommand=cmd(queuecommand,"Set");
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };

t[#t+1] = LoadFont("Common Normal") .. {
	InitCommand=cmd(xy,frameX-5,frameY;halign,1;zoom,0.3;maxwidth,40/0.45);
	BeginCommand=cmd(queuecommand,"Set");
	SetCommand=function(self)
		local top = SCREENMAN:GetTopScreen()
		if top:GetName() == "ScreenSelectMusic" or top:GetName() == "ScreenNetSelectMusic" then
			local wheel = top:GetMusicWheel()
			self:settextf("%d/%d",wheel:GetCurrentIndex()+1,wheel:GetNumItems())
		end;
	end;
	SortOrderChangedMessageCommand=cmd(queuecommand,"Set");
	CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
};

--t[#t+1] = StandardDecorationFromFileOptional("BPMDisplay","BPMDisplay");
--t[#t+1] = StandardDecorationFromFileOptional("BPMLabel","BPMLabel");

--just a simple mouse rollover test.
--[[ 
local function Update(self)
	t.InitCommand=cmd(SetUpdateFunction,Update);
	if isOver(self:GetChild("CurrentSort")) then
    	self:GetChild("CurrentSort"):diffusealpha(0.5)
    else
    	self:GetChild("CurrentSort"):diffusealpha(1)
    end;
end; 

t.InitCommand=cmd(SetUpdateFunction,Update);
--]]

return t