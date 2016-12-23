--Similar to dummy.lua and I'm just commenting again because I don't know what I am exactly doing. -Misterkister

local update = false
local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false);
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0;); -- visible(false) doesn't seem to work with sleep
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1;);
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 0 then
			self:queuecommand("On");
			self:visible(true)
			update = true
		else 
			self:queuecommand("Off");
			update = false
		end;
	end;
	TabChangedMessageCommand=cmd(queuecommand,"Set");
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set");
};

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320,400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX = 10
local offsetY = 20
local pn = GAMESTATE:GetEnabledPlayers()[1]

-- t[#t+1] = Def.Quad{
	-- InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"));
-- };

-- t[#t+1] = Def.Quad{
	-- InitCommand=cmd(xy,frameX+5,frameY+offsetY+5;zoomto,150,150*3/4;halign,0;valign,0;diffuse,color("#000000CC"));
-- };

-- t[#t+1] = Def.Sprite {
	-- InitCommand=cmd(xy,frameX,frameY+offsetY-75;diffusealpha,0.8;);
	-- Name="BG";
	-- SetCommand=function(self)
		-- if update then
			-- self:finishtweening()
			-- self:sleep(0.25)
			-- local song = GAMESTATE:GetCurrentSong()
			
			-- if song then
				-- if song:HasJacket() then
					-- self:visible(true);
					-- self:Load(song:GetJacketPath())
				-- elseif song:HasBackground() then
					-- self:visible(true)
					-- self:Load(song:GetBackgroundPath())
				-- else
					-- self:visible(false)
				-- end
			-- else
				-- self:visible(false)
			-- end;
			-- self:scaletofit(frameX+5,frameY+5+offsetY,frameX+150+5,frameY+150*3/4+offsetY+5)
			-- self:y(frameY+5+offsetY+150*3/8)
			-- self:x(frameX+75+5)
			-- self:smooth(0.5)
			-- self:diffusealpha(0.8)
		-- end
	-- end;
	-- BeginCommand=cmd(queuecommand,"Set");
	-- CurrentSongChangedMessageCommand=cmd(finishtweening;smooth,0.5;diffusealpha,0;sleep,0.35;queuecommand,"Set");
-- };

local cdtitleX = frameX+440
local cdtitleY = frameY+225

if not IsUsingWideScreen() == true then
cdtitleX = frameX+440-105
cdtitleY = frameY+225
end;

t[#t+1] = Def.Sprite {
		InitCommand=cmd(xy,cdtitleX,cdtitleY+10;zoomy,0;valign,1);
		Name="CDTitle";
		SetCommand=function(self)
			if update then
				local song = GAMESTATE:GetCurrentSong()	
				if song then
					if song:HasCDTitle() then
						self:visible(true)
						self:Load(song:GetCDTitlePath())
					else
						self:visible(false)
					end
				else
					self:visible(false)
				end;
				local height = self:GetHeight()
				local width = self:GetWidth()
				
				if height >= 80 and width >= 100 then
					if height*(100/80) >= width then
					self:zoom(80/height)
					else
					self:zoom(100/width)
					end
				elseif height >= 80 then
					self:zoom(80/height)
				elseif width >= 100 then
					self:zoom(100/width)
				else
					self:zoom(1)
				end
				self:diffusealpha(1)
			end
		end;
	BeginCommand=cmd(queuecommand,"Set");
	CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
};
		

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="StepsAndMeter";
	-- InitCommand=cmd(xy,frameX+frameWidth-offsetX,frameY+offsetY+10;zoom,0.5;halign,1;);
	-- SetCommand=function(self)
		-- local steps = GAMESTATE:GetCurrentSteps(pn)
		-- if steps ~= nil and update then
			-- local diff = getDifficulty(steps:GetDifficulty())
			-- local stype = ToEnumShortString(steps:GetStepsType()):gsub("%_"," ")
			-- local meter = steps:GetMeter()
			-- self:settext(stype.." "..diff.." "..meter)
			-- self:diffuse(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(),steps:GetDifficulty())))
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
	-- CurrentStepsP1ChangedMessageCommand=cmd(queuecommand,"Set");
	-- CurrentStepsP2ChangedMessageCommand=cmd(queuecommand,"Set");
-- };

local stepstuffX = frameX+frameWidth-offsetX-300
local stepstuffY = frameY+offsetY-23

if not IsUsingWideScreen() == true then
stepstuffX = frameX+frameWidth-offsetX+30
stepstuffY = frameY+offsetY-13 + 110
end;

