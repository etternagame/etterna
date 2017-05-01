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
end

local t = Def.ActorFrame{
	BeginCommand=cmd(queuecommand,"Set";visible,false),
	OffCommand=cmd(bouncebegin,0.2;xy,-500,0;diffusealpha,0), -- visible(false) doesn't seem to work with sleep
	OnCommand=cmd(bouncebegin,0.2;xy,0,0;diffusealpha,1),
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 2 then
			self:queuecommand("On")
			self:visible(true)
			update = true
			self:playcommand("InitScore")
		else 
			self:queuecommand("Off")
			update = false
		end
	end,
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
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
			end
			if rtTable[rates[rateIndex]] ~= nil then
				score = rtTable[rates[rateIndex]][scoreIndex]
				MESSAGEMAN:Broadcast("ScoreUpdate")
			end
		end
	end,
	PlayerJoinedMessageCommand=cmd(queuecommand,"Set"),
	CurrentSongChangedMessageCommand=cmd(queuecommand,"Set"),
	CurrentStepsP1ChangedMessageCommand=cmd(queuecommand,"Set"),
	CurrentStepsP2ChangedMessageCommand=cmd(queuecommand,"Set"),
	InitScoreCommand=function(self)
			if GAMESTATE:GetCurrentSong() ~= nil then
				hsTable = getScoresByKey(pn)			
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
			end
			MESSAGEMAN:Broadcast("ScoreUpdate")
	end
}

local frameX = 10
local frameY = 45
local frameWidth = capWideScale(320,400)
local frameHeight = 350
local fontScale = 0.4
local offsetX = 10
local offsetY = 20

local judges = {'TapNoteScore_W1','TapNoteScore_W2','TapNoteScore_W3','TapNoteScore_W4','TapNoteScore_W5','TapNoteScore_Miss','HoldNoteScore_Held','HoldNoteScore_LetGo'}

t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))}
t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY;zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)}

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX+5,frameY+offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive')),
	BeginCommand=cmd(settext,"Score Info")
}

t[#t+1] = LoadFont("Common Large")..{
	Name="Grades",
	InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+20;zoom,0.6;halign,0;maxwidth,50/0.6),
	SetCommand=function(self)
		if score ~= nil then
			self:settext(THEME:GetString("Grade",ToEnumShortString(score:GetGrade())))
			self:diffuse(getGradeColor(score:GetGrade()))
		else
			self:settext("")
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

-- Wife display
t[#t+1] = LoadFont("Common Normal")..{
	Name="Score",
	InitCommand=cmd(xy,frameX+offsetX+55,frameY+offsetY+28;zoom,0.5;halign,0),
	SetCommand=function(self)
		if score ~= nil then
			if score:GetWifeScore() == 0 then 
				self:settextf("NA (%s)", "Wife")
			else
				self:settextf("%05.2f%% (%s)", notShit.floor(score:GetWifeScore()*10000)/100, "Wife")
			end
		else
			self:settextf("00.00%% (%s)", "Wife")
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

-- Rescoring stuff
-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Score",
	-- InitCommand=cmd(xy,frameX+offsetX+155,frameY+offsetY+14;zoom,0.5;halign,0),
	-- SetCommand=function(self)
		-- if score ~= nil then
			-- if score:GetWifeScore() == 0 then 
				-- self:settextf("NA (%s)", "Wife")
			-- else
				-- self:settextf("%05.2f%% (%s)", notShit.floor(score:RescoreToWifeJudge(4)*10000)/100, "Wife J4")
			-- end
		-- else
			-- self:settextf("00.00%% (%s)", "Wife")
		-- end
	-- end,
	-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
-- }

-- t[#t+1] = LoadFont("Common Normal")..{
	-- Name="Score",
	-- InitCommand=cmd(xy,frameX+offsetX+155,frameY+offsetY+58;zoom,0.5;halign,0),
	-- SetCommand=function(self)
		-- if score ~= nil then
			-- if score:GetWifeScore() == 0 then 
				-- self:settext("")
			-- else
				-- self:settextf("%5.2f", score:GetSkillsetSSR(5))
			-- end
		-- else
			-- self:settext("")
		-- end
	-- end,
	-- ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
-- }

