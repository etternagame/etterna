local tzoom = 0.5
local pdh = 48 * tzoom
local ygap = 2
local packspaceY = pdh + ygap

local numgoals = 12
local ind = 0
local offx = 5
local width = SCREEN_WIDTH * 0.56
local dwidth = width - offx * 2
local height = (numgoals + 2) * packspaceY

local adjx = 10
local c0x = 20 -- for: priority and delete button
local c1x = c0x + 25 -- for: rate header, rate and percent
local c2x = c1x + (tzoom * 4 * adjx) -- for: song header and song name
local c5x = dwidth -- for: diff header, msd and steps diff
local c4x = c5x - adjx - (tzoom * 3.5 * adjx) -- for: date header, assigned, achieved
local c3x = c4x - adjx - (tzoom * 10 * adjx) -- for: filter header and song name
local headeroff = packspaceY / 1.5

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

local hoverAlpha = 0.6

local function byAchieved(scoregoal, nocolor, yescolor)
	if not scoregoal or scoregoal:IsAchieved() then
		return yescolor or Saturation(getMainColor("enabled"), 0.55) end
	return nocolor or color("#aaaaaa")
end
local filts = {
	THEME:GetString("TabGoals", "FilterAll"),
	THEME:GetString("TabGoals", "FilterCompleted"),
	THEME:GetString("TabGoals", "FilterIncomplete")
}

local translated_info = {
	PriorityLong = THEME:GetString("TabGoals", "PriorityLong"),
	PriorityShort = THEME:GetString("TabGoals", "PriorityShort"),
	RateLong = THEME:GetString("TabGoals", "RateLong"),
	RateShort = THEME:GetString("TabGoals", "RateShort"),
	Song = THEME:GetString("TabGoals", "Song"),
	Date = THEME:GetString("TabGoals", "Date"),
	Difficulty = THEME:GetString("TabGoals", "Difficulty"),
	Best = THEME:GetString("TabGoals", "Best"),
	Assigned = THEME:GetString("TabGoals", "AssignedDate"),
	Achieved = THEME:GetString("TabGoals", "AchievedDate"),
	Vacuous = THEME:GetString("TabGoals", "VacuousGoal"),
}

