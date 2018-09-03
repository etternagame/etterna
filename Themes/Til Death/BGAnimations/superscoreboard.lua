local tzoom = 0.5
local pdh = 48 * tzoom
local ygap = 2
local packspaceY = pdh + ygap

local numscores = 12
local ind = 0
local offx = 5
local width = SCREEN_WIDTH * 0.56
local dwidth = width - offx * 2
local height = (numscores+2) * packspaceY

local adjx = 14
local c0x = 10
local c1x = 20 + c0x
local c2x = c1x + (tzoom*7*adjx)			-- guesswork adjustment for epxected text length
local c5x = dwidth							-- right aligned cols
local c4x = c5x - adjx - (tzoom*3*adjx) 	-- right aligned cols
local c3x = c4x - adjx - (tzoom*10*adjx) 	-- right aligned cols
local headeroff = packspaceY/1.5
local row2yoff = 1
local moving
local cheese
local collapsed = false

-- will eat any mousewheel inputs to scroll pages while mouse is over the background frame
local function input(event)
	if cheese:GetVisible() and isOver(cheese:GetChild("FrameDisplay")) then
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

local function highlight(self)
	self:queuecommand("Highlight")
	self:queuecommand("WHAZZZAAAA")
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
local topornah = {"Top Scores", "All Scores"}

