local t = Def.ActorFrame{}

local scoreType = themeConfig:get_data().global.DefaultScoreType

if GAMESTATE:GetNumPlayersEnabled() == 1 and themeConfig:get_data().eval.ScoreBoardEnabled then
	t[#t+1] = LoadActor("scoreboard")
end


t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=cmd(xy,SCREEN_CENTER_X,capWideScale(135,150);zoom,0.4;maxwidth,400/0.4),
	BeginCommand=cmd(queuecommand,"Set"),
	SetCommand=function(self) 
		if GAMESTATE:IsCourseMode() then
			self:settext(GAMESTATE:GetCurrentCourse():GetDisplayFullTitle().." // "..GAMESTATE:GetCurrentCourse():GetScripter())
		else
			self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle().." // "..GAMESTATE:GetCurrentSong():GetDisplayArtist()) 
		end
	end
}

-- Rate String
t[#t+1] = LoadFont("Common normal")..{
	InitCommand=cmd(xy,SCREEN_CENTER_X,capWideScale(145,160);zoom,0.5;halign,0.5),
	BeginCommand=function(self)
		if getCurRateString() == "1x" then
			self:settext("")
		else
			self:settext(getCurRateString())
		end
	end
}

local function GraphDisplay( pn )
	local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(pn)

	local t = Def.ActorFrame {
		Def.GraphDisplay {
			InitCommand=cmd(Load,"GraphDisplay"),
			BeginCommand=function(self)
				local ss = SCREENMAN:GetTopScreen():GetStageStats()
				self:Set( ss, ss:GetPlayerStageStats(pn) )
				self:diffusealpha(0.7)
				self:GetChild("Line"):diffusealpha(0)
				self:zoom(0.8)
				self:xy(-22,8)
			end
		}
	}
	return t
end

local function ComboGraph( pn )
	local t = Def.ActorFrame {
		Def.ComboGraph {
			InitCommand=cmd(Load,"ComboGraph"..ToEnumShortString(pn)),
			BeginCommand=function(self)
				local ss = SCREENMAN:GetTopScreen():GetStageStats()
				self:Set( ss, ss:GetPlayerStageStats(pn) )
				self:zoom(0.8)
				self:xy(-22,-2)
			end
		}
	}
	return t
end

--ScoreBoard
local judges = {'TapNoteScore_W1','TapNoteScore_W2','TapNoteScore_W3','TapNoteScore_W4','TapNoteScore_W5','TapNoteScore_Miss'}

local pssP1 = STATSMAN:GetCurStageStats():GetPlayerStageStats(PLAYER_1)

local frameX = 20
local frameY = 140
local frameWidth = SCREEN_CENTER_X-120

