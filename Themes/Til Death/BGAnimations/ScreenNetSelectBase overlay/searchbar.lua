-- Search functionalities
-- to be used in ScreenNetSelectMusic and/or ScreenSelectMusic

local Songs = SONGMAN:GetAllSongs();
local Results = {};
local currentResult = 1;

local function showResult(self, index)
	song = Results[index];
	local topScreen = SCREENMAN:GetTopScreen();
	local screenName = topScreen:GetName();
	if screenName == "ScreenNetSelectMusic" or screenName == "ScreenSelectMusic" then
		local MusicWheel = topScreen:GetChild("MusicWheel");
		if MusicWheel ~= nil then
			MusicWheel:SelectSong(song);
			MusicWheel:SelectSong(song); -- twice is better
			GAMESTATE:SetCurrentSong(song);
			MESSAGEMAN:Broadcast("CurrentSongChanged",nil);
			MusicWheel:Move(-1);
			MusicWheel:Move(1);
			MusicWheel:Move(0); -- lazy fix to trigger song previews
		end;
	end;
end;

function tablelength(T)
  local count = 0
  for _ in pairs(T) do count = count + 1 end
  return count
end

local function searchSong(self ,searchString)
	if string.len(searchString) < 3 then
		return;
	end;
	
	searchString = string.lower(searchString);
	Results = {};
	text = "";
	n = 0;
	for k,v in pairs(Songs) do
		title = string.lower(v:GetDisplayFullTitle());
		if string.find(title,searchString) ~= nil and
		v:GetStepsSeconds() > 0.0 then
			text = text .. title .. "\n";
			n = n + 1;
			key = {song = v, i=n}
			table.insert(Results, n, v);
		end;
	end;
	
	local c = self:GetChildren();
	
	if tablelength(Results) > 0 then
		if tablelength(Results) == 1 then
			c.textfield2:settext(tablelength(Results) .. " Result found for \""..searchString.."\"");
		else
			c.textfield2:settext(tablelength(Results) .. " Results found for \""..searchString.."\"");
		end;
		
		currentResult = 1;
		showResult(self, currentResult);
	else
		c.textfield2:settext("Sorry. There's no result for \""..searchString.."\" :(");
	end;
end;

local function DetermineHighlight(self,element,highlightedElement)
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
		highlightedElement:diffusealpha(1.0);
	else
		highlightedElement:diffusealpha(0.0);
	end;
end;

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


local mouseClickHandled = false;

local function UpdateSearchBar(self)

	if self == nil then
		return;
	end;
	local c = self:GetChildren();
	
	DetermineHighlight(self,c.frame,c.highlightedFrame);
	DetermineHighlight(self,c.searchButton,c.highlightedSearchButton);
	DetermineHighlight(self,c.leftButton,c.highlightedLeftButton);
	DetermineHighlight(self,c.rightButton,c.highlightedRightButton);
	
	if mouseClickHandled then
		mouseClickHandled = false;
	end;
end;

local leftShiftPressed = false;
local leftCtrlPressed = false;