local scoretable
local o = Def.ActorFrame{
	Name = "ScoreDisplay",
	InitCommand=function(self)
		cheese = self
		self:SetUpdateFunction(highlight)
	end,
	BeginCommand=function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	OnCommand=function(self)
		GetPlayerOrMachineProfile(PLAYER_1):SetFromAll()
		self:queuecommand("ChartLeaderboardUpdate")
	end,
	ChartLeaderboardUpdateMessageCommand=function(self)
		scoretable = DLMAN:RequestChartLeaderBoard(GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey())
		ind = 0
		self:playcommand("Update")
	end,
	UpdateCommand=function(self)
		if ind == #scoretable then
			ind = ind - numscores
		elseif ind > #scoretable - (#scoretable % numscores) then
			ind = #scoretable - (#scoretable % numscores)
		end
		if ind < 0 then
			ind = 0
		end
	end,
	NextPageCommand=function(self)
		ind = ind + numscores
		self:queuecommand("Update")
	end,
	PrevPageCommand=function(self)
		ind = ind - numscores
		self:queuecommand("Update")
	end,
	CollapseCommand=function(self)
		tzoom = 0.5 * 0.75
		pdh = 38 * tzoom
		ygap = 2
		packspaceY = pdh + ygap
		
		numscores = 10
		ind = 0
		offx = 5
		width = math.max(SCREEN_WIDTH * 0.25,240)
		dwidth = width - offx * 2
		height = (numscores+2) * packspaceY
		
		adjx = 14
		c0x = 10
		c1x = 10 + c0x
		c2x = c1x + (tzoom*7*adjx)
		c5x = dwidth
		c4x = c5x - adjx - (tzoom*3*adjx)
		c3x = c4x - adjx - (tzoom*10*adjx)
		headeroff = packspaceY/1.5
		row2yoff = 1
		collapsed = true
		self:diffusealpha(0.8)
		
		if FILTERMAN:grabposx("Doot") <= 10 or FILTERMAN:grabposy("Doot") <= 45 or FILTERMAN:grabposx("Doot") >= SCREEN_WIDTH - 60 or FILTERMAN:grabposy("Doot") >= SCREEN_HEIGHT - 45 then
			self:xy(10, 45)
		else
			self:xy(FILTERMAN:grabposx("Doot"),FILTERMAN:grabposy("Doot"))
		end
		
		FILTERMAN:HelpImTrappedInAChineseFortuneCodingFactory(true)
		self:playcommand("Init")
	end,
	ExpandCommand=function(self)
		tzoom = 0.5
		pdh = 48 * tzoom
		ygap = 2
		packspaceY = pdh + ygap
		
		numscores = 12
		ind = 0
		offx = 5
		width = SCREEN_WIDTH * 0.56
		dwidth = width - offx * 2
		height = (numscores+2) * packspaceY
		
		adjx = 14
		c0x = 10
		c1x = 20 + c0x
		c2x = c1x + (tzoom*7*adjx)			-- guesswork adjustment for epxected text length
		c5x = dwidth							-- right aligned cols
		c4x = c5x - adjx - (tzoom*3*adjx) 	-- right aligned cols
		c3x = c4x - adjx - (tzoom*10*adjx) 	-- right aligned cols
		headeroff = packspaceY/1.5
		row2yoff = 1
		collapsed = false
		self:diffusealpha(1)
		FILTERMAN:HelpImTrappedInAChineseFortuneCodingFactory(false)
		self:playcommand("Init")
	end,

	Def.Quad{
		Name = "FrameDisplay",
		InitCommand=function(self)
			self:zoomto(width,height-headeroff):halign(0):valign(0):diffuse(color("#333333")) 
		end,
		HighlightCommand=function(self)
			if isOver(self) and collapsed then
				self:diffusealpha(1)
			else
				self:diffusealpha(0.8)
			end
		end,
		MouseRightClickMessageCommand=function(self)
			if isOver(self) and not collapsed then
				FILTERMAN:HelpImTrappedInAChineseFortuneCodingFactory(true)
				self:GetParent():GetParent():playcommand("Collapse")
			elseif isOver(self) then
				self:GetParent():GetParent():playcommand("Expand")
			end
		end,
	},
	
	-- headers
	Def.Quad{
		Name="HeaderBar",
		InitCommand=function(self)
			self:xy(offx, headeroff):zoomto(dwidth,pdh):halign(0):diffuse(color("#111111"))
		end,
	},
	
	-- grabby thing
	Def.Quad{
		InitCommand=function(self)
			self:xy(dwidth/4, headeroff):zoomto(dwidth - dwidth/4,pdh):halign(0):diffusealpha(1):diffuse(color("#111111"))
		end,
		WHAZZZAAAACommand=function(self)
			if isOver(self) and collapsed then
				self:diffusealpha(0.6):diffuse(color("#fafafa"))
				if INPUTFILTER:IsBeingPressed("Mouse 0", "Mouse") then
					self:diffusealpha(0):zoomto(400,400)
					local nx = INPUTFILTER:GetMouseX() - width/2
					local ny = INPUTFILTER:GetMouseY() - self:GetY()
					self:GetParent():xy(nx,ny)
					FILTERMAN:savepos("Doot", nx, ny)
				else
					self:zoomto(dwidth/2,pdh/2)
				end
			else
				self:diffusealpha(0):diffuse(color("#111111"))
			end
		end,
	},
	
	LoadFont("Common normal") .. {	--current rate toggle
		InitCommand=function(self)
			self:xy(c5x - 10, headeroff):zoom(tzoom):halign(1)
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		UpdateCommand=function(self)
			if DLMAN:GetCurrentRateFilter() then 
				self:settext(filts[2])
			else
				self:settext(filts[1])
			end
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				DLMAN:ToggleRateFilter()
				ind = 0
				self:GetParent():queuecommand("ChartLeaderboardUpdate")
			end
		end,
	},
	LoadFont("Common normal") .. {	--top score/all score toggle
		InitCommand=function(self)
			self:xy(c5x - 115, headeroff):zoom(tzoom):halign(1)
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		UpdateCommand=function(self)
			if DLMAN:GetTopScoresOnlyFilter() then 
				self:settext(topornah[1])
			else
				self:settext(topornah[2])
			end
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				DLMAN:ToggleTopScoresOnlyFilter()
				ind = 0
				self:GetParent():queuecommand("ChartLeaderboardUpdate")
			end
		end,
	},
}

