--[[ Really hacky song search, don't want to use existing textinput because search results should update dynamically rather than requiring new input;
	need to do efficiency and organizational rewrites or better yet take a different approach altogether, i mean they did say not to do this in the 
	first place because blah blah asian people sanskrit moonelmorunestar type but you know, whatever, murica.
	Will probably will merge this into a tab so there can be a nice display of search string/instructions and displayed results regardless
	
	Don't think i need to add more characters but maybe later if it seems i do, also artist search isnt enabled in any way atm
	note to self: searching by best player grade might be useful, such as all songs for which the best grade is failed
--]]

-- need to redo this whole mess

local searchstring = ""
local englishes = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"}
if not storedSearchInfo() then
	storeSearchInfo(SONGMAN:GetAllSongs())
end

local allSongs
local allTitles
local allArtists
local results = {}
local resultsindex = {}
local curResult = 1
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)

local function searchInput(event)
	local hurrr = false
	if event.type ~= "InputEventType_Release" then
		if event.button == "Back" then
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			SCREENMAN:GetTopScreen():RemoveInputCallback(searchInput)
			ms.ok("Search Cancelled")
			storeSongSearchResult(nil, nil) 
			SCREENMAN:SetNewScreen("ScreenSelectMusic")
		elseif event.button == "Start" then
			SCREENMAN:set_input_redirected(PLAYER_1, false)						-- make sure we store the song found so we can retrieve it and set the music wheel in the new music select screen to it
			SCREENMAN:GetTopScreen():RemoveInputCallback(searchInput)           -- this is because setting and removing input callbacks on the same screen more than once for some reason prevents the 
			ms.ok("Search Finished")                                            -- subsequent input callbacks to not recieve any inputs, effectively forcing a game restart since input_redirected is true
			storeSongSearchResult(GAMESTATE:GetCurrentSong(),GAMESTATE:GetCurrentSteps(PLAYER_1))
			SCREENMAN:SetNewScreen("ScreenSelectMusic")							
		elseif event.DeviceInput.button == "DeviceButton_space" then			-- add space to the string
			searchstring = searchstring.." "
			MESSAGEMAN:Broadcast("SearchUpdate")
		elseif event.DeviceInput.button == "DeviceButton_backspace" or event.DeviceInput.button == "DeviceButton_delete" then
			searchstring = searchstring:sub(1, -2)										-- remove the last element of the string
			MESSAGEMAN:Broadcast("SearchUpdate")
		else
			for i=1,#englishes do														-- add standard characters to string
				if event.DeviceInput.button == "DeviceButton_"..englishes[i] then
					searchstring = searchstring..englishes[i]
					MESSAGEMAN:Broadcast("SearchUpdate")
					hurrr = true
				end
			end
		end
	end
	
	if hurrr then
		if event.button == "Left" then											-- need to be here so that only the arrrow keys swap search results, not left binds such as "a"		
			MESSAGEMAN:Broadcast("SwitchResultsLeft")							
		elseif event.button == "Right" then
			MESSAGEMAN:Broadcast("SwitchResultsRight")
		end
	end
	return true
end

local t = Def.ActorFrame{
	CodeMessageCommand=function(self,params)
		if params.Name == "SongSearch" then
			ms.ok("Song search activated")
			allSongs = getAllSongs()
			allTitles = getAllTitles()
			allArtists = getAllArtists()
			storeSongSearchResult(GAMESTATE:GetCurrentSong()) 					-- store current song so we can get back to it if search is cancelled, rather than having the most recently played song load
			SCREENMAN:GetTopScreen():AddInputCallback(searchInput)
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
		end
	end
}

-- oh boy.. auto set the wheel to any search result that is stored and then clear it away if so, cant do this earlier in the script since music wheel doesn't exist yet
-- this garbage is to make sure the preview music of the most recently played song doesnt play instead of the song search match... and even then.. if you hold tab....
if recallSearchResult() then
	t[#t+1] = Def.Actor{
		OnCommand=function(self)
			setSongFromCompletedSearch()
			MESSAGEMAN:Broadcast("UpdateChart")
			MESSAGEMAN:Broadcast("RefreshChartInfo")
		end,
	}
end

t[#t+1] = Def.Actor{
	SearchUpdateMessageCommand=cmd(sleep,0;queuecommand,"Set"),
	SetCommand=function(self)
		self:finishtweening()
		results = {}
		resultsindex = {}
		
		for i =1, #allTitles do 
			if string.find(allTitles[i], searchstring) then
				results[#results+1] = allTitles[i]
				resultsindex[#resultsindex+1] = i
			end
		end
		
		curResult = 1
		setSongFromSearch(allSongs[resultsindex[curResult]])
	end,
	SwitchResultsLeftMessageCommand=function(self)
		if curResult == 1 then
			curResult = #results
		else
			curResult = curResult-1
		end
		
		setSongFromSearch(allSongs[resultsindex[curResult]])
	end,
	SwitchResultsRightMessageCommand=function(self)	
		if curResult == #results then
			curResult = 1
		else
			curResult = curResult+1
		end
		
		setSongFromSearch(allSongs[resultsindex[curResult]])
	end,
}

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX,frameY-90;zoom,0.5;halign,0),
	BeginCommand=function(self)
		self:settext("")
	end,
	SetCommand=function(self) 
		self:settext("Searching for:")
	end,
	RefreshSearchResultsMessageCommand=cmd(queuecommand,"Set"),
	SearchEndMessageCommand=cmd(queuecommand,"Begin"),
}

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX+15,frameY-70;zoom,0.7;halign,0;maxwidth,470),
	BeginCommand=function(self)
		self:settext("")
	end,
	SetCommand=function(self) 
		self:settext(searchstring)
	end,
	RefreshSearchResultsMessageCommand=cmd(queuecommand,"Set"),
	SearchEndMessageCommand=cmd(queuecommand,"Begin"),
}

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX,frameY-50;zoom,0.4;halign,0),
	BeginCommand=function(self)
		self:settext("")
	end,
	SetCommand=function(self)
		if #results ~= 0 then
			self:settext("Showing Match:"..curResult.." of "..#results)
		elseif searchstring == "" then
			self:settext("No search string to match")
		else
			self:settext("No matches for current query. Backspace or delete to alter search string.")
		end
	end,
	RefreshSearchResultsMessageCommand=cmd(queuecommand,"Set"),
	SearchEndMessageCommand=cmd(queuecommand,"Begin"),
}
t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX,frameY-40;zoom,0.35;halign,0),
	BeginCommand=function(self)
		self:settext("")
	end,
	SetCommand=function(self) 
		self:settext("Left or Right to change selection.")
	end,
	RefreshSearchResultsMessageCommand=cmd(queuecommand,"Set"),
	SearchEndMessageCommand=cmd(queuecommand,"Begin"),
}

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX,frameY-31;zoom,0.35;halign,0),
	BeginCommand=function(self)
		self:settext("")
	end,
	SetCommand=function(self) 
		self:settext("Back or Start to exit search.")
	end,
	RefreshSearchResultsMessageCommand=cmd(queuecommand,"Set"),
	SearchEndMessageCommand=cmd(queuecommand,"Begin"),
}

return t