local function input(event)

	local topScreen = SCREENMAN:GetTopScreen();
	local SearchBar = topScreen:GetChildren().Overlay:GetChildren().SearchBar;
	local c = SearchBar:GetChildren();
	
	if event.type == "InputEventType_FirstPress" then
		if event.DeviceInput.button == "DeviceButton_insert" and leftShiftPressed and leftCtrlPressed == false then
			c.textfield:settext("");
			local topScreen = SCREENMAN:GetTopScreen();
			local screenName = topScreen:GetName();
			if screenName == "ScreenNetSelectMusic" or screenName == "ScreenSelectMusic" then
				SCREENMAN:AddNewScreenToTop("ScreenTextEntry");
				local searchSettings = {
					Question = "Type to search the song. (Maximum character is 128):",
					MaxInputLength = 128,
					Password = false,
					OnOK = function(answer)
						MESSAGEMAN:Broadcast("SM_Search",{searchString = answer});
					end,
				};
				SCREENMAN:GetTopScreen():Load(searchSettings);
			end;
		end;
		
		if event.DeviceInput.button == "DeviceButton_pgup" and leftShiftPressed and leftCtrlPressed == false then
			local searchString = c.textfield:GetText();
			if searchString ~= "Press here to look up the song" then
				searchSong(SearchBar, searchString);
			end;
		end;
		
		if event.DeviceInput.button == "DeviceButton_insert" and leftCtrlPressed then
			if currentResult > 1 and currentResult <= tablelength(Results) then
				currentResult = currentResult - 1;
				showResult(SearchBar, currentResult);
			end;
		end;
		
		if event.DeviceInput.button == "DeviceButton_pgup" and leftCtrlPressed then
			if currentResult >= 1 and currentResult < tablelength(Results) then
				currentResult = currentResult + 1;
				showResult(SearchBar, currentResult);
			end;
		end;
		
		
	end;
	
	if event.DeviceInput.button == "DeviceButton_left shift" then
		if event.type == "InputEventType_Release" then
			leftShiftPressed = false;
		else
			leftShiftPressed = true;
		end;
	end;
	
	if event.DeviceInput.button == "DeviceButton_left ctrl" then
		if event.type == "InputEventType_Release" then
			leftCtrlPressed = false;
		else
			leftCtrlPressed = true;
		end;
	end;
	
	return true;
end

local textX = -(SCREEN_WIDTH*0.3)/2 + 20
local textY = 19

if not IsUsingWideScreen() == true then
textX = -(SCREEN_WIDTH*0.3)/2 - 40
end;