local function makeScoreDisplay(i)
	local hs
	
	local o = Def.ActorFrame{
		InitCommand=function(self)
			self:y(packspaceY*i + headeroff)
			if i > numscores then
				self:visible(false)
			end
		end,
		UpdateCommand=function(self)
			hs = scoretable[(i + ind)]
			if hs and i <= numscores then
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
			end,
			HighlightCommand=function(self)
				if isOver(self) and collapsed then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.6)
				end
			end,
		},
		
		LoadFont("Common normal") .. {	--rank
			InitCommand=function(self)
				self:x(c0x):zoom(tzoom):halign(0):valign(0)
				if collapsed then
					self:x(c0x):zoom(tzoom):halign(0):valign(0.5)
				end
			end,
			DisplayCommand=function(self)
				self:settextf("%i.", i + ind)
			end
		},
		
		LoadFont("Common normal") .. {	--ssr
			InitCommand=function(self)
				self:x(c2x - c1x + offx):zoom(tzoom+0.05):halign(0.5):valign(1)
				if collapsed then
					self:x(46):zoom(tzoom+0.15):halign(0.5):valign(0.5):maxwidth(20/tzoom)
				end
			end,
			DisplayCommand=function(self)
				local ssr = hs:GetSkillsetSSR("Overall")
				self:settextf("%.2f",ssr):diffuse(byMSD(ssr))
			end,
		},
		
		LoadFont("Common normal") .. {	--rate
			InitCommand=function(self)
				self:x(c2x - c1x + offx):zoom(tzoom-0.05):halign(0.5):valign(0):addy(row2yoff)
				if collapsed then
					self:x(c4x - 14):zoom(tzoom):halign(1):valign(0.5):addy(-row2yoff):maxwidth(30/tzoom)
				end
			end,
			DisplayCommand=function(self)
				local ratestring = string.format("%.2f", hs:GetMusicRate()):gsub("%.?0$", "").."x"
				self:settext(ratestring)
			end,
			ExpandCommand=function(self)
				self:addy(-row2yoff)
			end,
		},

		LoadFont("Common normal") .. {	--name
			InitCommand=function(self)
				self:x(c2x):zoom(tzoom+0.1):maxwidth((c3x-c2x - capWideScale(10,40))/tzoom):halign(0):valign(1)
				if collapsed then
					self:x(c2x + 10):maxwidth(60/tzoom):zoom(tzoom+0.2):valign(0.5)
				end
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
				if isOver(self) then
					local urlstringyo = "https://etternaonline.com/user/"..hs:GetDisplayName()
					GAMESTATE:ApplyGameCommand("urlnoexit,"..urlstringyo)
				end
			end,
		},
		
		LoadFont("Common normal") .. {	--judgments
			InitCommand=function(self)
				if not collapsed then
					self:x(c2x):zoom(tzoom-0.05):halign(0):valign(0):maxwidth(width/2/tzoom):addy(row2yoff)
				end
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
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) then
					local urlstringyo = "https://etternaonline.com/score/view/"..hs:GetScoreid()..hs:GetUserid()
					GAMESTATE:ApplyGameCommand("urlnoexit,"..urlstringyo)
				end
			end,
			CollapseCommand=function(self)
				self:visible(false)
			end,
			ExpandCommand=function(self)
				self:visible(true):addy(-row2yoff)
			end,
		},
		
		LoadFont("Common normal") .. {	--percent
			InitCommand=function(self)
				self:x(c5x):zoom(tzoom+0.15):halign(1):valign(1)
				if collapsed then
					self:x(c5x):zoom(tzoom+0.15):halign(1):valign(0.5):maxwidth(30/tzoom)
				end
			end,
			DisplayCommand=function(self)
				self:settextf("%05.2f%%", hs:GetWifeScore()*10000/100):diffuse(byGrade(hs:GetWifeGrade()))
			end,
		},
		
		LoadFont("Common normal") .. {	--date
			InitCommand=function(self)
				if not collapsed then
					self:x(c5x):zoom(tzoom-0.05):halign(1):valign(0):maxwidth(width/4/tzoom):addy(row2yoff)
				end
			end,
			DisplayCommand=function(self)
				self:settext(hs:GetDate())
			end,
			CollapseCommand=function(self)
				self:visible(false)
			end,
			ExpandCommand=function(self)
				self:visible(true):addy(-row2yoff)
			end,
		},
	}
	return o
end

for i=1,numscores do
	o[#o+1] = makeScoreDisplay(i)
end

return o