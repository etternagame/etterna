-- Search functionalities
-- to be used in ScreenNetSelectMusic and/or ScreenSelectMusic
-- Now uses Til Death's search system

local searchstring = ""
local englishes = {"a", "b", "c", "d", "e","f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"}
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)
local active = false
local whee
local mouseClickHandled = false

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

local t = Def.ActorFrame{}


local mousetwoClickHandled = false
t[#t+1] = Def.ActorFrame{
	OnCommand=function(self)
		local topScreen = SCREENMAN:GetTopScreen();
		--whee = topScreen:GetMusicWheel() --Idk why this gives an error
		whee = topScreen:GetChild("MusicWheel");
	end,
	LeftClickMessageCommand = function(self, params)
		local c = self:GetChildren();
		if mousetwoClickHandled == false then
			if DeterminePressed(self,c.searchButtonn) then
				SCREENMAN:AddNewScreenToTop("ScreenTextEntry");
				mousetwoClickHandled = true
				local searchSettings = {
					Question = "Type to search the song. (Maximum character is 128):",
					MaxInputLength = 128,
					OnOK = function(answer)
						searchstring = answer
						MESSAGEMAN:Broadcast("SM_Search",{searchstring});
						MESSAGEMAN:Broadcast("UpdateString")
						whee:SongSearch(searchstring)
						mousetwoClickHandled = false
					end,
					OnCancel = function()
						searchstring = ''
						MESSAGEMAN:Broadcast("SM_Search",{searchstring});
						MESSAGEMAN:Broadcast("UpdateString")
						whee:SongSearch(searchstring)
						mousetwoClickHandled = false
					end,
				};
				SCREENMAN:GetTopScreen():Load(searchSettings);
			end;
		end;
	end;
	LoadActor(THEME:GetPathG("","SearchBar/searchButton")) .. {
		Name = "searchButtonn";
		InitCommand=cmd(xy,(SCREEN_WIDTH/2)+50,SCREEN_TOP+15;setsize,24,24);
		OnCommand=cmd();
	};
}


return t