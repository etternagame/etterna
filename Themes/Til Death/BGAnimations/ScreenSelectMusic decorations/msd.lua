--Local vars
local update = false
local steps
local song
local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320,400)
local frameHeight = 350
local fontScale = 0.4
local distY = 15
local offsetX = 10
local offsetY = 20
local pn = GAMESTATE:GetEnabledPlayers()[1]
local greatest = 0
local steps
local meter = {}
meter[1] = 0.00

--Actor Frame
local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false),
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0),
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1),
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 1 then
			self:queuecommand("On")
			self:visible(true)
			song = GAMESTATE:GetCurrentSong()
			steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
			
			--Find max MSD value, store MSD values in meter[]
			-- I plan to have c++ store the highest msd value as a separate variable to aid in the filter process so this won't be needed afterwards - mina
			greatest=0
			if song and steps then
				for i=1,#ms.SkillSets do 
					meter[i+1] = steps:GetMSD(getCurRateValue(), i)
					if meter[i+1] > meter[greatest+1] then
						greatest = i
					end
				end
			end
			
			MESSAGEMAN:Broadcast("UpdateMSDInfo")
			update = true
		else 
			self:queuecommand("Off")
			update = false
		end
	end,
	CurrentRateChangedMessageCommand=cmd(queuecommand,"Set"),
	RefreshChartInfoMessageCommand=cmd(queuecommand,"Set"),
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
}

--BG quad
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}

--Skillset label function
local function littlebits(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+35,frameY+120 + 22*i;halign,0;valign,0;zoom,0.5;maxwidth,110/0.6),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			--skillset name
			if song and steps then
				self:settext(ms.SkillSets[i]..":")
			else
				self:settext("")
			end
			--highlight
			if greatest == i then
				self:diffuse(getMainColor('positive'))
			else
				self:diffuse(getMainColor('negative'))
			end
			--If negative BPM empty label
			if steps and steps:GetTimingData():HasWarps() then
				self:settext("");
			end
		end,
		UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameX+225,frameY+120 + 22*i;halign,1;valign,0;zoom,0.5;maxwidth,110/0.6),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			if song and steps then
				self:settextf("%05.2f",meter[i+1])
				self:diffuse(ByMSD(meter[i+1]))
			else
				self:settext("")
			end
			--If negative BPM empty label
			if steps and steps:GetTimingData():HasWarps() then
				self:settext("");
			end
		end,
		CurrentRateChangedMessageCommand=cmd(queuecommand,"Set"),
		UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
		}
	}
	return t
end

--Song Title
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}
t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"MSD Breakdown (Wip)")}
t[#t+1] = LoadFont("Common Large")..{
	InitCommand=cmd(xy,frameX+5,frameY+35;zoom,0.6;halign,0;diffuse,getMainColor('positive');maxwidth,SCREEN_CENTER_X/0.7),
	SetCommand=function(self)
		if song then
			self:settext(song:GetDisplayMainTitle())
		else
			self:settext("")
		end
	end,
	UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
}


-- Music Rate Display
t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=cmd(xy,frameX+frameWidth-100,frameY+offsetY+65;visible,true;halign,0;zoom,0.4;maxwidth,capWideScale(get43size(360),360)/capWideScale(get43size(0.45),0.45)),
	SetCommand=function(self)
		self:settext(getCurRateDisplayString())
	end,
	CurrentRateChangedCommand=cmd(queuecommand,"set")
}

--Difficulty
t[#t+1] = LoadFont("Common Normal")..{
	Name="StepsAndMeter",
	InitCommand=cmd(xy,frameX+frameWidth-offsetX,frameY+offsetY+50;zoom,0.5;halign,1),
	SetCommand=function(self)
		steps = GAMESTATE:GetCurrentSteps(pn)
		if steps ~= nil then
			local diff = getDifficulty(steps:GetDifficulty())
			local stype = ToEnumShortString(steps:GetStepsType()):gsub("%_"," ")
			local meter = steps:GetMeter()
			if update then
				self:settext(stype.." "..diff.." "..meter)
				self:diffuse(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(),steps:GetDifficulty())))
			end
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}


--NPS
t[#t+1] = LoadFont("Common Normal")..{
	Name="NPS";
	InitCommand=cmd(xy,frameX+frameWidth-15,frameY+60;zoom,0.4;halign,1;);
	SetCommand=function(self)
		steps = GAMESTATE:GetCurrentSteps(pn)
		--local song = GAMESTATE:GetCurrentSong()
		local notecount = 0
		local length = 1
		if steps ~= nil and song ~= nil and update then
			length = song:GetStepsSeconds()
			notecount = steps:GetRadarValues(pn):GetValue("RadarCategory_Notes")
			self:settext(string.format("%0.2f Average NPS",notecount/length * getCurRateValue()))
			self:diffuse(Saturation(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(),steps:GetDifficulty())),0.3))
		else
			self:settext("0.00 Average NPS")
		end
	end;
	CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
	CurrentStepsP1ChangedMessageCommand=cmd(queuecommand,"Set");
	CurrentStepsP2ChangedMessageCommand=cmd(queuecommand,"Set");
};

--Negative BPMs label
t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX+45,frameY+135;zoom,0.8;halign,0;diffuse,getMainColor('negative');settext,"Negative Bpms";),
	SetCommand=function(self)
		if steps and steps:GetTimingData():HasWarps() then
			self:settext("Negative Bpms");
		else
			self:settext("");
		end
	end,
	UpdateMSDInfoCommand=cmd(queuecommand,"Set"),
}

--Skillset labels
for i=1,#ms.SkillSets do 
	t[#t+1] = littlebits(i)
end

return t
