local tzoom = 0.5
local pdh = 48 * tzoom
local ygap = 2
local packspaceY = pdh + ygap
local currentCountry = "Global"

local numscores = 13
local ind = 0
local offx = 5
local width = SCREEN_WIDTH * 0.56
local dwidth = width - offx * 2
local height = (numscores + 2) * packspaceY - packspaceY / 3 -- account dumbly for header being moved up

local adjx = 14
local c0x = 10
local c1x = 20 + c0x
local c2x = c1x + (tzoom * 7 * adjx) -- guesswork adjustment for epxected text length
local c5x = dwidth -- right aligned cols
local c4x = c5x - adjx - (tzoom * 3 * adjx) -- right aligned cols
local c3x = c4x - adjx - (tzoom * 10 * adjx) -- right aligned cols
local headeroff = packspaceY / 2
local row2yoff = 1
local moving
local cheese
local collapsed = false

local isGlobalRanking = true

-- will eat any mousewheel inputs to scroll pages while mouse is over the background frame
local function input(event)
	if isOver(cheese:GetChild("FrameDisplay")) then -- visibility checks are built into isover now -mina
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
	if self:IsVisible() then
		self:queuecommand("Highlight")
		self:queuecommand("WHAZZZAAAA")
	end
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
		return getMainColor("positive")
	end
	return color("#aaaaaa")
end

local filts = {
	THEME:GetString("NestedScores", "FilterAll"),
	THEME:GetString("NestedScores", "FilterCurrent")
}
local topornah = {
	THEME:GetString("NestedScores", "ScoresTop"),
	THEME:GetString("NestedScores", "ScoresAll")
}
local ccornah = {
	THEME:GetString("NestedScores", "ShowInvalid"),
	THEME:GetString("NestedScores", "HideInvalid")
}

local translated_info = {
	LoginToView = THEME:GetString("NestedScores", "LoginToView"),
	NoScoresFound = THEME:GetString("NestedScores", "NoScoresFound"),
	RetrievingScores = THEME:GetString("NestedScores", "RetrievingScores"),
	Watch = THEME:GetString("NestedScores", "WatchReplay")
}