t[#t+1] = LoadFont("Common Normal")..{
	Name="StepsAndMeter";
	InitCommand=cmd(xy,stepstuffX,stepstuffY;zoom,0.4;halign,1;);
	SetCommand=function(self)
		local steps = GAMESTATE:GetCurrentSteps(pn)
		local song = GAMESTATE:GetCurrentSong()
		local notecount = 0
		local length = 1
		if steps ~= nil and song ~= nil and update then
			length = song:GetStepsSeconds()
			notecount = steps:GetRadarValues(pn):GetValue("RadarCategory_Notes")
			self:settext(string.format("%0.2f Average NPS",notecount/length))
			self:diffuse(Saturation(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(),steps:GetDifficulty())),0.3))
		else
			self:settext("0.00 Average NPS")
		end
	end;
	CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
	CurrentStepsP1ChangedMessageCommand=cmd(queuecommand,"Set");
	CurrentStepsP2ChangedMessageCommand=cmd(queuecommand,"Set");
};

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Song Title";
	-- InitCommand=cmd(xy,frameX+offsetX+150,frameY+offsetY+45;zoom,0.6;halign,0;maxwidth,((frameWidth-offsetX*2-150)/0.6)-40);
	-- SetCommand=function(self)
		-- if update then
			-- local song = GAMESTATE:GetCurrentSong()
			-- if song ~= nil then
				-- self:settext(song:GetDisplayMainTitle())
				-- self:diffuse(color("#FFFFFF"))
			-- else
				-- self:settext("Not Available")
				-- self:diffuse(getMainColor("disabled"))
			-- end
			-- self:GetParent():GetChild("Song Length"):x(math.min(((frameWidth-offsetX*2-capWideScale(120,150))/0.6),self:GetWidth()*0.6+self:GetX()+5))
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };
-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Song Length";
	-- InitCommand=cmd(xy,frameX+offsetX+150,frameY+offsetY+44;zoom,0.4;halign,0;maxwidth,(frameWidth-offsetX*2-150)/0.6);
	-- SetCommand=function(self)
		-- if update then
			-- local song = GAMESTATE:GetCurrentSong()
			-- local length = 0
			-- if song ~= nil then
				-- length = song:GetStepsSeconds()
				-- self:visible(true)
			-- end
			-- self:settext(string.format("%s",SecondsToMSS(length)))
			-- self:diffuse(getSongLengthColor(length))
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };
-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Song SubTitle";
	-- InitCommand=cmd(xy,frameX+offsetX+150,frameY+offsetY+60;zoom,0.4;halign,0;maxwidth,(frameWidth-offsetX*2-150)/0.4);
	-- SetCommand=function(self)
		-- if update then
			-- local song = GAMESTATE:GetCurrentSong()
			-- if song ~= nil then
				-- self:visible(true)
				-- self:settext(song:GetDisplaySubTitle())
			-- else
				-- self:visible(false)
			-- end
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };
-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Song Artist";
	-- InitCommand=cmd(xy,frameX+offsetX+150,frameY+offsetY+73;zoom,0.4;halign,0;maxwidth,(frameWidth-offsetX*2-150)/0.4);
	-- SetCommand=function(self)
		-- local song = GAMESTATE:GetCurrentSong()
		-- if song ~= nil then
			-- self:visible(true)
			-- self:settext(song:GetDisplayArtist())
			-- self:diffuse(color("#FFFFFF"))
			-- if #song:GetDisplaySubTitle() == 0 then
				-- self:y(frameY+offsetY+60)
			-- else
				-- self:y(frameY+offsetY+73)
			-- end
		-- else
			-- self:y(frameY+offsetY+60)
			-- self:settext("Not Available")
			-- self:diffuse(getMainColor("disabled"))
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Song BPM";
	-- InitCommand=cmd(xy,frameX+offsetX+150,frameY+offsetY+130;zoom,0.4;halign,0;maxwidth,(frameWidth-offsetX*2-150)/0.4);
	-- SetCommand=function(self)
		-- local song = GAMESTATE:GetCurrentSong()
		-- local bpms = {0,0}
		-- if song ~= nil then
			-- bpms = song:GetTimingData():GetActualBPM()
			-- for k,v in pairs(bpms) do
				-- bpms[k] = math.round(bpms[k])
			-- end
			-- self:visible(true)
			-- if bpms[1] == bpms[2] and bpms[1]~= nil then
				-- self:settext(string.format("BPM: %d",bpms[1]))
			-- else
				-- self:settext(string.format("BPM: %d-%d (%d)",bpms[1],bpms[2],getCommonBPM(song:GetTimingData():GetBPMsAndTimes(true),song:GetLastBeat())))
			-- end
			-- self:diffuse(color("#FFFFFF"))
		-- else
			-- self:settext("Not Available")
			-- self:diffuse(getMainColor("disabled"))
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="BPM Change Count";
	-- InitCommand=cmd(xy,frameX+offsetX+125,frameY+offsetY-15;zoom,0.4;halign,0;maxwidth,(frameWidth-offsetX*2-150)/0.4);
	-- SetCommand=function(self)
		-- local song = GAMESTATE:GetCurrentSong()
		-- local bpms = {0,0}
		-- if song ~= nil then
			-- self:settext(string.format("BPM Changes: %d",getBPMChangeCount(song:GetTimingData():GetBPMsAndTimes(true))))
			-- self:diffuse(color("#FFFFFF"))
		-- else
			-- self:settext("Not Available")
			-- self:diffuse(getMainColor("disabled"))
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };

