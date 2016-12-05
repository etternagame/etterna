local searchstring = ""
local englishes = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"}
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)
local active = false
local whee

local function searchInput(event)
	if event.type ~= "InputEventType_Release" and active == true then
		if event.button == "Start" then
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			active = false
		elseif event.DeviceInput.button == "DeviceButton_space" then					-- add space to the string
			searchstring = searchstring.." "
		elseif event.DeviceInput.button == "DeviceButton_backspace" or event.DeviceInput.button == "DeviceButton_delete" then
			searchstring = searchstring:sub(1, -2)										-- remove the last element of the string
		else
			for i=1,#englishes do														-- add standard characters to string
				if event.DeviceInput.button == "DeviceButton_"..englishes[i] then
					searchstring = searchstring..englishes[i]
				end
			end
		end
	end
	MESSAGEMAN:Broadcast("UpdateString")
	whee:SongSearch(searchstring)
end

local t = Def.ActorFrame{
	CodeMessageCommand=function(self,params)
			ms.ok("Song search activated")
			active = true
			whee = SCREENMAN:GetTopScreen():GetMusicWheel()
			SCREENMAN:GetTopScreen():AddInputCallback(searchInput)
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
	end
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
	UpdateStringMessageCommand=cmd(queuecommand,"Set"),
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