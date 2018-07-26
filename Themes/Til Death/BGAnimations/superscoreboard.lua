local tzoom = 0.5
local pdh = 48 * tzoom
local ygap = 2
local packspaceY = pdh + ygap

local numgoals = 12
local ind = 0
local offx = 5
local width = SCREEN_WIDTH * 0.56
local dwidth = width - offx * 2
local height = (numgoals+2) * packspaceY

local adjx = 14
local c0x = 10
local c1x = 20 + c0x
local c2x = c1x + (tzoom*7*adjx)			-- guesswork adjustment for epxected text length
local c5x = dwidth							-- right aligned cols
local c4x = c5x - adjx - (tzoom*3*adjx) 	-- right aligned cols
local c3x = c4x - adjx - (tzoom*10*adjx) 	-- right aligned cols
local headeroff = packspaceY/1.5

local moving
local cheese

-- will eat any mousewheel inputs to scroll pages while mouse is over the background frame
local function input(event)
	if isOver(cheese:GetChild("FrameDisplay")) then
		if event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" then
			moving = true
			cheese:queuecommand("PrevPage")
			return true
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" then
			cheese:queuecommand("NextPage")
			return true
		elseif moving == true then
			moving = false
		end
	end
	return false
end

local function highlight(self)
	self:queuecommand("Highlight")
end

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end

local function byAchieved(scoregoal)
	if not scoregoal or scoregoal:IsAchieved() then
		return getMainColor('positive')
	end
	return color("#aaaaaa")
end
local filts = {"All Rates", "Current Rate"}

local goaltable
local o = Def.ActorFrame{
	Name = "GoalDisplay",
	InitCommand=function(self)
		cheese = self
		self:xy(0,0)
		self:SetUpdateFunction(highlight)
	end,
	BeginCommand=function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	OnCommand=function(self)
		GetPlayerOrMachineProfile(PLAYER_1):SetFromAll()
		self:queuecommand("GoalTableRefresh")
	end,
	GoalTableRefreshMessageCommand=function(self)
		goaltable = DLMAN:RequestChartLeaderBoard(GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey())
		ind = 0
		self:queuecommand("Update")
	end,
	UpdateCommand=function(self)
		if ind == #goaltable then
			ind = ind - numgoals
		elseif ind > #goaltable - (#goaltable % numgoals) then
			ind = #goaltable - (#goaltable % numgoals)
		end
		if ind < 0 then
			ind = 0
		end
	end,
	DFRFinishedMessageCommand=function(self)
		GetPlayerOrMachineProfile(PLAYER_1):SetFromAll()
		self:queuecommand("GoalTableRefresh")
	end,
	NextPageCommand=function(self)
		ind = ind + numgoals
		self:queuecommand("Update")
	end,
	PrevPageCommand=function(self)
		ind = ind - numgoals
		self:queuecommand("Update")
	end,
	WheeDoooMessageCommand=function(self)
		self:queuecommand("NextPage")
	end,

	Def.Quad{
	Name = "FrameDisplay",
	InitCommand=function(self)
		self:zoomto(width,height-headeroff):halign(0):valign(0):diffuse(color("#333333")) 
	end
	},
	
	-- headers
	Def.Quad{
		InitCommand=function(self)
			self:xy(offx, headeroff):zoomto(dwidth,pdh):halign(0):diffuse(color("#111111"))
		end,
	},
	LoadFont("Common normal") .. {	--index
		InitCommand=function(self)
			self:xy(width/2, headeroff):zoom(tzoom):halign(0.5)
		end,
		UpdateCommand=function(self)
			self:settextf("%i-%i", ind+1, ind+numgoals)
		end,
	},
	
	LoadFont("Common normal") .. {	--rate
		InitCommand=function(self)
			self:xy(c1x + 25, headeroff):zoom(tzoom):halign(0.5)
		end,
		UpdateCommand=function(self)
			self:settext("R")
		end,
		HighlightCommand=function(self)
			if isOver(self) then
				self:settext("Rate"):diffusealpha(0.6)
			else
				self:settext("R"):diffusealpha(1)
			end
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				GetPlayerOrMachineProfile(PLAYER_1):SortByRate()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end,
	},
	
	LoadFont("Common normal") .. {	--name
		InitCommand=function(self)
			self:xy(c2x, headeroff):zoom(tzoom):halign(0):settext("Song")
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				GetPlayerOrMachineProfile(PLAYER_1):SortByName()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end,
	},
	
	LoadFont("Common normal") .. {	--completed toggle // filters
		InitCommand=function(self)
			self:xy(c3x- capWideScale(15,40), headeroff):zoom(tzoom):halign(0):settext(filts[1])
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				GetPlayerOrMachineProfile(PLAYER_1):ToggleFilter()
				ind = 0
				self:settext(filts[GetPlayerOrMachineProfile(PLAYER_1):GetFilterMode()])
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end,
	},
	
	LoadFont("Common normal") .. {	--date
		InitCommand=function(self)
			self:xy(c4x- 5, headeroff):zoom(tzoom):halign(1):settext("Date")
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				GetPlayerOrMachineProfile(PLAYER_1):SortByDate()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end,
	},
}

