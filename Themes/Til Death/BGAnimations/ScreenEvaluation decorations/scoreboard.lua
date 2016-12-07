

function gradestring(tier) --to be moved
	if tier == "Grade_Tier01" then
		return 'AAAA'
	elseif tier == "Grade_Tier02" then
		return 'AAA'
	elseif tier == "Grade_Tier03" then
		return 'AA'
	elseif tier == "Grade_Tier04" then
		return 'A'
	elseif tier == "Grade_Tier05" then
		return 'B'
	elseif tier == "Grade_Tier06" then
		return 'C'
	elseif tier == "Grade_Tier07" then
		return 'D'
	elseif tier == 'Grade_Failed' then
		return 'F'
	else
		return tier
	end;
end;

local lines = 5 -- number of scores to display
local framex = SCREEN_WIDTH-capWideScale(get43size(230),230)
local framey = 60
local frameWidth = capWideScale(get43size(220),220)
local spacing = 34


local song = STATSMAN:GetCurStageStats():GetPlayedSongs()[1]

local profile
local steps
local origTable
local hsTable
local rtTable
local scoreIndex
local score

local player = GAMESTATE:GetEnabledPlayers()[1]

if GAMESTATE:IsPlayerEnabled(player) then
	profile = GetPlayerOrMachineProfile(player)
	steps = STATSMAN:GetCurStageStats():GetPlayerStageStats(player):GetPlayedSteps()[1]
	origTable = getScoresByKey(player)
	score = STATSMAN:GetCurStageStats():GetPlayerStageStats(player):GetHighScore()--origTable[STATSMAN:GetCurStageStats():GetPlayerStageStats(player):GetPersonalHighScoreIndex()+1]
	rtTable = getRateTable(origTable)
	if themeConfig:get_data().global.RateSort then
		hsTable = sortScore(rtTable[getRate(score)] or {},0)
	else
		hsTable = sortScore(rtTable["All"] or {},0)
	end;
	scoreIndex = getHighScoreIndex(hsTable,score)
end;

