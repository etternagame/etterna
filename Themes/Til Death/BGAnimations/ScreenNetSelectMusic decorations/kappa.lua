local update = false

local hsTable
local rtTable
local rates
local rateIndex = 1
local scoreIndex = 1
local score
local pn = GAMESTATE:GetEnabledPlayers()[1]

local defaultRateText = ""
if themeConfig:get_data().global.RateSort then
	defaultRateText = "1.0x"
else
	defaultRateText = "All"
end;

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
			self:playcommand("InitScore")
		else 
			self:queuecommand("Off");
			update = false
		end;
	end;
	TabChangedMessageCommand=cmd(queuecommand,"Set");
	CodeMessageCommand=function(self,params)
		if update then
			if params.Name == "NextRate" then
				rateIndex = ((rateIndex)%(#rates))+1
				scoreIndex = 1
			elseif params.Name == "PrevRate" then
				rateIndex = ((rateIndex-2)%(#rates))+1
				scoreIndex = 1
			elseif params.Name == "NextScore" then
				if rtTable[rates[rateIndex]] ~= nil then
					scoreIndex = ((scoreIndex)%(#rtTable[rates[rateIndex]]))+1
				end
			elseif params.Name == "PrevScore" then
				if rtTable[rates[rateIndex]] ~= nil then
					scoreIndex = ((scoreIndex-2)%(#rtTable[rates[rateIndex]]))+1
				end
			end;
			if rtTable[rates[rateIndex]] ~= nil then
				score = rtTable[rates[rateIndex]][scoreIndex]
				MESSAGEMAN:Broadcast("ScoreUpdate")
			end;
		end;
	end;
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set");
	CurrentSongChangedMessageCommand=cmd(queuecommand,"Set");
	CurrentStepsP1ChangedMessageCommand=cmd(queuecommand,"Set");
	CurrentStepsP2ChangedMessageCommand=cmd(queuecommand,"Set");
	InitScoreCommand=function(self)
			if GAMESTATE:GetCurrentSong() ~= nil then
				hsTable = getScoreList(pn)
				if hsTable ~= nil and hsTable[1] ~= nil then
					rtTable = getRateTable(hsTable)
					rates,rateIndex = getUsedRates(rtTable)
					scoreIndex = 1
					score = rtTable[rates[rateIndex]][scoreIndex]
				else
					rtTable = {}
					rates,rateIndex = {defaultRateText},1
					scoreIndex = 1
					score = nil
				end;
			else
				hsTable = {}
				rtTable = {}
				rates,rateIndex = {defaultRateText},1
				scoreIndex = 1
				score = nil
			end;
			MESSAGEMAN:Broadcast("ScoreUpdate")
	end;
};

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320,400)
local frameHeight = 350
local fontScale = 0.4
local offsetX = 10
local offsetY = 20

local judges = {'TapNoteScore_W1','TapNoteScore_W2','TapNoteScore_W3','TapNoteScore_W4','TapNoteScore_W5','TapNoteScore_Miss','HoldNoteScore_Held','HoldNoteScore_LetGo'}

-- t[#t+1] = Def.Quad{
	-- InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"));
-- };

-- t[#t+1] = Def.Quad{
	-- InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5);
-- };

-- t[#t+1] = LoadFont("Common Normal")..{
	-- InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive'));
	-- BeginCommand=cmd(settext,"Score Info")
-- };

-- t[#t+1] = LoadFont("Common Large")..{
	-- Name="Grades";
	-- InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+20;zoom,0.6;halign,0;maxwidth,50/0.6);
	-- SetCommand=function(self)
		-- if score ~= nil then
			-- self:settext(THEME:GetString("Grade",ToEnumShortString(score:GetGrade())))
			-- self:diffuse(getGradeColor(score:GetGrade()))
		-- else
			-- self:settext("")
		-- end;
	-- end;
	-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
-- };

--Just putting functions here because I need to fix this theme. -Misterkister
local scorekappaX = SCREEN_CENTER_X-10
local scorekappaY = SCREEN_CENTER_Y-50

if not IsUsingWideScreen() == true then
scorekappaX = SCREEN_CENTER_X-300
scorekappaY = SCREEN_CENTER_Y-65
end;

t[#t+1] = LoadFont("Common Normal")..{
	Name="Score";
	InitCommand=cmd(xy,scorekappaX,scorekappaY;zoom,0.5;halign,0;);
	SetCommand=function(self)
		if score ~= nil then
			local curscore = getScore(score,0)
			local maxscore = getMaxScore(pn,0)	
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end;
			local pscore = (curscore/maxscore)

			self:settextf("%05.2f%% (%s)",math.floor((pscore)*10000)/100,scoringToText(0))
		else
			self:settextf("00.00%% (%s)",scoringToText(0))
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

--Just putting functions here because I need to fix this theme. -Misterkister
local rawX = SCREEN_CENTER_X-150

if not IsUsingWideScreen() == true then
rawX = SCREEN_CENTER_X-310
end;

t[#t+1] = LoadFont("Common Normal")..{
	Name="RawScore";
	InitCommand=cmd(xy,rawX,SCREEN_CENTER_Y-50;zoom,0.4;halign,0;);
	SetCommand=function(self)
		if score ~= nil then
			local curscore = getScore(score,0)
			local dpscore = getScore(score,1)
			local grade = getScoreGrade(score)
			local neargrade,diff = getNearbyGrade(pn,dpscore,grade)
			local maxscore = getMaxScore(pn,0)
			if diff >= 0 then
				diff = tostring("+"..diff)
			else
				diff = tostring(diff)
			end;	
			if maxscore == 0 or maxscore == nil then
				maxscore = 1
			end;

			self:settextf("%04d/%04d (%s %s)",curscore,maxscore,THEME:GetString("Grade",ToEnumShortString(neargrade)),diff)
		else
			self:settext("0000/0000 (D +0)")
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

local clearmaniaX = SCREEN_CENTER_X-400
local clearmaniaY = SCREEN_CENTER_Y-52

if not IsUsingWideScreen() == true then
clearmaniaX = SCREEN_CENTER_X-300
clearmaniaY = SCREEN_CENTER_Y-80
end;

t[#t+1] = LoadFont("Common Normal")..{
	Name="ClearType";
	InitCommand=cmd(xy,clearmaniaX,clearmaniaY;zoom,0.5;halign,0);
	SetCommand=function(self)
		self:settext(getClearTypeFromScore(pn,score,0))
		self:diffuse(getClearTypeFromScore(pn,score,2))
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

local combomaxX = frameX+offsetX-310
local combomaxY = frameY+offsetY-108

if not IsUsingWideScreen() == true then
combomaxX = frameX+offsetX-290
combomaxY = frameY+offsetY+58-30
end;

t[#t+1] = LoadFont("Common Normal")..{
	Name="Combo";
	InitCommand=cmd(xy,combomaxX+395,combomaxY+85;zoom,0.4;halign,0);
	SetCommand=function(self)
		if score ~= nil then
			local maxCombo = getScoreMaxCombo(score)
			self:settextf("Max Combo: %d",maxCombo)
		else
			self:settext("Max Combo: 0")
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

local misscrankX = frameX+offsetX+70
local misscrankY = frameY+offsetY+125

if not IsUsingWideScreen() == true then
misscrankY = frameY+offsetY+123 - 28
end;

t[#t+1] = LoadFont("Common Normal")..{
	Name="MissCount";
	InitCommand=cmd(xy,misscrankX,misscrankY;zoom,0.4;halign,0);
	SetCommand=function(self)
		if score ~= nil then
			local missCount = getScoreMissCount(score)
			if missCount ~= nil then
				self:settext("Miss Count: "..missCount)
			else
				self:settext("Miss Count: -")
			end
		else
			self:settext("Miss Count: -")
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

local datestuffY = frameY+offsetY+88
local datestuffX = frameX+offsetX

if IsUsingWideScreen() == true then
datestuffY = frameY+offsetY+88-100
datestuffX = frameX+offsetX-10
end;

if not IsUsingWideScreen() == true then
datestuffY = frameY+offsetY+88-100
datestuffX = frameX+offsetX-10
end;

t[#t+1] = LoadFont("Common Normal")..{
	Name="Date";
	InitCommand=cmd(xy,datestuffX,datestuffY;zoom,0.4;halign,0);
	SetCommand=function(self)
		if score ~= nil then
			self:settext("Date Achieved: "..getScoreDate(score))
		else
			self:settext("Date Achieved: ")
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Mods";
	-- InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+103;zoom,0.4;halign,0);
	-- SetCommand=function(self)
		-- if score ~= nil then
			-- self:settext("Mods: " ..score:GetModifiers())
		-- else
			-- self:settext("Mods:")
		-- end;
	-- end;
	-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
-- };

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Mods";
	-- InitCommand=cmd(xy,frameX+offsetX,frameY+frameHeight-10;zoom,0.4;halign,0);
	-- SetCommand=function(self)
		-- if ghostDataExists(pn,score) then
			-- self:settext("Ghost Data Available")
			-- self:diffuse(getMainColor('enabled'))
		-- else
			-- self:settext("Ghost Data Unavailable")
			-- self:diffuse(getMainColor('disabled'))
		-- end;
	-- end;
	-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
-- };

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="StepsAndMeter";
	-- InitCommand=cmd(xy,frameX+frameWidth-offsetX,frameY+offsetY+10;zoom,0.5;halign,1;);
	-- SetCommand=function(self)
		-- local steps = GAMESTATE:GetCurrentSteps(pn)
		-- if steps ~= nil then
			-- local diff = getDifficulty(steps:GetDifficulty())
			-- local stype = ToEnumShortString(steps:GetStepsType()):gsub("%_"," ")
			-- local meter = steps:GetMeter()
			-- if update then
				-- self:settext(stype.." "..diff.." "..meter)
				-- self:diffuse(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(),steps:GetDifficulty())))
			-- end;
		-- end;
	-- end;
	-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
-- };

local rateX = frameX+frameWidth-offsetX
local rateY = frameY+frameHeight-10

if IsUsingWideScreen() == true then
rateX = frameX+frameWidth-offsetX - 140
rateY = frameY+frameHeight-10 - 195
end;

--This works the same as one from widescreen because I'm lazy. -Misterkister
if not IsUsingWideScreen() == true then
rateX = frameX+frameWidth-offsetX - 140
rateY = frameY+frameHeight-10 - 195
end;

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,rateX-50,rateY;zoom,0.4;halign,1;);
	SetCommand=function(self)
		if hsTable ~= nil and rates ~= nil and rtTable[rates[rateIndex]] ~= nil then
			self:settextf("PB:")
		else
			self:settext("PB:")
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

if not IsUsingWideScreen() == true then
t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,rateX+10,rateY;zoom,0.4;halign,1;);
	SetCommand=function(self)
		if hsTable ~= nil and rates ~= nil and rtTable[rates[rateIndex]] ~= nil then
			self:settextf("||")
		else
			self:settext("||")
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};
end;

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,rateX,rateY;zoom,0.4;halign,1;);
	SetCommand=function(self)
		if hsTable ~= nil and rates ~= nil and rtTable[rates[rateIndex]] ~= nil then
			self:settextf("Rate %s",rates[rateIndex],scoreIndex,#rtTable[rates[rateIndex]])
		else
			self:settext("")
		end;
	end;
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
};