local t = Def.ActorFrame{

	Name="SearchBar";

	InitCommand=function(self)
		if IsUsingWideScreen() then
			self:xy(SCREEN_CENTER_X*1.5 - (SCREEN_WIDTH*0.3) + 28 + 10, SCREEN_TOP);
		else
			self:xy(SCREEN_CENTER_X*1.5 - (SCREEN_WIDTH*0.3) + 28 - 20, SCREEN_TOP);
		end;
		
		self:addy(-60);
		self:linear(0.0005);
		self:addy(1);
		self:linear(0.0005);
		self:addy(78);
	end;
	
	
	SM_SearchNowCommand = function(self,params)
		local c = self:GetChildren();
		local searchString = c.textfield:GetText();
		if searchString ~= "Click to find the song" then
			searchSong(self, searchString);
		end;
	end;
	
	SM_SearchMessageCommand = function(self,params)
		local c = self:GetChildren();
		c.textfield:settext(params.searchString);
	end;
	
	LeftClickMessageCommand = function(self, params)
		local c = self:GetChildren();
		
		if mouseClickHandled == false then 
		
			if DeterminePressed(self,c.frame) then
				c.textfield:settext("");
				local topScreen = SCREENMAN:GetTopScreen();
				local screenName = topScreen:GetName();
				if screenName == "ScreenNetSelectMusic" or screenName == "ScreenSelectMusic" then
					SCREENMAN:AddNewScreenToTop("ScreenTextEntry");
					local searchSettings = {
						Question = "Type to search the song. (Maximum character is 128):",
						MaxInputLength = 128,
						OnOK = function(answer)
							MESSAGEMAN:Broadcast("SM_Search",{searchString = answer});
						end,
					};
					SCREENMAN:GetTopScreen():Load(searchSettings);
				end;
			end;
			
			if DeterminePressed(self,c.searchButton) then
				local searchString = c.textfield:GetText();
				if searchString ~= "Click to find the song" then
					searchSong(self, searchString);
				end;
			end;
			
			if DeterminePressed(self,c.leftButton) then
				if currentResult > 1 and currentResult <= tablelength(Results) then
					currentResult = currentResult - 1;
					showResult(self, currentResult);
				end;
			end;
			
			if DeterminePressed(self,c.rightButton) then
				if currentResult >= 1 and currentResult < tablelength(Results) then
					currentResult = currentResult + 1;
					showResult(self, currentResult);
				end;
			end;
			
			mouseClickHandled = true;
			
		end;
		
		return true;
		
	end;
	
	LoadActor(THEME:GetPathG("","SearchBar/searchButton")) .. {
		Name = "searchButton";
		InitCommand=cmd(xy,(SCREEN_WIDTH*0.3)/2 + 18,0;setsize,24,24);
		OnCommand=cmd();
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/highlightedSearchButton")) .. {
		Name = "highlightedSearchButton";
		InitCommand=cmd(xy,(SCREEN_WIDTH*0.3)/2 + 18,0;setsize,24,24;diffusealpha,0.0);
		OnCommand=cmd();
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/leftButton")) .. {
		Name = "leftButton";
		InitCommand=cmd(xy,(SCREEN_WIDTH*0.3)/2 + 18 + 28 ,0;setsize,24,24);
		OnCommand=cmd();
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/highlightedLeftButton")) .. {
		Name = "highlightedLeftButton";
		InitCommand=cmd(xy,(SCREEN_WIDTH*0.3)/2 + 18 + 28 ,0;setsize,24,24;diffusealpha,0.0);
		OnCommand=cmd();
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/rightButton")) .. {
		Name = "rightButton";
		InitCommand=cmd(xy,(SCREEN_WIDTH*0.3)/2 + 18 + 28 + 28, 0;setsize,24,24);
		OnCommand=cmd();
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/highlightedRightButton")) .. {
		Name = "highlightedRightButton";
		InitCommand=cmd(xy,(SCREEN_WIDTH*0.3)/2 + 18 + 28 + 28, 0;setsize,24,24;diffusealpha,0.0);
		OnCommand=cmd();
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/frame")) .. {
		Name = "frame";
		InitCommand=cmd(setsize,(SCREEN_WIDTH*0.3),24);
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/highlightedFrame")) .. {
		Name = "highlightedFrame";
		InitCommand=cmd(setsize,(SCREEN_WIDTH*0.3),24;diffusealpha,0.0);
	};
	
	LoadActor(THEME:GetPathG("","SearchBar/focusedFrame")) .. {
		Name = "focusedFrame";
		InitCommand=cmd(setsize,(SCREEN_WIDTH*0.3),24;diffusealpha,0.0);
	};
	
	LoadFont("Common normal") .. {
		Name="textfield";
		InitCommand=cmd(zoom,0.6;horizalign,center);
		OnCommand=cmd(diffuse,color("0.7,0.7,0.7,1");strokecolor,color("#000000"));
		Text="Click to find the song";
	};
		
	LoadFont("Common normal") .. {
		Name="textfield2";
		InitCommand=cmd(xy,textX+(SCREEN_WIDTH*0.2),textY;zoom,0.5;horizalign,center);
		OnCommand=cmd(halign,0;diffuse,color("0.7,0.7,0.7,1");strokecolor,color("#000000"));
		Text="";
	};
	
	BeginCommand=function(self)
		self:SetUpdateFunction( UpdateSearchBar );
		self:SetUpdateRate( 1/30 );
	end;
};

t[#t+1] = Def.ActorFrame {
	BeginCommand = function(self)
	
		if IsUsingWideScreen() == false then
			local topScreen = SCREENMAN:GetTopScreen();
			if topScreen then
				local screenName = topScreen:GetName();
				if screenName == "ScreenNetSelectMusic" or screenName == "ScreenSelectMusic" then
					local children = topScreen:GetChildren();
					children.ChatOutput:addx(-15);
					children.ChatInput:addx(-15);
					
					children.StepsDisplayP1:addx(67);
					children.StepsDisplayP1:GetChildren().Frame:zoomx(0.82);
					children.StepsDisplayP1:GetChildren().Frame:zoomy(1.0);
					children.StepsDisplayP1:GetChildren().Description:zoom(0.82);
					children.StepsDisplayP1:GetChildren().Meter:zoom(0.82);
					children.StepsDisplayP1:GetChildren().Meter:addx(-4);
					--children.StepsDisplayP1:GetChildren().StepsType:zoom(0.82); 		child doesn't exist? - mina
				end;
			end;
		end;
		
		local screen= SCREENMAN:GetTopScreen()
		screen:AddInputCallback(input)
	end;
};
return t;