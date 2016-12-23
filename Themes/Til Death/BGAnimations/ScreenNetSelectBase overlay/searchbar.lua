-- Search functionalities
-- to be used in ScreenNetSelectMusic and/or ScreenSelectMusic
-- Now uses Til Death's search system

local searchstring = ""
local englishes = {"a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"}
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)
local active = false
local whee
local mouseClickHandled = false;

local function searchInput(event)
	if event.type ~= "InputEventType_Release" and active == true then
		if event.button == "Back" then
			searchstring = ""
			whee:SongSearch(searchstring)
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("TabChanged")
		elseif event.button == "Start" then
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("TabChanged")
		elseif event.DeviceInput.button == "DeviceButton_space" then					-- add space to the string
			searchstring = searchstring.." "
		elseif event.DeviceInput.button == "DeviceButton_backspace"then
			searchstring = searchstring:sub(1, -2)								-- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			searchstring = ""
		elseif event.DeviceInput.button == "DeviceButton_="  then
			searchstring = searchstring.."="
		else
			for i=1,#englishes do														-- add standard characters to string
				if event.DeviceInput.button == "DeviceButton_"..englishes[i] then
					searchstring = searchstring..englishes[i]
				end
			end
		end
		MESSAGEMAN:Broadcast("UpdateString")
		whee:SongSearch(searchstring)
	end
end

local function DeterminePressed(self,element)
	local region_x = self:GetX() + element:GetX();
	local region_y = self:GetY() + element:GetY();
	local region_width = element:GetZoomedWidth();
	local region_height = element:GetZoomedHeight();
	
	local mouse_x = INPUTFILTER:GetMouseX();
	local mouse_y = INPUTFILTER:GetMouseY();
	
	if region_x - region_width/2 <= mouse_x and
		region_x + region_width/2 >= mouse_x and
		region_y - region_height/2 <= mouse_y and
		region_y + region_height/2 >= mouse_y then
		return true;
	else
		return false;
	end;
end;

local function UpdateSearchBar(self)
	if self == nil then
		return;
	end;
	if active then
			ms.ok("Song search activated")
			self:visible(true)
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
	else
			self:visible(false)
			self:queuecommand("Off")
			SCREENMAN:set_input_redirected(PLAYER_1, false)
	end
end;


local t = Def.ActorFrame{}

t[#t+1] = Def.ActorFrame{
	LeftClickMessageCommand = function(self, params)
		local c = self:GetChildren();
		if DeterminePressed(self,c.searchButton) then
			if mouseClickHandled == false then 
				if active == false then
					ms.ok("Song search activated")
					local c = self:GetChildren()
					active = true
					SCREENMAN:set_input_redirected(PLAYER_1, true)
					MESSAGEMAN:Broadcast("RefreshSearchResults")
				else
					local c = self:GetChildren()
					self:finishtweening()
					self:queuecommand("Off")
					active = false
					SCREENMAN:set_input_redirected(PLAYER_1, false)
				end
				mouseClickHandled = true
			end;
		end;
		return true;
	end;
	LoadActor(THEME:GetPathG("","SearchBar/searchButton")) .. {
		Name = "searchButton";
		InitCommand=cmd(xy,(SCREEN_WIDTH/2)+50,SCREEN_TOP+15;setsize,24,24);
		OnCommand=cmd();
	};
}




t[#t+1] = Def.ActorFrame{
	Name="SearchBar";
	
	BeginCommand=function(self)
		UpdateSearchBar(self)
		self:SetUpdateFunction( UpdateSearchBar )
		self:SetUpdateRate( 1/30 )
	end;
	
	OnCommand=function(self)
		local topScreen = SCREENMAN:GetTopScreen();
		--whee = topScreen:GetMusicWheel() --Idk why this gives an error
		whee = topScreen:GetChild("MusicWheel");
		topScreen:AddInputCallback(searchInput)
		self:visible(false)
	end,
	TabChangedMessageCommand = function(self)
		if active == false then
			ms.ok("Song search activated")
			local c = self:GetChildren()
			active = true
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
		else
			local c = self:GetChildren()
			self:finishtweening()
			self:queuecommand("Off")
			active = false
			SCREENMAN:set_input_redirected(PLAYER_1, false)
		end
		mouseClickHandled = false
	end;
	
	Def.Quad{
		InitCommand=cmd(xy,frameX+20,frameY-210;zoomto,400,300;halign,0;valign,0;diffuse,color("#333333CC"););
	};
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+250-capWideScale(get43size(120),30),frameY-90;zoom,0.7;halign,0.5;maxwidth,470),
		SetCommand=function(self) 
			if active then
				self:settext("Search Active:")
				self:diffuse(getGradeColor("Grade_Tier03"))
			else
				self:settext("Search Complete:")
				self:diffuse(byJudgment("TapNoteScore_Miss"))
			end
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+250-capWideScale(get43size(120),30),frameY-50;zoom,0.7;halign,0.5;maxwidth,470;settext,searchstring),
		SetCommand=function(self) 
			self:settext(searchstring)
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-200;zoom,0.4;halign,0),
		SetCommand=function(self) 
			self:settext("Start to lock search results.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-175;zoom,0.4;halign,0;settext,"Back to cancel search.";),
		SetCommand=function(self) 
			self:settext("Back to cancel search.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+20,frameY-150;zoom,0.4;halign,0;settext,"Delete resets search query.";),
		SetCommand=function(self) 
			self:settext("Delete resets search query.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Normal")..{
		InitCommand=cmd(xy,frameX+20,frameY+70;zoom,0.5;halign,0;settext,"Currently supports standard english alphabet only.";),
		SetCommand=function(self) 
			self:settext("Currently supports standard english alphabet only.")
		end,
		UpdateStringMessageCommand=cmd(queuecommand,"Set"),
	},
}

return t