--Input event for mouse clicks
local function input(event)
	local scoreBoard = SCREENMAN:GetTopScreen():GetChildren().scoreBoard
	if event.DeviceInput.button == 'DeviceButton_left mouse button' then
		if event.type == "InputEventType_Release" then
			for i=0,math.min(lines,#hsTable)-1 do
				if isOver(scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("mouseOver")) then
					scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("grade"):visible(not scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("grade"):GetVisible())
					scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("judge"):visible(not scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("judge"):GetVisible())
					scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("date"):visible(not scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("date"):GetVisible())
					scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("option"):visible(not scoreBoard:GetChild("scoreItem"..tostring(i)):GetChild("option"):GetVisible())
				end;
			end;
		end;
	end
return false;
end


local t = Def.ActorFrame{
	Name="scoreBoard";
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end
};

local function scoreitem(pn,index,scoreIndex,drawindex)

	--First box always displays the 1st place score
	if drawindex == 0 then
		index = 1
	end;

	--Whether the score at index is the score that was just played.
	local equals = (index == scoreIndex)

	--
	local t = Def.ActorFrame {
		Name="scoreItem"..tostring(drawindex);

		--The main quad
		Def.Quad{
			InitCommand=cmd(xy,framex,framey+(drawindex*spacing)-4;zoomto,frameWidth,30;halign,0;valign,0;diffuse,color("#333333");diffusealpha,1;diffuserightedge,color("#33333333"));
			BeginCommand=function(self)
				self:visible(GAMESTATE:IsHumanPlayer(pn));
			end;
		};

		--Highlight quad for the current score
		Def.Quad{
			InitCommand=cmd(xy,framex,framey+(drawindex*spacing)-4;zoomto,frameWidth,30;halign,0;valign,0;diffuse,color("#ffffff");diffusealpha,0.3;diffuserightedge,color("#33333300"));
			BeginCommand=function(self)
				self:visible(GAMESTATE:IsHumanPlayer(pn) and equals);
			end;
		};

		--Quad that will act as the bounding box for mouse rollover/click stuff.
		Def.Quad{
			Name="mouseOver";
			InitCommand=cmd(xy,framex,framey+(drawindex*spacing)-4;zoomto,frameWidth,30;halign,0;valign,0;diffuse,getMainColor('highlight');diffusealpha,0.05),
			BeginCommand=function(self)
				self:visible(false)
			end
		},

		--ClearType lamps
		Def.Quad{
			InitCommand=cmd(xy,framex,framey+(drawindex*spacing)-4;zoomto,8,30;halign,0;valign,0;diffuse,getClearTypeFromScore(pn,hsTable[index],2)),
			BeginCommand=function(self)
				self:visible(GAMESTATE:IsHumanPlayer(pn))
			end
		},

		--rank
		LoadFont("Common normal")..{
			InitCommand=cmd(xy,framex-8,framey+12+(drawindex*spacing);zoom,0.35),
			BeginCommand=function(self)
				if #hsTable >= 1 then
					self:settext(index)
					if equals then
						self:diffuse(color("#ffcccc"))
					else
						self:stopeffect()
					end
				end
			end
		},

		-- DP grade and %score
		LoadFont("Common normal")..{
			Name="grade";
			InitCommand=cmd(xy,framex+10,framey+(drawindex*spacing)+2;zoom,0.35;halign,0;maxwidth,(frameWidth-15)/0.3);
			BeginCommand=function(self)
				local curscore = getScore(hsTable[index])
				local maxscore = getMaxScore(pn,0)
				if maxscore == 0 or maxscore == nil then
					maxscore = 1
				end;
				local pscore = (curscore/maxscore)
				self:settextf("%.2f%% (%s)",math.floor((pscore)*10000)/100,"DP"); 
			end;
		};
		
		-- Wife grade and %score
		LoadFont("Common normal")..{
			Name="grade",
			InitCommand=cmd(xy,framex+10,framey+11+(drawindex*spacing);zoom,0.35;halign,0;maxwidth,(frameWidth-15)/0.3),
			BeginCommand=function(self)
				if hsTable[index]:GetWifeScore() == 0 then 
					self:settextf("NA (%s)", "Wife")
				else
					self:settextf("%05.2f%% (%s)", notShit.floor(hsTable[index]:GetWifeScore()*10000)/100, "Wife")
				end
			end
		},


		--mods
		LoadFont("Common normal")..{
			Name="option";
			InitCommand=cmd(xy,framex+10,framey+11+(drawindex*spacing);zoom,0.35;halign,0;maxwidth,(frameWidth-15)/0.35);
			BeginCommand=function(self)
				self:settext(hsTable[index]:GetModifiers()); 
				self:visible(false)
			end;
		};
	
		--grade text
		LoadFont("Common normal")..{
			InitCommand=cmd(xy,framex+130+capWideScale(get43size(0),50),framey+2+(drawindex*spacing);zoom,0.35;halign,0.5;maxwidth,(frameWidth-15)/0.35),
			BeginCommand=function(self)
				if #hsTable >= 1 and index>= 1 then
					self:settext(gradestring(hsTable[index]:GetGrade()))
					self:diffuse(getGradeColor(hsTable[index]:GetGrade()))
				end
			end
		},
		
		--cleartype text
		LoadFont("Common normal")..{
			InitCommand=cmd(xy,framex+130+capWideScale(get43size(0),50),framey+12+(drawindex*spacing);zoom,0.35;halign,0.5;maxwidth,(frameWidth-15)/0.35),
			BeginCommand=function(self)
				if #hsTable >= 1 and index>= 1 then
					self:settext(getClearTypeFromScore(pn,hsTable[index],0))
					self:diffuse(getClearTypeFromScore(pn,hsTable[index],2))
				end
			end
		},
		
		--max combo
		LoadFont("Common normal")..{
			InitCommand=cmd(xy,framex+130+capWideScale(get43size(0),50),framey+22+(drawindex*spacing);zoom,0.35;halign,0.5;maxwidth,(frameWidth-15)/0.35),
			BeginCommand=function(self)
				if #hsTable >= 1 and index>= 1 then
					self:settextf("%sx",hsTable[index]:GetMaxCombo())
				end
			end
		},

		--judgment
		LoadFont("Common normal")..{
			Name="judge";
			InitCommand=cmd(xy,framex+10,framey+20+(drawindex*spacing);zoom,0.35;halign,0;maxwidth,(frameWidth-15)/0.35),
			BeginCommand=function(self)
				if #hsTable >= 1 and index>= 1 then
					self:settextf("%d / %d / %d / %d / %d / %d",
						hsTable[index]:GetTapNoteScore("TapNoteScore_W1"),
						hsTable[index]:GetTapNoteScore("TapNoteScore_W2"),
						hsTable[index]:GetTapNoteScore("TapNoteScore_W3"),
						hsTable[index]:GetTapNoteScore("TapNoteScore_W4"),
						hsTable[index]:GetTapNoteScore("TapNoteScore_W5"),
						hsTable[index]:GetTapNoteScore("TapNoteScore_Miss"))
				end
			end
		},

		--date
		LoadFont("Common normal")..{
			Name="date";
			InitCommand=cmd(xy,framex+10,framey+20+(drawindex*spacing);zoom,0.35;halign,0),
			BeginCommand=function(self)
				if #hsTable >= 1 and index>= 1 then
					self:settext(hsTable[index]:GetDate())
				end
				self:visible(false)
			end
		},

	};
	return t;
end

--can't have more lines than the # of scores huehuehu
if lines > #hsTable then
	lines = #hsTable
end;

local drawindex = 0
local startind = 1
local finishind = lines+startind-1

-- Sets the range of indexes to display depending on your rank
if scoreIndex>math.floor(#hsTable-lines/2) then
	startind = #hsTable-lines+1
	finishind = #hsTable 
elseif scoreIndex>math.floor(lines/2) then
	finishind = scoreIndex + math.floor(lines/2)
	if lines%2 == 1 then
		startind = scoreIndex - math.floor(lines/2)
	else
		startind = scoreIndex - math.floor(lines/2)+1
	end;
end;

while drawindex<#hsTable and startind<=finishind do
	t[#t+1] = scoreitem(player,startind,scoreIndex,drawindex)
	startind = startind+1
	drawindex  = drawindex+1
end;

--Text that sits above the scoreboard with some info
t[#t+1] = LoadFont("Common normal")..{
	InitCommand=cmd(xy,SCREEN_CENTER_X,framey+95;zoom,0.5;halign,0.5;);
	BeginCommand=function(self)
		local text = ""
		text = getRate(score)
		self:settext(text)
		if text == "1.0x" then 
      self:diffusealpha(0)
		end
	end;
};


if tonumber(PREFSMAN:GetPreference("MaxHighScoresPerListForPlayer")) ~= 3 then
	themeConfig:get_data().global.ScoreBoardNag = false
	themeConfig:set_dirty()
	themeConfig:save()
end

if themeConfig:get_data().global.ScoreBoardNag and #origTable == tonumber(PREFSMAN:GetPreference("MaxHighScoresPerListForPlayer")) then
	t[#t+1] = LoadFont("Common normal")..{
		InitCommand=cmd(xy,framex+frameWidth/2,framey+4*spacing;zoom,0.30;valign,0;diffusealpha,0.8;maxwidth,frameWidth/0.30);
		BeginCommand=function(self)
			local text = string.format("You have reached the maximum number of saved scores for this chart."..
							" \n Lower ranked scores will be removed as you save more scores.\n\n"..
							" Please increase the values for 'Max Machine Scores' and \n'Max Player Scores'"..
							" from the Arcade Options to raise this limit.\n\n\n"..
							"This will no longer appear once the limit is set to any non-default value.\n(You may change back afterwards if you want)\n\n"..
							"The current limit is %s. (Default is 3)",PREFSMAN:GetPreference("MaxHighScoresPerListForPlayer") or 0)
			self:settext(text)
		end;
	}
end

--Update function for showing mouse rollovers
local function Update(self)
	t.InitCommand=cmd(SetUpdateFunction,Update);
	for i=0,drawindex-1 do
		if isOver(self:GetChild("scoreItem"..tostring(i)):GetChild("mouseOver")) then
			self:GetChild("scoreItem"..tostring(i)):GetChild("mouseOver"):visible(true)
		else
			self:GetChild("scoreItem"..tostring(i)):GetChild("mouseOver"):visible(false)
			self:GetChild("scoreItem"..tostring(i)):GetChild("grade"):visible(true)
			self:GetChild("scoreItem"..tostring(i)):GetChild("judge"):visible(true)
			self:GetChild("scoreItem"..tostring(i)):GetChild("date"):visible(false)
			self:GetChild("scoreItem"..tostring(i)):GetChild("option"):visible(false)
		end
	end
end
t.InitCommand=cmd(SetUpdateFunction,Update)


--[[
t[#t+1] = LoadFont("Common normal")..{
	InitCommand=cmd(xy,framex,framey+10+(spacing);zoom,1;halign,0);
	BeginCommand=function(self)
		self:settext(scoreIndex)
	end;
};
--]]

return t;