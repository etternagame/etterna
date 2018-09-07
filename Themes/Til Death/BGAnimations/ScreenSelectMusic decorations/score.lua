-- refactored a bit but still needs work -mina
local collapsed = false
local rtTable
local rates
local rateIndex = 1
local scoreIndex = 1
local score
local pn = GAMESTATE:GetEnabledPlayers()[1]
local nestedTab = 1
local nestedTabs = {"Local", "Online"}

local frameX = 10
local frameY = 45
local frameWidth = SCREEN_WIDTH * 0.56
local frameHeight = 350
local fontScale = 0.4
local offsetX = 10
local offsetY = 30
local netScoresPerPage = 8
local netScoresCurrentPage = 1
local nestedTabButtonWidth = 153
local nestedTabButtonHeight = 20
local netPageButtonWidth = 50
local netPageButtonHeight = 50
local headeroffY = 26/1.5

local selectedrateonly

local judges = {'TapNoteScore_W1','TapNoteScore_W2','TapNoteScore_W3','TapNoteScore_W4','TapNoteScore_W5','TapNoteScore_Miss','HoldNoteScore_Held','HoldNoteScore_LetGo'}

local defaultRateText = ""
if themeConfig:get_data().global.RateSort then
	defaultRateText = "1.0x"
else
	defaultRateText = "All"
end

local function isOver(element)
	if element:GetParent():GetParent():GetVisible() == false then
		return false end
	if element:GetParent():GetVisible() == false then
		return false end
	if element:GetVisible() == false then
		return false end
	local x = getTrueX(element)
	local y = getTrueY(element)
	local hAlign = element:GetHAlign()
	local vAlign = element:GetVAlign()
	local w = element:GetZoomedWidth()
	local h = element:GetZoomedHeight()

	local mouseX = INPUTFILTER:GetMouseX()
	local mouseY = INPUTFILTER:GetMouseY()

	local withinX = (mouseX >= (x-(hAlign*w))) and (mouseX <= ((x+w)-(hAlign*w)))
	local withinY = (mouseY >= (y-(vAlign*h))) and (mouseY <= ((y+h)-(vAlign*h)))

	return (withinX and withinY)
end

-- should maybe make some of these generic
local function highlight(self)
	self:queuecommand("Highlight")
end

-- note: will use the local isover functionality
local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end

local ret = Def.ActorFrame{
	BeginCommand=function(self)
		self:queuecommand("Set"):visible(false)
		self:GetChild("LocalScores"):visible(false)
		self:GetChild("ScoreDisplay"):xy(frameX,frameY):visible(false)
		
		if FILTERMAN:oopsimlazylol() then	-- set saved position and auto collapse
			nestedTab = 2
			self:GetChild("LocalScores"):visible(false)
			self:GetChild("ScoreDisplay"):xy(FILTERMAN:grabposx("Doot"),FILTERMAN:grabposy("Doot")):visible(true)
			self:playcommand("Collapse")
		end
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(-500,0):diffusealpha(0) -- visible(false)
	end,
	OnCommand=function(self)
		self:bouncebegin(0.2):xy(0,0):diffusealpha(1)
		if nestedTab == 1 then
			self:GetChild("LocalScores"):visible(true)
		end
	end,
	SetCommand=function(self)
		self:finishtweening(1)
		if getTabIndex() == 2 then						-- switching to this tab
			if collapsed then							-- expand if collaped
				self:queuecommand("Expand")
			else
				self:queuecommand("On")
				self:visible(true)
			end
		elseif collapsed and getTabIndex() == 0 then	-- display on general tab if collapsed
			self:queuecommand("On")
			self:visible(true)							-- not sure about whether this works or is needed
		elseif collapsed and getTabIndex() ~= 0 then	-- but not others
			self:queuecommand("Off")
		elseif not collapsed then						-- if not collapsed, never display outside of this tab
			self:queuecommand("Off")
		end
	end,
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	UpdateChartMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	CollapseCommand=function(self)
		collapsed = true
		resetTabIndex()
		MESSAGEMAN:Broadcast("TabChanged")
	end,
	ExpandCommand=function(self)
		collapsed = false
		if getTabIndex() ~= 2 then
			setTabIndex(2)
		end
		self:GetChild("ScoreDisplay"):xy(frameX,frameY)
		MESSAGEMAN:Broadcast("TabChanged")
	end
}