local scoretable = {}
local o =
	Def.ActorFrame {
	Name = "ScoreDisplay",
	InitCommand = function(self)
		cheese = self
		self:SetUpdateFunction(highlight)
		self:SetUpdateFunctionInterval(0.05)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
		self:playcommand("Update")
	end,
	GetFilteredLeaderboardCommand = function(self)
		if GAMESTATE:GetCurrentSong() then
			scoretable = DLMAN:GetChartLeaderBoard(GAMESTATE:GetCurrentSteps(PLAYER_1):GetChartKey(), currentCountry)
			ind = 0
			self:playcommand("Update")
		end
	end,
	SetFromLeaderboardCommand = function(self, lb)
		scoretable = lb
		ind = 0
		self:playcommand("GetFilteredLeaderboard") -- we can move all the filter stuff to lua so we're not being dumb hurr hur -mina
		self:playcommand("Update")
	end,
	UpdateCommand = function(self)
		if not scoretable then
			scoretable = {}
		end
		if ind == #scoretable then
			ind = ind - numscores
		elseif ind > #scoretable - (#scoretable % numscores) then
			ind = #scoretable - (#scoretable % numscores)
		end
		if ind < 0 then
			ind = 0
		end
	end,
	NextPageCommand = function(self)
		ind = ind + numscores
		self:queuecommand("Update")
	end,
	PrevPageCommand = function(self)
		ind = ind - numscores
		self:queuecommand("Update")
	end,
	CollapseCommand = function(self)
		tzoom = 0.5 * 0.75
		pdh = 38 * tzoom
		ygap = 2
		packspaceY = pdh + ygap

		numscores = 10
		ind = 0
		offx = 5
		width = math.max(SCREEN_WIDTH * 0.25, 240)
		dwidth = width - offx * 2
		height = (numscores + 2) * packspaceY

		adjx = 14
		c0x = 10
		c1x = 10 + c0x
		c2x = c1x + (tzoom * 7 * adjx)
		c5x = dwidth
		c4x = c5x - adjx - (tzoom * 3 * adjx)
		c3x = c4x - adjx - (tzoom * 10 * adjx)
		headeroff = packspaceY / 2
		row2yoff = 1
		collapsed = true
		self:diffusealpha(0.8)

		if
			-- a generic bounds check function that snaps an actor onto the screen or within specified coordinates should be added as an actor member, ie, not this -mina
			FILTERMAN:grabposx("ScoreDisplay") <= 10 or FILTERMAN:grabposy("ScoreDisplay") <= 45 or
				FILTERMAN:grabposx("ScoreDisplay") >= SCREEN_WIDTH - 60 or
				FILTERMAN:grabposy("ScoreDisplay") >= SCREEN_HEIGHT - 45
		 then
			self:xy(10, 45)
		else
			self:LoadXY()
		end

		FILTERMAN:HelpImTrappedInAChineseFortuneCodingFactory(true)
		self:playcommand("Init")
	end,
	ExpandCommand = function(self)
		tzoom = 0.5
		pdh = 48 * tzoom
		ygap = 2
		packspaceY = pdh + ygap

		numscores = 13
		ind = 0
		offx = 5
		width = SCREEN_WIDTH * 0.56
		dwidth = width - offx * 2
		height = (numscores + 2) * packspaceY - packspaceY / 3

		adjx = 14
		c0x = 10
		c1x = 20 + c0x
		c2x = c1x + (tzoom * 7 * adjx) -- guesswork adjustment for epxected text length
		c5x = dwidth -- right aligned cols
		c4x = c5x - adjx - (tzoom * 3 * adjx) -- right aligned cols
		c3x = c4x - adjx - (tzoom * 10 * adjx) -- right aligned cols
		headeroff = packspaceY / 1.5
		row2yoff = 1
		collapsed = false
		self:diffusealpha(1)
		FILTERMAN:HelpImTrappedInAChineseFortuneCodingFactory(false)
		self:playcommand("Init")
	end,
	Def.Quad {
		Name = "FrameDisplay",
		InitCommand = function(self)
			self:zoomto(width, height - headeroff):halign(0):valign(0):diffuse(color("#333333"))
		end,
		HighlightCommand = function(self)
			if isOver(self) and collapsed then
				self:diffusealpha(1)
			else
				self:diffusealpha(0.8)
			end
		end,
		MouseRightClickMessageCommand = function(self)
			if isOver(self) and not collapsed then
				FILTERMAN:HelpImTrappedInAChineseFortuneCodingFactory(true)
				self:GetParent():GetParent():playcommand("Collapse")
			elseif isOver(self) then
				self:GetParent():GetParent():playcommand("Expand")
				SCREENMAN:GetTopScreen():PausePreviewNoteField()
			end
		end
	},
	-- headers
	Def.Quad {
		Name = "HeaderBar",
		InitCommand = function(self)
			self:zoomto(width, pdh - 8 * tzoom):halign(0):diffuse(getMainColor("frames")):diffusealpha(0.5):valign(0)
		end
	},
	-- grabby thing
	Def.Quad {
		InitCommand = function(self)
			self:xy(dwidth / 4, headeroff):zoomto(dwidth - dwidth / 4, pdh - 8 * tzoom):halign(0):diffuse(getMainColor("frames")):diffusealpha(
				0.5
			):valign(1)
		end,
		WHAZZZAAAACommand = function(self)
			if isOver(self) and collapsed then
				self:diffusealpha(0.6):diffuse(color("#fafafa"))
				if INPUTFILTER:IsBeingPressed("Mouse 0", "Mouse") then
					self:diffusealpha(0):zoomto(400, 400):valign(0.5):halign(0.5)
					local nx = INPUTFILTER:GetMouseX() - width / 2
					local ny = INPUTFILTER:GetMouseY() - self:GetY()
					self:GetParent():SaveXY(nx, ny) -- this can probably be wrapped for convenience -mina
					self:GetParent():LoadXY()
				else
					self:zoomto(dwidth / 2, pdh / 2):valign(1):halign(0)
				end
			else
				self:diffuse(getMainColor("frames")):diffusealpha(0)
			end
		end
	},
	LoadFont("Common normal") ..
		{
			-- informational text about online scores
			Name = "RequestStatus",
			InitCommand = function(self)
				if collapsed then
					self:xy(c1x, headeroff + 15):zoom(tzoom):halign(0)
				else
					self:xy(c1x, headeroff + 25):zoom(tzoom):halign(0)
				end
			end,
			UpdateCommand = function(self)
				local numberofscores = #scoretable
				local online = DLMAN:IsLoggedIn()
				if not GAMESTATE:GetCurrentSong() then
					self:settext("")
				elseif not online and #scoretable == 0 then
					self:settext(translated_info["LoginToView"])
				else
					if #scoretable == 0 then
						self:settext(translated_info["NoScoresFound"])
					else
						self:settext("")
					end
				end
			end,
			CurrentSongChangedMessageCommand = function(self)
				local online = DLMAN:IsLoggedIn()
				if not GAMESTATE:GetCurrentSong() then
					self:settext("")
				elseif not online and #scoretable == 0 then
					self:settext(translated_info["LoginToView"])
				else
					self:settext(translated_info["NoScoresFound"])
				end
			end
		},
	LoadFont("Common normal") ..
		{
			--current rate toggle
			InitCommand = function(self)
				self:xy(c5x, headeroff):zoom(tzoom):halign(1):valign(1)
			end,
			HighlightCommand = function(self)
				highlightIfOver(self)
			end,
			UpdateCommand = function(self)
				if DLMAN:GetCurrentRateFilter() then
					self:settext(filts[2])
				else
					self:settext(filts[1])
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) then
					DLMAN:ToggleRateFilter()
					ind = 0
					self:GetParent():queuecommand("GetFilteredLeaderboard")
				end
			end
		},
	LoadFont("Common normal") ..
		{
			--top score/all score toggle
			InitCommand = function(self)
				if collapsed then
					self:xy(c5x - 175, headeroff):zoom(tzoom):halign(1):valign(1)
				else
					self:xy(c5x - 160, headeroff):zoom(tzoom):halign(1):valign(1)
				end
			end,
			HighlightCommand = function(self)
				highlightIfOver(self)
			end,
			UpdateCommand = function(self)
				if DLMAN:GetTopScoresOnlyFilter() then
					self:settext(topornah[1])
				else
					self:settext(topornah[2])
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) then
					DLMAN:ToggleTopScoresOnlyFilter()
					ind = 0
					self:GetParent():queuecommand("GetFilteredLeaderboard")
				end
			end
		},
	LoadFont("Common normal") ..
		{
			--ccon/off filter toggle
			InitCommand = function(self)
				if collapsed then
					self:visible(false)
					--self:xy(c5x - 110, headeroff):zoom(tzoom):halign(1):valign(1)
				else
					self:visible(true)
					self:xy(c5x - 80, headeroff):zoom(tzoom):halign(1):valign(1)
				end
			end,
			HighlightCommand = function(self)
				highlightIfOver(self)
			end,
			UpdateCommand = function(self)
				if DLMAN:GetCCFilter() then
					self:settext(ccornah[1])
				else
					self:settext(ccornah[2])
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) then
					DLMAN:ToggleCCFilter()
					ind = 0
					self:GetParent():queuecommand("GetFilteredLeaderboard")
				end
			end
		}
}