t[#t+1] = LoadFont("Common Normal")..{
	Name="ClearType",
	InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+41;zoom,0.5;halign,0);
	SetCommand=function(self)
		if score ~= nil then
			self:settext(getClearTypeFromScore(pn,score,0))
			self:diffuse(getClearTypeFromScore(pn,score,2))
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="Combo",
	InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+58;zoom,0.4;halign,0);
	SetCommand=function(self)
		if score ~= nil then
			local maxCombo = getScoreMaxCombo(score)
			self:settextf("Max Combo: %d",maxCombo)
		else
			self:settext("Max Combo: 0")
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="MissCount",
	InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+73;zoom,0.4;halign,0);
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
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="Date",
	InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+88;zoom,0.4;halign,0);
	SetCommand=function(self)
		if score ~= nil then
			self:settext("Date Achieved: "..getScoreDate(score))
		else
			self:settext("Date Achieved: ")
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="Mods",
	InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+103;zoom,0.4;halign,0);
	SetCommand=function(self)
		if score ~= nil then
			self:settext("Mods: " ..score:GetModifiers())
		else
			self:settext("Mods:")
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="StepsAndMeter",
	InitCommand=cmd(xy,frameX+frameWidth-offsetX,frameY+offsetY+10;zoom,0.5;halign,1),
	SetCommand=function(self)
		local steps = GAMESTATE:GetCurrentSteps(pn)
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

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,frameX+frameWidth-offsetX,frameY+frameHeight-10;zoom,0.4;halign,1),
	SetCommand=function(self)
		if hsTable ~= nil and rates ~= nil and rtTable[rates[rateIndex]] ~= nil then
			self:settextf("Rate %s - Showing %d/%d",rates[rateIndex],scoreIndex,#rtTable[rates[rateIndex]])
		else
			self:settext("No Scores Saved")
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="ChordCohesion",
	InitCommand=cmd(xy,frameX+frameWidth/40,frameY+frameHeight-10;zoom,0.4;halign,0),
	SetCommand=function(self)
		if score ~= nil then
			if score:GetChordCohesion() == true then
				self:settext("Chord Cohesion: Yes")
			else
				self:settext("Chord Cohesion: No")
			end
		else
			self:settext("Chord Cohesion:")
		end
	end,
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
}

t[#t+1] = Def.Quad{
	Name="ScrollBar",
	InitCommand=cmd(xy,frameX+frameWidth,frameY+frameHeight;zoomto,4,0;halign,1;valign,1;diffuse,getMainColor('highlight');diffusealpha,0.5),
	ScoreUpdateMessageCommand=cmd(queuecommand,"Set"),
	SetCommand=function(self,params)
		self:finishtweening()
		self:smooth(0.2)
		if hsTable ~= nil and rates ~= nil and rtTable[rates[rateIndex]] ~= nil then
			self:zoomy(((frameHeight-offsetY)/#rtTable[rates[rateIndex]]))
			self:y(frameY+offsetY+(((frameHeight-offsetY)/#rtTable[rates[rateIndex]])*scoreIndex))
		else
			self:zoomy(frameHeight-offsetY)
			self:y(frameY+frameHeight)
		end
	end
}

local function makeText(index)
	return LoadFont("Common Normal")..{
		InitCommand=cmd(xy,frameX+frameWidth-offsetX,frameY+offsetY+15+(index*15);zoom,fontScale;halign,1),
		SetCommand=function(self)
			local count = 0
			if update then
				if rtTable[rates[index]] ~= nil then
					count = #rtTable[rates[index]]
				end
				if index <= #rates then
					self:settextf("%s (%d)",rates[index],count)
					if index == rateIndex then
						self:diffuse(color("#FFFFFF"))
					else
						self:diffuse(getMainColor('positive'))
					end
				else
					self:settext("")
				end
			end
		end,
		ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
	}
end

for i=1,10 do
	t[#t+1] =makeText(i)
end

local function makeJudge(index,judge)
	local t = Def.ActorFrame{InitCommand=cmd(xy,frameX+offsetX,frameY+offsetY+125+((index-1)*18))}

	--labels
	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(zoom,0.5;halign,0),
		BeginCommand=function(self)
			self:settext(getJudgeStrings(judge))
			self:diffuse(byJudgment(judge))
		end
	};

	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(x,120;zoom,0.5;halign,1),
		SetCommand=function(self)
			if score ~= nil then
				if judge ~= 'HoldNoteScore_Held' and judge ~= 'HoldNoteScore_LetGo' then
					self:settext(getScoreTapNoteScore(score,judge))
				else
					self:settext(getScoreHoldNoteScore(score,judge))
				end
			else
				self:settext("0")
			end
		end,
		ScoreUpdateMessageCommand=cmd(queuecommand,"Set"),
	};

	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(x,122;zoom,0.3;halign,0),
		SetCommand=function(self)
			if score ~= nil then
				if judge ~= 'HoldNoteScore_Held' and judge ~= 'HoldNoteScore_LetGo' then
					local taps = math.max(1,getMaxNotes(pn))
					local count = getScoreTapNoteScore(score,judge)
					self:settextf("(%03.2f%%)",(count/taps)*100)
				else
					local holds = math.max(1,getMaxHolds(pn))
					local count = getScoreHoldNoteScore(score,judge)

					self:settextf("(%03.2f%%)",(count/holds)*100)
				end
			else
				self:settext("(0.00%)")
			end
		end,
		ScoreUpdateMessageCommand=cmd(queuecommand,"Set")
	};

	return t
end

for i=1,#judges do
	t[#t+1] =makeJudge(i,judges[i])
end

return t