local radarValues = {
	{'RadarCategory_Notes','Notes'},
	{'RadarCategory_TapsAndHolds','Taps'},
	{'RadarCategory_Jumps','Jumps'},
	{'RadarCategory_Hands','Hands'},
	{'RadarCategory_Holds','Holds'},
	{'RadarCategory_Rolls','Rolls'},
	{'RadarCategory_Mines','Mines'},
	{'RadarCategory_Lifts','Lifts'},
	{'RadarCategory_Fakes','Fakes'},
}

local radarX = frameX+offsetX+450
local stuffstuffstuffX = frameX+offsetX+400

if not IsUsingWideScreen() == true then
radarX = frameX+offsetX+450-120
stuffstuffstuffX = frameX+offsetX+400-110
end;

for k,v in ipairs(radarValues) do
	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(xy,stuffstuffstuffX,frameY+offsetY+230+(15*(k-1));zoom,0.4;halign,0;maxwidth,(frameWidth-offsetX*2-150)/0.4);
		OnCommand=function(self)
			self:settext(v[2]..": ")
		end;
	}
	t[#t+1] = LoadFont("Common Normal")..{
		Name="RadarValue"..v[1];
		InitCommand=cmd(xy,radarX,frameY+offsetY+230+(15*(k-1));zoom,0.4;halign,0;maxwidth,(frameWidth-offsetX*2-150)/0.4);
		SetCommand=function(self)
			local song = GAMESTATE:GetCurrentSong()
			local steps = GAMESTATE:GetCurrentSteps(pn)
			local count = 0
			if song ~= nil and steps ~= nil and update then
				count = steps:GetRadarValues(pn):GetValue(v[1])
				self:settext(count)
				self:diffuse(color("#FFFFFF"))
			else
				self:settext(0)
				self:diffuse(getMainColor("disabled"))
			end
		end;
		CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
		CurrentStepsP1ChangedMessageCommand=cmd(queuecommand,"Set");
		CurrentStepsP2ChangedMessageCommand=cmd(queuecommand,"Set");
	}
end

-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+offsetX,frameY+frameHeight-10-distY*2;zoom,fontScale;halign,0;);
	-- BeginCommand=function(self)
		-- self:settext("Path:")
	-- end
-- }

-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+offsetX+35,frameY+frameHeight-10-distY*2;zoom,fontScale;halign,0;maxwidth,(frameWidth-35-offsetX-10)/fontScale);
	-- BeginCommand=cmd(queuecommand,"Set");
	-- SetCommand=function(self)
		-- if update then
			-- local song = GAMESTATE:GetCurrentSong()
			-- if song ~= nil then
				-- self:settext(song:GetSongDir())
				-- self:diffuse(color("#FFFFFF"))
			-- else
				-- self:settext("Not Available")
				-- self:diffuse(getMainColor("disabled"))
			-- end
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- }



-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+offsetX,frameY+frameHeight-10-distY;zoom,fontScale;halign,0;);
	-- BeginCommand=function(self)
		-- self:settext("SHA-1:")
	-- end
-- }

-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+offsetX+35,frameY+frameHeight-10-distY;zoom,fontScale;halign,0;maxwidth,(frameWidth-35)/fontScale);
	-- BeginCommand=cmd(queuecommand,"Set");
	-- SetCommand=function(self)
		-- if update then
			-- local song = GAMESTATE:GetCurrentSong()
			-- local pn = GAMESTATE:GetEnabledPlayers()[1]
			-- local step = GAMESTATE:GetCurrentSteps(pn)
			-- if song ~= nil then
				-- self:diffuse(color("#FFFFFF"))
				-- self:settext(SHA1FileHex(step:GetFilename()))
			-- else
				-- self:settext("Not Available")
				-- self:diffuse(getMainColor("disabled"))
			-- end
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };

-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+offsetX,frameY+frameHeight-10;zoom,fontScale;halign,0;);
	-- BeginCommand=function(self)
		-- self:settext("MD5:")
	-- end
-- }

--Not important. -Misterkister
-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+frameWidth/2,frameY+frameHeight-75;zoom,fontScale;);
	-- BeginCommand=function(self)
		-- self:settext("More to be added soon(TM)....ish")
		-- self:diffusealpha(0.2)
	-- end
-- }

-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+offsetX+35,frameY+frameHeight-10;zoom,fontScale;halign,0;maxwidth,(frameWidth-35)/fontScale);
	-- BeginCommand=cmd(queuecommand,"Set");
	-- SetCommand=function(self)
		-- if update then
			-- local song = GAMESTATE:GetCurrentSong()
			-- local pn = GAMESTATE:GetEnabledPlayers()[1]
			-- local step = GAMESTATE:GetCurrentSteps(pn)
			-- if song ~= nil then
				-- self:diffuse(color("#FFFFFF"))
				-- self:settext(MD5FileHex(step:GetFilename()))
			-- else
				-- self:settext("Not Available")
				-- self:diffuse(getMainColor("disabled"))
			-- end
		-- end
	-- end;
	-- CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
-- };

-- t[#t+1] = Def.Quad{
	-- InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,color("#000000"));
-- };

-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor(1));
	-- BeginCommand=cmd(settext,"Simfile Info")
-- };

return t