t[#t+1] = Def.Quad{
	-- Name="ScrollBar";
	-- InitCommand=cmd(xy,frameX+frameWidth,frameY+frameHeight;zoomto,4,0;halign,1;valign,1;diffuse,getMainColor('highlight');diffusealpha,0.5);
	-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
	-- SetCommand=function(self,params)
		-- self:finishtweening()
		-- self:smooth(0.2)
		-- if hsTable ~= nil and rates ~= nil and rtTable[rates[rateIndex]] ~= nil then
			-- self:zoomy(((frameHeight-offsetY)/#rtTable[rates[rateIndex]]))
			-- self:y(frameY+offsetY+(((frameHeight-offsetY)/#rtTable[rates[rateIndex]])*scoreIndex))
		-- else
			-- self:zoomy(frameHeight-offsetY)
			-- self:y(frameY+frameHeight)
		-- end;
	-- end;
};


local function makeText(index)
	-- return LoadFont("Common Normal")..{
		-- -- InitCommand=cmd(xy,frameX+frameWidth-offsetX,frameY+offsetY+15+(index*15);zoom,fontScale;halign,1;);
		-- -- SetCommand=function(self)
			-- -- local count = 0
			-- -- if update then
				-- -- if rtTable[rates[index]] ~= nil then
					-- -- count = #rtTable[rates[index]]
				-- -- end;
				-- -- if index <= #rates then
					-- -- self:settextf("%s (%d)",rates[index],count)
					-- -- if index == rateIndex then
						-- -- self:diffuse(getMainColor('positive'))
					-- -- else
						-- -- self:diffuse(color("#FFFFFF"))
					-- -- end;
				-- -- else
					-- -- self:settext("")
				-- -- end;
			-- -- end;
		-- -- end;
		-- -- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
	-- -- };
end;

for i=1,10 do
	t[#t+1] =makeText(i)
end;

local function makeJudge(index,judge)
	local t = Def.ActorFrame{
		InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+125+((index-1)*18););
	}

	--labels
	-- t[#t+1] = LoadFont("Common Normal")..{
		-- InitCommand=cmd(zoom,0.5;halign,0;);
		-- BeginCommand=function(self)
			-- self:settext(getJudgeStrings(judge))
			-- self:diffuse(TapNoteScoreToColor(judge))
		-- end;
	-- };

	-- t[#t+1] = LoadFont("Common Normal")..{
		-- InitCommand=cmd(x,120;zoom,0.5;halign,1;);
		-- SetCommand=function(self)
			-- if score ~= nil then
				-- if judge ~= 'HoldNoteScore_Held' and judge ~= 'HoldNoteScore_LetGo' then
					-- self:settext(getScoreTapNoteScore(score,judge))
				-- else
					-- self:settext(getScoreHoldNoteScore(score,judge))
				-- end;
			-- else
				-- self:settext("0")
			-- end;
		-- end;
		-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
	-- };

	-- t[#t+1] = LoadFont("Common Normal")..{
		-- InitCommand=cmd(x,122;zoom,0.3;halign,0;);
		-- SetCommand=function(self)
			-- if score ~= nil then
				-- if judge ~= 'HoldNoteScore_Held' and judge ~= 'HoldNoteScore_LetGo' then
					-- local taps = math.max(1,getMaxNotes(pn))
					-- local count = getScoreTapNoteScore(score,judge)
					-- self:settextf("(%03.2f%%)",(count/taps)*100)
				-- else
					-- local holds = math.max(1,getMaxHolds(pn))
					-- local count = getScoreHoldNoteScore(score,judge)

					-- self:settextf("(%03.2f%%)",(count/holds)*100)
				-- end;
			-- else
				-- self:settext("(0.00%)")
			-- end;
		-- end;
		-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set");
	-- };

	return t

end;

for i=1,#judges do
	t[#t+1] =makeJudge(i,judges[i])
end;

return t