local goaltable
local o = Def.ActorFrame {
	Name = "GoalDisplay",
	InitCommand = function(self)
		cheese = self
		self:xy(0, 0)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	OnCommand = function(self)
		GetPlayerOrMachineProfile(PLAYER_1):SetFromAll()
		self:queuecommand("GoalTableRefresh")
	end,
	GoalTableRefreshMessageCommand = function(self)
		goaltable = GetPlayerOrMachineProfile(PLAYER_1):GetGoalTable()
		ind = 0
		self:queuecommand("Update")
	end,
	UpdateCommand = function(self)
		if ind == #goaltable then
			ind = ind - numgoals
		elseif ind > #goaltable - (#goaltable % numgoals) then
			ind = #goaltable - (#goaltable % numgoals)
		end
		if ind < 0 then
			ind = 0
		end
	end,
	DFRFinishedMessageCommand = function(self)
		GetPlayerOrMachineProfile(PLAYER_1):SetFromAll()
		self:queuecommand("GoalTableRefresh")
	end,
	NextPageCommand = function(self)
		ind = ind + numgoals
		self:queuecommand("Update")
	end,
	PrevPageCommand = function(self)
		ind = ind - numgoals
		self:queuecommand("Update")
	end,
	Def.Quad {
		Name = "FrameDisplay",
		InitCommand = function(self)
			self:zoomto(width, height - headeroff):halign(0):valign(0):diffuse(getMainColor("tabs"))
		end
	},
	-- headers
	Def.Quad {
		InitCommand = function(self)
			self:xy(offx, headeroff):zoomto(dwidth, pdh):halign(0):diffuse(getMainColor("frames"))
		end
	},
	UIElements.TextToolTip(1, 1, "Common normal") .. {
		--priority header
		InitCommand = function(self)
			self:xy(c0x, headeroff):zoom(tzoom):halign(0.5)
			self:diffuse(getMainColor("positive"))
		end,
		UpdateCommand = function(self)
			self:settext(translated_info["PriorityShort"])
		end,
		MouseOverCommand = function(self)
			self:settext(translated_info["PriorityLong"]):diffusealpha(0.6):x(c0x+9)
			self:GetParent():GetChild("RateHeader"):settext(translated_info["RateShort"]):x(c1x+12):zoom(tzoom/1.5)
		end,
		MouseOutCommand = function(self)
			self:settext(translated_info["PriorityShort"]):diffusealpha(1):x(c0x)
			self:GetParent():GetChild("RateHeader"):settext(translated_info["RateLong"]):x(c1x):zoom(tzoom)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				GetPlayerOrMachineProfile(PLAYER_1):SortByPriority()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common normal") .. {
		--rate header
		Name = "RateHeader",
		InitCommand = function(self)
			self:xy(c1x, headeroff):zoom(tzoom):halign(0.5):settext(translated_info["RateLong"])
			self:diffuse(getMainColor("positive"))
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				GetPlayerOrMachineProfile(PLAYER_1):SortByRate()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common normal") .. {
		--song header
		InitCommand = function(self)
			self:xy(c2x, headeroff):zoom(tzoom):halign(0):settext(translated_info["Song"])
			self:diffuse(getMainColor("positive"))
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				GetPlayerOrMachineProfile(PLAYER_1):SortByName()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end
	},
	LoadFont("Common normal") .. {
		--index header
		InitCommand = function(self)
			self:xy(width / 2, headeroff):zoom(tzoom):halign(0.5)
		end,
		UpdateCommand = function(self)
			self:settextf("%i-%i (%i)", ind + 1, ind + numgoals, #goaltable)
		end
	},
	UIElements.TextToolTip(1, 1, "Common normal") .. {
		--completed toggle // filter header
		InitCommand = function(self)
			self:xy(width/2 + capWideScale(37, 45), headeroff):zoom(tzoom):halign(0):settext(filts[1])
			self:diffuse(getMainColor("positive"))
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				GetPlayerOrMachineProfile(PLAYER_1):ToggleFilter()
				ind = 0
				self:settext(filts[GetPlayerOrMachineProfile(PLAYER_1):GetFilterMode()])
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common normal") .. {
		--date header
		InitCommand = function(self)
			self:xy(c4x - capWideScale(5, 35), headeroff):zoom(tzoom):halign(1):settext(translated_info["Date"])
			self:diffuse(getMainColor("positive"))
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				GetPlayerOrMachineProfile(PLAYER_1):SortByDate()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common normal") .. {
		--diff header
		InitCommand = function(self)
			self:xy(c5x, headeroff):zoom(tzoom):halign(1):settext(translated_info["Difficulty"])
			self:diffuse(getMainColor("positive"))
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				GetPlayerOrMachineProfile(PLAYER_1):SortByDiff()
				ind = 0
				self:GetParent():queuecommand("GoalTableRefresh")
			end
		end
	}
}

local function makeGoalDisplay(i)
	local sg
	local ck
	local goalsong
	local goalsteps

	local o = Def.ActorFrame {
		InitCommand = function(self)
			self:y(packspaceY * i + headeroff)
		end,
		UpdateCommand = function(self)
			sg = goaltable[(i + ind)]
			if sg then
				ck = sg:GetChartKey()
				goalsong = SONGMAN:GetSongByChartKey(ck)
				goalsteps = SONGMAN:GetStepsByChartKey(ck)
				self:queuecommand("Display")
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		Def.Quad {
			InitCommand = function(self)
				self:x(offx):zoomto(dwidth, pdh):halign(0)
			end,
			DisplayCommand = function(self)
				self:diffuse(color("#111111D9"))
			end
		},
		UIElements.TextToolTip(1, 1, "Common normal") .. {
			--priority
			InitCommand = function(self)
				self:x(c0x):zoom(tzoom):halign(0.5):valign(1)
			end,
			DisplayCommand = function(self)
				self:settext(sg:GetPriority())
				self:diffuse(byAchieved(sg, getMainColor("positive"),Color.White))
			end,
			MouseOverCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(hoverAlpha)
				end
			end,
			MouseOutCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(1)
				end
			end,
			MouseOverCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(hoverAlpha)
				end
			end,
			MouseOutCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(1)
				end
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and sg then
					sg:SetPriority(sg:GetPriority() + 1)
					self:GetParent():queuecommand("Update")
				elseif params.event == "DeviceButton_right mouse button" and sg then
					sg:SetPriority(sg:GetPriority() - 1)
					self:GetParent():queuecommand("Update")
				end
			end,
		},
		UIElements.SpriteButton(1, 1, THEME:GetPathG("", "X.png")) .. {
			-- delete button
			InitCommand = function(self)
				self:xy(c0x - 13,pdh/2.3):zoom(0.3):halign(0):valign(1):diffuse(Color.Red)
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(hoverAlpha)
			end,
			MouseOutCommand = function(self)
				self:diffusealpha(1)
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" then
					sg:Delete()
					GetPlayerOrMachineProfile(PLAYER_1):SetFromAll()
					self:GetParent():GetParent():queuecommand("GoalTableRefresh")
				end
			end
		},
		UIElements.TextToolTip(1, 1, "Common normal") .. {
			--rate
			InitCommand = function(self)
				self:x(c1x):zoom(tzoom):halign(0.5):valign(1)
			end,
			DisplayCommand = function(self)
				local ratestring = string.format("%.2f", sg:GetRate()):gsub("%.?0$", "") .. "x"
				self:settext(ratestring)
				self:diffuse(byAchieved(sg, getMainColor("positive")))
			end,
			MouseOverCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(hoverAlpha)
				end
			end,
			MouseOutCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(1)
				end
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and sg then
					sg:SetRate(sg:GetRate() + 0.1)
					self:GetParent():queuecommand("Update")
				elseif params.event == "DeviceButton_right mouse button" and sg then
					sg:SetRate(sg:GetRate() - 0.1)
					self:GetParent():queuecommand("Update")
				end
			end,
		},
		UIElements.TextToolTip(1, 1, "Common normal") .. {
			--percent
			InitCommand = function(self)
				self:x(c1x):zoom(tzoom):halign(0.5):valign(0):maxwidth((50 - capWideScale(10, 10)) / tzoom)
			end,
			DisplayCommand = function(self)
				local perc = notShit.round(sg:GetPercent() * 100000) / 1000
				if perc <= 99 or perc == 100 then
					self:settextf("%.f%%", perc)
				elseif (perc < 99.8) then
					self:settextf("%.2f%%", perc)
				else
					self:settextf("%.3f%%", perc)
				end
				self:diffuse(byAchieved(sg, getMainColor("positive")))
			end,
			MouseOverCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(hoverAlpha)
				end
			end,
			MouseOutCommand = function(self)
				if sg and not sg:IsAchieved() then
					self:diffusealpha(1)
				end
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and sg then
					sg:SetPercent(sg:GetPercent() + 0.01)
					self:GetParent():queuecommand("Update")
				elseif params.event == "DeviceButton_right mouse button" and sg then
					sg:SetPercent(sg:GetPercent() - 0.01)
					self:GetParent():queuecommand("Update")
				end
			end,
		},
		UIElements.TextToolTip(1, 1, "Common normal") .. {
			--song name
			InitCommand = function(self)
				self:x(c2x):zoom(tzoom):maxwidth((c3x - c2x - capWideScale(32, 62)) / tzoom):halign(0):valign(1):draworder(1)
			end,
			DisplayCommand = function(self)
				if goalsong then
					self:settext(goalsong:GetDisplayMainTitle()):diffuse(getMainColor("positive"))
				else
					self:settext(sg:GetChartKey()):diffuse(getMainColor("negative"))
				end
			end,
			MouseOverCommand = function(self)
				self:diffusealpha(hoverAlpha)
			end,
			MouseOutCommand = function(self)
				self:diffusealpha(1)
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and sg ~= nil and goalsong and goalsteps then
					local success = SCREENMAN:GetTopScreen():GetMusicWheel():SelectSong(goalsong)
					if success then
						GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred"):MusicRate(sg:GetRate())
						GAMESTATE:GetSongOptionsObject("ModsLevel_Song"):MusicRate(sg:GetRate())
						GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate(sg:GetRate())
						MESSAGEMAN:Broadcast("GoalSelected")
					end
				end
			end
		},
		LoadFont("Common normal") .. {
			--pb
			InitCommand = function(self)
				self:x(c2x):zoom(tzoom):halign(0):valign(0)
			end,
			DisplayCommand = function(self)
				local pb = sg:GetPBUpTo()
				if pb then
					local pbwife = pb:GetWifeScore() * 100
					local pbstr = ""
					if pbwife > 99.65 then
						pbstr = string.format("%05.4f%%", notShit.floor(pbwife, 4))
					else
						pbstr = string.format("%05.2f%%", notShit.floor(pbwife, 2))
					end
					if pb:GetMusicRate() < sg:GetRate() then
						local ratestring = string.format("%.2f", pb:GetMusicRate()):gsub("%.?0$", "") .. "x"
						self:settextf("%s: %s (%s)", translated_info["Best"], pbstr, ratestring)
					else
						self:settextf("%s: %s", translated_info["Best"], pbstr)
					end
					self:diffuse(getGradeColor(pb:GetWifeGrade()))
					self:visible(true)
				else
					self:settextf("(%s: %5.2f%%)", translated_info["Best"], 0)
					self:diffuse(byAchieved(sg))
				end
			end
		},
		LoadFont("Common normal") .. {
			--assigned
			InitCommand = function(self)
				self:x(c4x):zoom(tzoom):halign(1):valign(0):maxwidth(width / 4 / tzoom)
			end,
			DisplayCommand = function(self)
				self:settextf("%s: %s", translated_info["Assigned"], sg:WhenAssigned()):diffuse(byAchieved(sg))
			end
		},
		LoadFont("Common normal") .. {
			--achieved
			InitCommand = function(self)
				self:x(c4x):zoom(tzoom):halign(1):valign(1):maxwidth(width / 4 / tzoom)
			end,
			DisplayCommand = function(self)
				if sg:IsAchieved() then
					self:settextf("%s: %s", translated_info["Achieved"], sg:WhenAchieved())
				elseif sg:IsVacuous() then
					self:settext(translated_info["Vacuous"])
				else
					self:settext("")
				end
				self:diffuse(byAchieved(sg))
			end
		},
		LoadFont("Common normal") .. {
			--msd diff
			InitCommand = function(self)
				self:x(c5x):zoom(tzoom):halign(1):valign(1)
			end,
			DisplayCommand = function(self)
				if goalsteps then
					local msd = goalsteps:GetMSD(sg:GetRate(), 1)
					self:settextf("%5.1f", msd):diffuse(byMSD(msd))
				else
					self:settext("??")
				end
			end
		},
		LoadFont("Common normal") .. {
			--steps diff
			InitCommand = function(self)
				self:x(c5x):zoom(tzoom):halign(1):valign(0)
			end,
			DisplayCommand = function(self)
				if goalsteps and goalsong then
					local diff = goalsteps:GetDifficulty()
					self:settext(getShortDifficulty(diff))
					self:diffuse(byDifficulty(diff))
				else
					self:settext("??")
					self:diffuse(getMainColor("negative"))
				end
			end
		},
	}
	return o
end

for i = 1, numgoals do
	o[#o + 1] = makeGoalDisplay(i)
end

return o