local function makeScoreDisplay(i)
	local hs

	local o =
		Def.ActorFrame {
		InitCommand = function(self)
			self:y(packspaceY * i + headeroff)
			if i > numscores then
				self:visible(false)
			end
			self:SetUpdateFunction(function(self)
				self:queuecommand("PercentMouseover")
			end)
			self:SetUpdateFunctionInterval(0.025)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:visible(false)
		end,
		UpdateCommand = function(self)
			hs = scoretable[(i + ind)]
			if hs and i <= numscores then
				self:visible(true)
				self:playcommand("Display")
			else
				self:visible(false)
			end
		end,
		Def.Quad {
			InitCommand = function(self)
				self:x(offx):zoomto(dwidth, pdh):halign(0)
			end,
			DisplayCommand = function(self)
				self:diffuse(color("#111111CC"))
			end,
			HighlightCommand = function(self)
				if isOver(self) then
					self:diffusealpha(0.8)
				else
					self:diffusealpha(0.6)
				end
			end
		},
		LoadFont("Common normal") ..
			{
				--rank
				InitCommand = function(self)
					self:x(c0x):zoom(tzoom):halign(0):valign(0)
					if collapsed then
						self:x(c0x):zoom(tzoom):halign(0):valign(0.5)
					end
				end,
				DisplayCommand = function(self)
					self:settextf("%i.", i + ind)
				end
			},
		LoadFont("Common normal") ..
			{
				--ssr
				InitCommand = function(self)
					self:x(c2x - c1x + offx):zoom(tzoom + 0.05):halign(0.5):valign(1)
					if collapsed then
						self:x(46):zoom(tzoom + 0.15):halign(0.5):valign(0.5):maxwidth(20 / tzoom)
					end
				end,
				DisplayCommand = function(self)
					local ssr = hs:GetSkillsetSSR("Overall")
					self:settextf("%.2f", ssr):diffuse(byMSD(ssr))
				end
			},
		LoadFont("Common normal") ..
			{
				--rate
				InitCommand = function(self)
					self:x(c2x - c1x + offx):zoom(tzoom - 0.05):halign(0.5):valign(0):addy(row2yoff)
					if collapsed then
						self:x(c4x - 14):zoom(tzoom):halign(1):valign(0.5):addy(-row2yoff):maxwidth(30 / tzoom)
					end
				end,
				DisplayCommand = function(self)
					local ratestring = string.format("%.2f", hs:GetMusicRate()):gsub("%.?0$", "") .. "x"
					self:settext(ratestring)
				end,
				ExpandCommand = function(self)
					self:addy(-row2yoff)
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "Burt" .. i,
				InitCommand = function(self)
					self:x(c2x):zoom(tzoom + 0.1):maxwidth((c3x - c2x - capWideScale(10, 40)) / tzoom):halign(0):valign(1)
					if collapsed then
						self:x(c2x + 10):maxwidth(60 / tzoom):zoom(tzoom + 0.2):valign(0.5)
					end
				end,
				DisplayCommand = function(self)
					self:settext(hs:GetDisplayName())
					if hs:GetChordCohesion() then
						self:diffuse(color("#F0EEA6"))
					else
						self:diffuse(getMainColor("positive"))
					end
				end,
				HighlightCommand = function(self)
					highlightIfOver(self)
				end,
				MouseLeftClickMessageCommand = function(self)
					if isOver(self) then
						local urlstringyo = "https://etternaonline.com/user/" .. hs:GetDisplayName()
						GAMESTATE:ApplyGameCommand("urlnoexit," .. urlstringyo)
					end
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "Ernie" .. i,
				InitCommand = function(self)
					if not collapsed then
						self:x(c2x):zoom(tzoom - 0.05):halign(0):valign(0):maxwidth(width / 2 / tzoom):addy(row2yoff)
					end
				end,
				DisplayCommand = function(self)
					self:settext(hs:GetJudgmentString())
					if hs:GetChordCohesion() then
						self:diffuse(color("#F0EEA6"))
					else
						self:diffuse(getMainColor("positive"))
					end
				end,
				HighlightCommand = function(self)
					highlightIfOver(self)
				end,
				MouseLeftClickMessageCommand = function(self)
					if isOver(self) then
						local urlstringyo = "https://etternaonline.com/score/view/" .. hs:GetScoreid() .. hs:GetUserid()
						GAMESTATE:ApplyGameCommand("urlnoexit," .. urlstringyo)
					end
				end,
				CollapseCommand = function(self)
					self:visible(false)
				end,
				ExpandCommand = function(self)
					self:visible(true):addy(-row2yoff)
				end
			},
		LoadFont("Common normal") ..
			{
				Name = "Replay" .. i,
				InitCommand = function(self)
					if not collapsed then
						self:x(capWideScale(c3x + 52, c3x)):zoom(tzoom - 0.05):halign(1):valign(0):maxwidth(width / 2 / tzoom):addy(
							row2yoff
						):diffuse(getMainColor("enabled"))
					end
				end,
				BeginCommand = function(self)
					if SCREENMAN:GetTopScreen():GetName() == "ScreenNetSelectMusic" then
						self:visible(false)
					end
				end,
				DisplayCommand = function(self)
					if GAMESTATE:GetCurrentSteps(PLAYER_1) then
						if hs:HasReplayData() then
							self:settext(translated_info["Watch"])
						else
							self:settext("")
						end
					end
				end,
				HighlightCommand = function(self)
					highlightIfOver(self)
				end,
				MouseLeftClickMessageCommand = function(self)
					if isOver(self) and hs then
						DLMAN:RequestOnlineScoreReplayData(
							hs,
							function()
								SCREENMAN:GetTopScreen():PlayReplay(hs)
							end
						)
					end
				end,
				CollapseCommand = function(self)
					self:visible(false)
				end,
				ExpandCommand = function(self)
					self:visible(true):addy(-row2yoff)
				end
			},
		Def.Quad {
				InitCommand = function(self)
					self:x(c5x):zoomto(50,10):halign(1):valign(1)
					if collapsed then
						self:x(c5x):zoomto(40, 10):halign(1):valign(0.5)
					end
					self:diffusealpha(0)
				end,
				PercentMouseoverCommand = function(self)
					if isOver(self) and self:IsVisible() then
						self:GetParent():GetChild("NormalText"):visible(false)
						self:GetParent():GetChild("LongerText"):visible(true)
					else
						self:GetParent():GetChild("NormalText"):visible(true)
						self:GetParent():GetChild("LongerText"):visible(false)
					end
				end,
				MouseLeftClickMessageCommand = function(self)
					if isOver(self) and hs and not collapsed then
						if SCREENMAN:GetTopScreen():GetName() == "ScreenNetSelectMusic" then return end
						if hs:HasReplayData() then
							DLMAN:RequestOnlineScoreReplayData(
								hs,
								function()
									setScoreForPlot(hs)
									SCREENMAN:AddNewScreenToTop("ScreenScoreTabOffsetPlot")					
								end
							)
						end
					end
				end
			},
		LoadFont("Common normal") ..
			{
				--percent
				Name="NormalText",
				InitCommand = function(self)
					self:x(c5x):zoom(tzoom + 0.15):halign(1):valign(1)
					if collapsed then
						self:x(c5x):zoom(tzoom + 0.15):halign(1):valign(0.5):maxwidth(30 / tzoom)
					end
				end,
				DisplayCommand = function(self)
					self:settextf("%05.2f%%", notShit.floor(hs:GetWifeScore() * 100, 2)):diffuse(byGrade(hs:GetWifeGrade()))
				end
			},
			LoadFont("Common normal") ..
			{
				--percent
				Name="LongerText",
				InitCommand = function(self)
					self:x(c5x):zoom(tzoom + 0.15):halign(1):valign(1)
					if collapsed then
						self:x(c5x):zoom(tzoom + 0.15):halign(1):valign(0.5):maxwidth(30 / tzoom)
					end
				end,
				DisplayCommand = function(self)
					self:settextf("%05.4f%%", notShit.floor(hs:GetWifeScore() * 100, 5)):diffuse(byGrade(hs:GetWifeGrade()))
				end
			},
		LoadFont("Common normal") ..
			{
				--date
				InitCommand = function(self)
					if not collapsed then
						self:x(c5x):zoom(tzoom - 0.05):halign(1):valign(0):maxwidth(width / 4 / tzoom):addy(row2yoff)
					end
				end,
				DisplayCommand = function(self)
					if IsUsingWideScreen() then
						self:settext(hs:GetDate())
					else
						self:settext(hs:GetDate():sub(1, 10))
					end
				end,
				CollapseCommand = function(self)
					self:visible(false)
				end,
				ExpandCommand = function(self)
					self:visible(true):addy(-row2yoff)
				end
			}
	}
	return o
end

for i = 1, numscores do
	o[#o + 1] = makeScoreDisplay(i)
end

--[[
--Commented for now
-- Todo: make the combobox scrollable
-- To handle a large amount of choices
local countryDropdown
countryDropdown =
	Widg.ComboBox {
	onSelectionChanged = function(newChoice)
		currentCountry = newChoice
		cheese:queuecommand("ChartLeaderboardUpdate")
	end,
	choice = "Global",
	choices = DLMAN:GetCountryCodes(),
	commands = {
		CollapseCommand = function(self)
			self:xy(c5x - 20, headeroff - 20):halign(0)
		end,
		ExpandCommand = function(self)
			self:xy(c5x - 89, headeroff)
		end,
		ChartLeaderboardUpdateMessageCommand = function(self)
			self:visible(DLMAN:IsLoggedIn())
		end
	},
	selectionColor = color("#111111"),
	itemColor = color("#111111"),
	hoverColor = getMainColor("highlight"),
	height = tzoom * 29,
	width = 50,
	x = c5x - 89, -- needs to be thought out for design purposes
	y = headeroff,
	visible = DLMAN:IsLoggedIn(),
	numitems = 4
}
o[#o + 1] = countryDropdown
]]
return o