local function makeGoalDisplay(i)
	local hs
	
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:y(packspaceY*i + headeroff)
		end,
		UpdateCommand=function(self)
			hs = goaltable[(i + ind)]
			if hs then
				self:queuecommand("Display")
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		
		Def.Quad{
			InitCommand=function(self)
				self:x(offx):zoomto(dwidth,pdh):halign(0)
			end,
			DisplayCommand=function(self)
				self:diffuse(color("#111111CC"))
			end
		},
		
		LoadFont("Common normal") .. {	--rank
			InitCommand=function(self)
				self:x(c0x):zoom(tzoom):halign(0):valign(0)
			end,
			DisplayCommand=function(self)
				self:settextf("%i.", i + ind)
			end
		},
		
		LoadFont("Common normal") .. {	--rate
			InitCommand=function(self)
				self:x(c2x - c1x + offx):zoom(tzoom-0.05):halign(0.5):valign(0)
			end,
			DisplayCommand=function(self)
				local ratestring = string.format("%.2f", hs:GetMusicRate()):gsub("%.?0$", "").."x"
				self:settext(ratestring)
			end,
		},
		
		LoadFont("Common normal") .. {	--ssr
			InitCommand=function(self)
				self:x(c2x - c1x + offx):zoom(tzoom+0.05):halign(0.5):valign(1)
			end,
			DisplayCommand=function(self)
				local ssr = hs:GetSkillsetSSR("Overall")
				self:settextf("%.2f",ssr):diffuse(byMSD(ssr))
			end,
		},
		

		LoadFont("Common normal") .. {	--name
			InitCommand=function(self)
				self:x(c2x):zoom(tzoom+0.1):maxwidth((c3x-c2x - capWideScale(10,40))/tzoom):halign(0):valign(1)
			end,
			DisplayCommand=function(self)
				self:settext(hs:GetDisplayName())
				if hs:GetChordCohesion() then
					self:diffuse(color("#F0EEA6"))
				else
					self:diffuse(getMainColor('positive'))
				end
			end,
			HighlightCommand=function(self)
				highlightIfOver(self)
			end,
			MouseLeftClickMessageCommand=function(self)
				self:addx(0)
			end
		},
		
		LoadFont("Common normal") .. {	--judgments
			InitCommand=function(self)
				self:x(c2x):zoom(tzoom-0.05):halign(0):valign(0):maxwidth(width/2/tzoom)
			end,
			DisplayCommand=function(self)
				self:settext(hs:GetJudgmentString())
				if hs:GetChordCohesion() then
					self:diffuse(color("#F0EEA6"))
				else
					self:diffuse(getMainColor('positive'))
				end
			end,
			HighlightCommand=function(self)
				highlightIfOver(self)
			end,
		},
		
		LoadFont("Common normal") .. {	--percent
			InitCommand=function(self)
				self:x(c5x):zoom(tzoom+0.15):halign(1):valign(1)
			end,
			DisplayCommand=function(self)
				self:settextf("%05.2f%%", hs:GetWifeScore()*10000/100):diffuse(byGrade(hs:GetWifeGrade()))
			end,
		},
		
		LoadFont("Common normal") .. {	--date
			InitCommand=function(self)
				self:x(c5x):zoom(tzoom-0.05):halign(1):valign(0):maxwidth(width/4/tzoom)
			end,
			DisplayCommand=function(self)
				self:settext(hs:GetDate())
			end,
		},
	}
	return o
end

for i=1,numgoals do
	o[#o+1] = makeGoalDisplay(i)
end

return o