local cheese
-- eats only inputs that would scroll to a new score
local function input(event)
	if cheese:GetVisible() and isOver(cheese:GetChild("FrameDisplay")) then
		if event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" then
			moving = true
			if nestedTab == 1 and rtTable and rtTable[rates[rateIndex]] ~= nil then
				cheese:queuecommand("PrevScore")
				return true
			end
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" then
			if nestedTab == 1 and rtTable ~= nil and rtTable[rates[rateIndex]] ~= nil then
				cheese:queuecommand("NextScore")
				return true
			end
		elseif moving == true then
			moving = false
		end
	end
	return false
end

local t = Def.ActorFrame {
	Name="LocalScores",
	InitCommand=function(self)
		rtTable = nil
		self:xy(frameX+offsetX,frameY+offsetY + headeroffY)
		self:SetUpdateFunction(highlight)
		cheese = self
	end,
	BeginCommand=function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	SetCommand=function(self)
		if nestedTab == 1 and self:GetVisible() then
			if GAMESTATE:GetCurrentSong() ~= nil then
				rtTable = getRateTable()
				if rtTable ~= nil then
					rates,rateIndex = getUsedRates(rtTable)
					scoreIndex = 1
					self:queuecommand("Display")
				else
					self:queuecommand("Init")
				end
			else
				self:queuecommand("Init")
			end
		end
	end,
	NestedTabChangedMessageCommand=function(self)
		self:visible(nestedTab == 1)
		self:queuecommand("Set")
	end,
	CodeMessageCommand=function(self,params)
		if nestedTab == 1 and rtTable ~= nil and rtTable[rates[rateIndex]] ~= nil then
			if params.Name == "NextRate" then
				self:queuecommand("NextRate")
			elseif params.Name == "PrevRate" then
				self:queuecommand("PrevRate")
			elseif params.Name == "NextScore" then
				self:queuecommand("NextScore")
			elseif params.Name == "PrevScore" then
				self:queuecommand("PrevScore")
			end
		end
	end,
	NextRateCommand=function(self)
		rateIndex = ((rateIndex)%(#rates))+1
		scoreIndex = 1
		self:queuecommand("Display")
	end,
	PrevRateCommand=function(self)
		rateIndex = ((rateIndex-2)%(#rates))+1
		scoreIndex = 1
		self:queuecommand("Display")
	end,
	NextScoreCommand=function(self)
		scoreIndex = ((scoreIndex)%(#rtTable[rates[rateIndex]]))+1
		self:queuecommand("Display")
	end,
	PrevScoreCommand=function(self)
		scoreIndex = ((scoreIndex-2)%(#rtTable[rates[rateIndex]]))+1
		self:queuecommand("Display")
	end,
	DisplayCommand=function(self)
		score = rtTable[rates[rateIndex]][scoreIndex]
		setScoreForPlot(score)
	end,

	Def.Quad{
		Name="FrameDisplay",
		InitCommand=function(self)
			self:xy(-frameX,-headeroffY - offsetY):zoomto(frameWidth,frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
		end,
		CollapseCommand=function(self)
			self:visible(false)
		end,
		ExpandCommand=function(self)
			self:visible(true)
		end,
	}
}

t[#t+1] = LoadFont("Common Large")..{
	Name="Grades",
	InitCommand=function(self)
		self:y(20):zoom(0.6):halign(0):maxwidth(50/0.6):settext("")
	end,
	DisplayCommand=function(self)
		self:settext(THEME:GetString("Grade",ToEnumShortString(score:GetWifeGrade())))
		self:diffuse(getGradeColor(score:GetWifeGrade()))
	end,
}

-- Wife display
t[#t+1] = LoadFont("Common Normal")..{
	Name="Wife",
	InitCommand=function(self)
		self:xy(55,15):zoom(0.6):halign(0):settext("")
	end,
	DisplayCommand=function(self)
		if score:GetWifeScore() == 0 then 
			self:settextf("NA")
		else
			self:settextf("%05.2f%%", notShit.floor(score:GetWifeScore()*10000)/100):diffuse(byGrade(score:GetWifeGrade()))
		end
	end,
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="Score",
	InitCommand=function(self)
		self:xy(55,33):zoom(0.6):halign(0):settext("")
	end,
	DisplayCommand=function(self)
		if score:GetWifeScore() == 0 then 
			self:settext("")
		else
			local overall = score:GetSkillsetSSR("Overall")
			self:settextf("%.2f", overall):diffuse(byMSD(overall))
		end
	end
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="ClearType",
	InitCommand=function(self)
		self:y(41):zoom(0.5):halign(0):halign(0):settext("No Play"):diffuse(color(colorConfig:get_data().clearType["NoPlay"]))
	end,
	DisplayCommand=function(self)
		self:settext(getClearTypeFromScore(pn,score,0))
		self:diffuse(getClearTypeFromScore(pn,score,2))
	end,
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="Combo",
	InitCommand=function(self)
		self:y(58):zoom(0.4):halign(0):settext("Max Combo:")
	end,
	DisplayCommand=function(self)
		self:settextf("Max Combo: %d",score:GetMaxCombo())
	end,
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="MissCount",
	InitCommand=function(self)
		self:y(73):zoom(0.4):halign(0):settext("Miss Count:")
	end,
	DisplayCommand=function(self)
		local missCount = getScoreMissCount(score)
		if missCount ~= nil then
			self:settext("Miss Count: "..missCount)
		else
			self:settext("Miss Count: -")
		end
	end
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="Date",
	InitCommand=function(self)
		self:y(88):zoom(0.4):halign(0):settext("Date Achieved:")
	end,
	DisplayCommand=function(self)
		self:settext("Date Achieved: "..getScoreDate(score))
	end,
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="Mods",
	InitCommand=function(self)
		self:y(103):zoom(0.4):halign(0):settext("Mods:")
	end,
	DisplayCommand=function(self)
		self:settext("Mods: " ..score:GetModifiers())
	end
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="StepsAndMeter",
	InitCommand=function(self)
		self:xy(frameWidth - offsetX - frameX,10):zoom(0.5):halign(1):queuecommand("Display")
	end,
	DisplayCommand=function(self)
		if GAMESTATE:GetCurrentSong() then
			local steps = GAMESTATE:GetCurrentSteps(pn)
			local diff = getDifficulty(steps:GetDifficulty())
			local stype = ToEnumShortString(steps:GetStepsType()):gsub("%_"," ")
			local meter = steps:GetMeter()
			self:settext(stype.." "..diff.." "..meter)
			self:diffuse(getDifficultyColor(GetCustomDifficulty(steps:GetStepsType(),steps:GetDifficulty())))
		end
	end,
}

t[#t+1] = LoadFont("Common Normal")..{
	InitCommand=function(self)
		self:xy(frameWidth-offsetX- frameX,frameHeight-headeroffY - 10 - offsetY):zoom(0.4):halign(1):settext("No Scores Saved")
	end,
	DisplayCommand=function(self)
		self:settextf("Rate %s - Showing %d/%d",rates[rateIndex],scoreIndex,#rtTable[rates[rateIndex]])
	end,
}

t[#t+1] = LoadFont("Common Normal")..{
	Name="ChordCohesion",
	InitCommand=function(self)
		self:y(frameHeight-headeroffY-10 - offsetY):zoom(0.4):halign(0):settext("Chord Cohesion:")
	end,
	DisplayCommand=function(self)
		if score:GetChordCohesion() then
			self:settext("Chord Cohesion: Yes")
		else
			self:settext("Chord Cohesion: No")
		end
	end,
}

t[#t+1] = Def.Quad{
	Name="ScrollBar",
	InitCommand=function(self)
		self:x(frameWidth-frameX):zoomto(4,0):halign(1):valign(1):diffuse(getMainColor('highlight')):diffusealpha(0.75)
	end,
	ScoreUpdateMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	DisplayCommand=function(self)
		self:finishtweening()
		self:smooth(0.2)
		self:zoomy(((frameHeight)/#rtTable[rates[rateIndex]]))
		self:y((((frameHeight)/#rtTable[rates[rateIndex]])*scoreIndex) - headeroffY - offsetY)
	end
}

local function makeText(index)
	return LoadFont("Common Normal")..{
		InitCommand=function(self)
			self:xy(frameWidth-offsetX- frameX,offsetY+15+(index*15)):zoom(fontScale+0.05):halign(1):settext("")
		end,
		DisplayCommand=function(self)
			local count = 0
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
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand=function(self)
			if nestedTab == 1 and isOver(self) then
				rateIndex = index
				scoreIndex = 1
				self:GetParent():queuecommand("Display")
			end
		end
	}
end

for i=1,10 do
	t[#t+1] =makeText(i)
end

local function makeJudge(index,judge)
	local t = Def.ActorFrame{InitCommand=function(self)
		self:y(125+((index-1)*18))
	end}

	--labels
	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=function(self)
			self:zoom(0.5):halign(0)
		end,
		BeginCommand=function(self)
			self:settext(getJudgeStrings(judge))
			self:diffuse(byJudgment(judge))
		end
	};

	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=function(self)
			self:x(120):zoom(0.5):halign(1):settext("0")
		end,
		DisplayCommand=function(self)
			if judge ~= 'HoldNoteScore_Held' and judge ~= 'HoldNoteScore_LetGo' then
				self:settext(getScoreTapNoteScore(score,judge))
			else
				self:settext(getScoreHoldNoteScore(score,judge))
			end
		end,
	};

	t[#t+1] = LoadFont("Common Normal")..{
		InitCommand=function(self)
			self:x(122):zoom(0.3):halign(0):settext("")
		end,
		DisplayCommand=function(self)
			if judge ~= 'HoldNoteScore_Held' and judge ~= 'HoldNoteScore_LetGo' then
				local taps = math.max(1,getMaxNotes(pn))
				local count = getScoreTapNoteScore(score,judge)
				self:settextf("(%03.2f%%)",(count/taps)*100)
			else
				local holds = math.max(1,getMaxHolds(pn))
				local count = getScoreHoldNoteScore(score,judge)
				self:settextf("(%03.2f%%)",(count/holds)*100)
			end
		end,
	};

	return t
end

for i=1,#judges do
	t[#t+1] =makeJudge(i,judges[i])
end

t[#t+1] = LoadFont("Common Normal")..{
	Name="Score",
	InitCommand=function(self)
		self:y(frameHeight-headeroffY - 30 - offsetY):zoom(0.5):halign(0):settext("")
	end,
	DisplayCommand=function(self)
		if score:HasReplayData() then 
			self:settext("Show Replay Data")
		else
			self:settext("No Replay Data")
		end
	end,
	HighlightCommand=function(self)
		highlightIfOver(self)
	end,
	MouseLeftClickMessageCommand=function(self)
		if nestedTab == 1 then
			if getTabIndex() == 2 and getScoreForPlot() and getScoreForPlot():HasReplayData() and isOver(self) then
				SCREENMAN:AddNewScreenToTop("ScreenScoreTabOffsetPlot")
			end
		end
	end
}

-- matches the built-in online score header
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(-frameX/2, -offsetY):zoomto(frameWidth - 10,24):halign(0):diffuse(color("#111111"))
	end,
}


ret[#ret+1] = t

function nestedTabButton(i) 
  return  
  Def.ActorFrame{
    InitCommand=function(self) 
      self:xy(frameX + offsetX + (i-1)*(nestedTabButtonWidth-80), frameY + headeroffY)
	  self:SetUpdateFunction(highlight)
    end, 
	CollapseCommand=function(self)
		self:visible(false)
	end,
	ExpandCommand=function(self)
		self:visible(true)
	end,
    LoadFont("Common normal") .. { 
      InitCommand=function(self) 
        self:diffuse(getMainColor('positive')):maxwidth(nestedTabButtonWidth-80):maxheight(40):zoom(0.75):settext(nestedTabs[i]):halign(0)
      end, 
	  HighlightCommand=function(self)
		if isOver(self) and nestedTab ~= i then
			self:diffusealpha(0.75)
		elseif nestedTab == i then
			self:diffusealpha(1)
		else
			self:diffusealpha(0.6)
		end
	  end,
      MouseLeftClickMessageCommand=function(self) 
        if isOver(self) then 
          nestedTab = i 
          MESSAGEMAN:Broadcast("NestedTabChanged") 
		  if nestedTab == 1 then
			self:GetParent():GetParent():GetChild("ScoreDisplay"):visible(false)
		  else
			self:GetParent():GetParent():GetChild("ScoreDisplay"):visible(true)
		  end
        end 
      end, 
      NestedTabChangedMessageCommand=function(self) 
        self:queuecommand("Set") 
      end
    }
  }
end

-- online score display
ret[#ret+1] = LoadActor("../superscoreboard")
for i=1,#nestedTabs do  
  ret[#ret+1] = nestedTabButton(i) 
end 


return ret