function scoreBoard(pn,position)
	local t = Def.ActorFrame{
		BeginCommand=function(self)
			if position == 1 then
				self:x(SCREEN_WIDTH-(frameX*2)-frameWidth)
			end
		end
	}
	
	local pss = STATSMAN:GetCurStageStats():GetPlayerStageStats(pn)
	local score = getScoreFromTable(getScoreList(PLAYER_1),pss:GetPersonalHighScoreIndex()+1)
	
	t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX-5,frameY;zoomto,frameWidth+10,220;halign,0;valign,0;diffuse,color("#333333CC"))};
	t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY+30;zoomto,frameWidth,2;halign,0;diffuse,getMainColor('highlight');diffusealpha,0.5)};
	t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY+55;zoomto,frameWidth,2;halign,0;diffuse,getMainColor('highlight');diffusealpha,0.5)};

	t[#t+1] = LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameWidth+frameX,frameY+32;zoom,0.5;halign,1;valign,0;maxwidth,200),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			local meter = score:GetSkillsetSSR(1)
			self:settextf("%5.2f", meter)
			self:diffuse(ByMSD(meter))
		end,
	};
	t[#t+1] = LoadFont("Common Large") .. {
		InitCommand=cmd(xy,frameWidth+frameX,frameY+7;zoom,0.5;halign,1;valign,0;maxwidth,200),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self)
			local steps = GAMESTATE:GetCurrentSteps(PLAYER_1)
			local diff = getDifficulty(steps:GetDifficulty())
			self:settext(getShortDifficulty(diff))
			self:diffuse(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(),steps:GetDifficulty())))
		end
	};
	
	-- Wife percent
	t[#t+1] = LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+5,frameY+9;zoom,0.45;halign,0;valign,0),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self) 
			self:diffuse(getGradeColor(pss:GetWifeGrade()))
			self:settextf("%05.2f%% (%s)",notShit.floor(pss:GetWifeScore()*10000)/100, "Wife")
		end,
	};
	
	-- DP percent
	t[#t+1] = LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+5,frameY+34;zoom,0.45;halign,0;valign,0),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self) 
			self:diffuse(getGradeColor(pss:GetGrade()))
			self:settextf("%05.2f%% (%s)",GetPercentDP(score), "DP")
		end,
	}
	
	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=cmd(xy,frameX+5,frameY+63;zoom,0.40;halign,0;maxwidth,frameWidth/0.4),
		BeginCommand=cmd(queuecommand,"Set"),
		SetCommand=function(self) 
			self:settext(GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString('ModsLevel_Current'))
		end
	}

	for k,v in ipairs(judges) do
		t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX,frameY+80+((k-1)*22);zoomto,frameWidth,18;halign,0;diffuse,byJudgment(v);diffusealpha,0.5)};
		t[#t+1] = Def.Quad{
			InitCommand=cmd(xy,frameX,frameY+80+((k-1)*22);zoomto,0,18;halign,0;diffuse,byJudgment(v);diffusealpha,0.5;),
			BeginCommand=cmd(glowshift;effectcolor1,color("1,1,1,"..tostring(pss:GetPercentageOfTaps(v)*0.4));effectcolor2,color("1,1,1,0");sleep,0.5;decelerate,2;zoomx,frameWidth*pss:GetPercentageOfTaps(v))
		};
		t[#t+1] = LoadFont("Common Large")..{
			InitCommand=cmd(xy,frameX+10,frameY+80+((k-1)*22);zoom,0.25;halign,0),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self) 
				self:settext(getJudgeStrings(v))
			end
		};
		t[#t+1] = LoadFont("Common Large")..{
			InitCommand=cmd(xy,frameX+frameWidth-40,frameY+80+((k-1)*22);zoom,0.25;halign,1),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self) 
				self:settext(pss:GetTapNoteScores(v))
			end
		};
		t[#t+1] = LoadFont("Common Normal")..{
			InitCommand=cmd(xy,frameX+frameWidth-38,frameY+80+((k-1)*22);zoom,0.3;halign,0),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self) 
				self:settextf("(%03.2f%%)",pss:GetPercentageOfTaps(v)*100)
			end
		};
	end

	local fart = {"Holds", "Mines", "Rolls", "Lifts", "Fakes"}
	t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameX-5,frameY+230;zoomto,frameWidth/2-10,60;halign,0;valign,0;diffuse,color("#333333CC"))};
	for i=1,#fart do
		t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX,frameY+230+10*i;zoom,0.4;halign,0;settext,fart[i])};
		t[#t+1] = LoadFont("Common Normal")..{
			InitCommand=cmd(xy,frameWidth/2,frameY+230+10*i;zoom,0.4;halign,1),
			BeginCommand=cmd(queuecommand,"Set"),
			SetCommand=function(self) 
				self:settextf("%03d/%03d",pss:GetRadarActual():GetValue("RadarCategory_"..fart[i]),pss:GetRadarPossible():GetValue("RadarCategory_"..fart[i]))
			end
		};
	end
	
	-- stats stuff
	devianceTable = pss:GetOffsetVector()
	t[#t+1] = Def.Quad{InitCommand=cmd(xy,frameWidth+25,frameY+230;zoomto,frameWidth/2+10,60;halign,1;valign,0;diffuse,color("#333333CC"))};
	local smallest,largest = wifeRange(devianceTable)
	local doot = {"Mean", "Mean(Abs)", "Sd", "Smallest", "Largest"}
	local mcscoot = {
		wifeMean(devianceTable), 
		ms.tableSum(devianceTable, 1,true)/#devianceTable,
		wifeSd(devianceTable),
		smallest, 
		largest
	}

	for i=1,#doot do
		t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameX+capWideScale(get43size(130),160),frameY+230+10*i;zoom,0.4;halign,0;settext,doot[i])};
		t[#t+1] = LoadFont("Common Normal")..{InitCommand=cmd(xy,frameWidth+20,frameY+230+10*i;zoom,0.4;halign,1;settextf,"%5.2fms",mcscoot[i])};
	end
	
	return t
end;


if GAMESTATE:IsPlayerEnabled(PLAYER_1) then
	t[#t+1] = scoreBoard(PLAYER_1,0)
	if ShowStandardDecoration("GraphDisplay") then
		t[#t+1] = StandardDecorationFromTable( "GraphDisplay" .. ToEnumShortString(PLAYER_1), GraphDisplay(PLAYER_1) )
	end
	if ShowStandardDecoration("ComboGraph") then
		t[#t+1] = StandardDecorationFromTable( "ComboGraph" .. ToEnumShortString(PLAYER_1),ComboGraph(PLAYER_1) )
	end
end


t[#t+1] = LoadActor("offsetplot")

return t