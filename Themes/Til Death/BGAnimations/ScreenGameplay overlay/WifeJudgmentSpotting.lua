--[[
	Basically rewriting the c++ code to not be total shit so this can also not be total shit.
]]
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local practiceMode = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingPractice()
local jcKeys = tableKeys(colorConfig:get_data().judgment)
local jcT = {} -- A "T" following a variable name will designate an object of type table.

for i = 1, #jcKeys do
	jcT[jcKeys[i]] = byJudgment(jcKeys[i])
end

local jdgT = {
	-- Table of judgments for the judgecounter
	"TapNoteScore_W1",
	"TapNoteScore_W2",
	"TapNoteScore_W3",
	"TapNoteScore_W4",
	"TapNoteScore_W5",
	"TapNoteScore_Miss",
	"HoldNoteScore_Held",
	"HoldNoteScore_LetGo"
}

local dvCur
local jdgCur  -- Note: only for judgments with OFFSETS, might reorganize a bit later
local tDiff
local wifey
local judgect
local pbtarget
local positive = getMainColor("positive")
local highlight = getMainColor("highlight")
local negative = getMainColor("negative")

local jdgCounts = {} -- Child references for the judge counter

-- We can also pull in some localized aliases for workhorse functions for a modest speed increase
local Round = notShit.round
local Floor = notShit.floor
local diffusealpha = Actor.diffusealpha
local diffuse = Actor.diffuse
local finishtweening = Actor.finishtweening
local linear = Actor.linear
local x = Actor.x
local y = Actor.y
local addx = Actor.addx
local addy = Actor.addy
local Zoomtoheight = Actor.zoomtoheight
local Zoomtowidth = Actor.zoomtowidth
local Zoomm = Actor.zoom
local queuecommand = Actor.queuecommand
local playcommand = Actor.queuecommand
local settext = BitmapText.settext
local Broadcast = MessageManager.Broadcast

-- these dont really work as borders since they have to be destroyed/remade in order to scale width/height
-- however we can use these to at least make centered snap lines for the screens -mina
local function dot(height, x)
	return Def.Quad {
		InitCommand = function(self)
			self:zoomto(dotwidth, height)
			self:addx(x)
		end
	}
end

local function dottedline(len, height, x, y, rot)
	local t =
		Def.ActorFrame {
		InitCommand = function(self)
			self:xy(x, y):addrotationz(rot)
			if x == 0 and y == 0 then
				self:diffusealpha(0.65)
			end
		end
	}
	local numdots = len / dotwidth
	t[#t + 1] = dot(height, 0)
	for i = 1, numdots / 4 do
		t[#t + 1] = dot(height, i * dotwidth * 2 - dotwidth / 2)
	end
	for i = 1, numdots / 4 do
		t[#t + 1] = dot(height, -i * dotwidth * 2 + dotwidth / 2)
	end
	return t
end

local function DottedBorder(width, height, bw, x, y)
	return Def.ActorFrame {
		Name = "Border",
		InitCommand = function(self)
			self:xy(x, y):visible(false):diffusealpha(0.35)
		end,
		dottedline(width, bw, 0, 0, 0),
		dottedline(width, bw, 0, height / 2, 0),
		dottedline(width, bw, 0, -height / 2, 0),
		dottedline(height, bw, 0, 0, 90),
		dottedline(height, bw, width / 2, 0, 90),
		dottedline(height, bw, -width / 2, 0, 90)
	}
end

-- Screenwide params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
isCentered = PREFSMAN:GetPreference("Center1Player")
local CenterX = SCREEN_CENTER_X
local mpOffset = 0
if not isCentered then
	CenterX =
		THEME:GetMetric(
		"ScreenGameplay",
		string.format("PlayerP1%sX", ToEnumShortString(GAMESTATE:GetCurrentStyle():GetStyleType()))
	)
	mpOffset = SCREEN_CENTER_X
end
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local screen  -- the screen after it is loaded

local WIDESCREENWHY = -5
local WIDESCREENWHX = -5

--error bar things
local errorBarFrameWidth = capWideScale(get43size(MovableValues.ErrorBarWidth), MovableValues.ErrorBarWidth)
local wscale = errorBarFrameWidth / 180

--differential tracker things
local targetTrackerMode = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTrackerMode

--receptor/Notefield things
local Notefield
local noteColumns
local usingReverse

--guess checking if things are enabled before changing them is good for not having a log full of errors
local enabledErrorBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ErrorBar
local enabledMiniBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).MiniProgressBar
local enabledFullBar = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).FullProgressBar
local enabledTargetTracker = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetTracker
local enabledDisplayPercent = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).DisplayPercent
local enabledJudgeCounter = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgeCounter
local leaderboardEnabled = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).leaderboardEnabled and DLMAN:IsLoggedIn()
local isReplay = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerController() == "PlayerController_Replay"

local function arbitraryErrorBarValue(value)
	errorBarFrameWidth = capWideScale(get43size(value), value)
	wscale = errorBarFrameWidth / 180
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
								     **Wife deviance tracker. Basically half the point of the theme.**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	For every doot there is an equal and opposite scoot.
]]
local t =
	Def.ActorFrame {
	Name = "WifePerch",
	OnCommand = function(self)
		if allowedCustomization then
			-- auto enable autoplay
			GAMESTATE:SetAutoplay(true)
		else
			GAMESTATE:SetAutoplay(false)
		end
		-- Discord thingies
		local largeImageTooltip =
			GetPlayerOrMachineProfile(PLAYER_1):GetDisplayName() ..
			": " .. string.format("%5.2f", GetPlayerOrMachineProfile(PLAYER_1):GetPlayerRating())
		local detail =
			GAMESTATE:GetCurrentSong():GetDisplayMainTitle() ..
			" " ..
				string.gsub(getCurRateDisplayString(), "Music", "") .. " [" .. GAMESTATE:GetCurrentSong():GetGroupName() .. "]"
		-- truncated to 128 characters(discord hard limit)
		detail = #detail < 128 and detail or string.sub(detail, 1, 124) .. "..."
		local state = "MSD: " .. string.format("%05.2f", GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(), 1))
		local endTime = os.time() + GetPlayableTime()
		GAMESTATE:UpdateDiscordPresence(largeImageTooltip, detail, state, endTime)

		-- now playing thing for streamers
		local streamerstuff =
			"Now playing " ..
			GAMESTATE:GetCurrentSong():GetDisplayMainTitle() ..
				" by " ..
					GAMESTATE:GetCurrentSong():GetDisplayArtist() ..
						" in " .. GAMESTATE:GetCurrentSong():GetGroupName() .. " " .. state
		File.Write("nowplaying.txt", streamerstuff)

		screen = SCREENMAN:GetTopScreen()
		usingReverse = GAMESTATE:GetPlayerState(PLAYER_1):GetCurrentPlayerOptions():UsingReverse()
		Notefield = screen:GetChild("PlayerP1"):GetChild("NoteField")
		Notefield:addy(MovableValues.NotefieldY * (usingReverse and 1 or -1))
		Notefield:addx(MovableValues.NotefieldX)
		noteColumns = Notefield:get_column_actors()
		-- lifebar stuff
		local lifebar = SCREENMAN:GetTopScreen():GetLifeMeter(PLAYER_1)

		if (allowedCustomization) then
			Movable.pressed = false
			Movable.current = "None"
			Movable.DeviceButton_r.element = Notefield
			Movable.DeviceButton_t.element = noteColumns
			Movable.DeviceButton_r.condition = true
			Movable.DeviceButton_t.condition = true
			self:GetChild("LifeP1"):GetChild("Border"):SetFakeParent(lifebar)
			Movable.DeviceButton_j.element = lifebar
			Movable.DeviceButton_j.condition = true
			Movable.DeviceButton_k.element = lifebar
			Movable.DeviceButton_k.condition = true
			Movable.DeviceButton_l.element = lifebar
			Movable.DeviceButton_l.condition = true
		end

		lifebar:zoomtowidth(MovableValues.LifeP1Width)
		lifebar:zoomtoheight(MovableValues.LifeP1Height)
		lifebar:xy(MovableValues.LifeP1X, MovableValues.LifeP1Y)
		lifebar:rotationz(MovableValues.LifeP1Rotation)

		for i, actor in ipairs(noteColumns) do
			actor:zoomtowidth(MovableValues.NotefieldWidth)
			actor:zoomtoheight(MovableValues.NotefieldHeight)
		end
	end,
	DoneLoadingNextSongMessageCommand = function(self)
		-- put notefield y pos back on doneloadingnextsong because playlist courses reset this for w.e reason -mina
		screen = SCREENMAN:GetTopScreen()

		-- nil checks are needed because these don't exist when doneloadingnextsong is sent initially
		-- which is convenient for us since addy -mina
		if screen ~= nil and screen:GetChild("PlayerP1") ~= nil then
			Notefield = screen:GetChild("PlayerP1"):GetChild("NoteField")
			Notefield:addy(MovableValues.NotefieldY * (usingReverse and 1 or -1))
		end
	end,
	JudgmentMessageCommand = function(self, msg)
		tDiff = msg.WifeDifferential
		wifey = Floor(msg.WifePercent * 100) / 100
		jdgct = msg.Val
		if msg.Offset ~= nil then
			dvCur = msg.Offset
		else
			dvCur = nil
		end
		if msg.WifePBGoal ~= nil and targetTrackerMode ~= 0 then
			pbtarget = msg.WifePBGoal
			tDiff = msg.WifePBDifferential
		end
		jdgCur = msg.Judgment
		queuecommand(self, "SpottedOffset")
	end
}

-- lifebard
t[#t + 1] =
	Def.ActorFrame {
	Name = "LifeP1",
	MovableBorder(200, 5, 1, -35, 0)
}
--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
																	**LaneCover**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Old scwh lanecover back for now. Equivalent to "screencutting" on ffr; essentially hides notes for a fixed distance before they appear
on screen so you can adjust the time arrows display on screen without modifying their spacing from each other.
]]
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).LaneCover then
	t[#t + 1] = LoadActor("lanecover")
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 					    	**Player Target Differential: Ghost target rewrite, average score gone for now**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Point differential to AA.
]]
-- Mostly clientside now. We set our desired target goal and listen to the results rather than calculating ourselves.
local target = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).TargetGoal
GAMESTATE:GetPlayerState(PLAYER_1):SetTargetGoal(target / 100)

-- We can save space by wrapping the personal best and set percent trackers into one function, however
-- this would make the actor needlessly cumbersome and unnecessarily punish those who don't use the
-- personal best tracker (although everything is efficient enough now it probably wouldn't matter)

-- moved it for better manipulation
local d =
	Def.ActorFrame {
	Name = "TargetTracker",
	InitCommand = function(self)
		if (allowedCustomization) then
			Movable.DeviceButton_7.element = self
			Movable.DeviceButton_8.element = self
			Movable.DeviceButton_8.Border = self:GetChild("Border")
			Movable.DeviceButton_7.condition = enabledTargetTracker
			Movable.DeviceButton_8.condition = enabledTargetTracker
		end
		self:xy(MovableValues.TargetTrackerX, MovableValues.TargetTrackerY):zoom(MovableValues.TargetTrackerZoom)
	end,
	MovableBorder(100, 13, 1, 0, 0)
}

if targetTrackerMode == 0 then
	d[#d + 1] =
		LoadFont("Common Normal") ..
		{
			Name = "PercentDifferential",
			InitCommand = function(self)
				self:halign(0):valign(1)
				if allowedCustomization then
					self:settextf("%5.2f (%5.2f%%)", -100, 100)
					setBorderAlignment(self:GetParent():GetChild("Border"), 0, 1)
					setBorderToText(self:GetParent():GetChild("Border"), self)
				end
				self:settextf("")
			end,
			SpottedOffsetCommand = function(self)
				if tDiff >= 0 then
					diffuse(self, positive)
				else
					diffuse(self, negative)
				end
				self:settextf("%5.2f (%5.2f%%)", tDiff, target)
			end
		}
else
	d[#d + 1] =
		LoadFont("Common Normal") ..
		{
			Name = "PBDifferential",
			InitCommand = function(self)
				self:halign(0):valign(1)
				if allowedCustomization then
					self:settextf("%5.2f (%5.2f%%)", -100, 100)
					setBorderAlignment(self:GetParent():GetChild("Border"), 0, 1)
					setBorderToText(self:GetParent():GetChild("Border"), self)
				end
				self:settextf("")
			end,
			SpottedOffsetCommand = function(self, msg)
				if pbtarget then
					if tDiff >= 0 then
						diffuse(self, color("#00ff00"))
					else
						diffuse(self, negative)
					end
					self:settextf("%5.2f (%5.2f%%)", tDiff, pbtarget * 100)
				else
					if tDiff >= 0 then
						diffuse(self, positive)
					else
						diffuse(self, negative)
					end
					self:settextf("%5.2f (%5.2f%%)", tDiff, target)
				end
			end
		}
end

if enabledTargetTracker then
	t[#t + 1] = d
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 					    									**Display Percent**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Displays the current percent for the score.
]]
local cp =
	Def.ActorFrame {
	Name = "DisplayPercent",
	InitCommand = function(self)
		if (allowedCustomization) then
			Movable.DeviceButton_w.element = self
			Movable.DeviceButton_e.element = self
			Movable.DeviceButton_w.condition = enabledDisplayPercent
			Movable.DeviceButton_e.condition = enabledDisplayPercent
			Movable.DeviceButton_w.Border = self:GetChild("Border")
			Movable.DeviceButton_e.Border = self:GetChild("Border")
		end
		self:zoom(MovableValues.DisplayPercentZoom):x(MovableValues.DisplayPercentX):y(MovableValues.DisplayPercentY)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(60, 13):diffuse(color("0,0,0,0.4")):halign(1):valign(0)
		end
	},
	-- Displays your current percentage score
	LoadFont("Common Large") ..
		{
			Name = "DisplayPercent",
			InitCommand = function(self)
				self:zoom(0.3):halign(1):valign(0)
			end,
			OnCommand = function(self)
				if allowedCustomization then
					self:settextf("%05.2f%%", -10000)
					setBorderAlignment(self:GetParent():GetChild("Border"), 1, 0)
					setBorderToText(self:GetParent():GetChild("Border"), self)
				end
				self:settextf("%05.2f%%", 0)
			end,
			SpottedOffsetCommand = function(self)
				self:settextf("%05.2f%%", wifey)
			end
		},
	MovableBorder(100, 13, 1, 0, 0)
}

if enabledDisplayPercent then
	t[#t + 1] = cp
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
											    	**Player judgment counter (aka pa counter)**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Counts judgments.
--]]
-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local spacing = 10 -- Spacing between the judgetypes
local frameWidth = 60 -- Width of the Frame
local frameHeight = ((#jdgT + 1) * spacing) -- Height of the Frame
local judgeFontSize = 0.40 -- Font sizes for different text elements
local countFontSize = 0.35
local gradeFontSize = 0.45
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--

local j =
	Def.ActorFrame {
	Name = "JudgeCounter",
	InitCommand = function(self)
		if (allowedCustomization) then
			Movable.DeviceButton_p.element = self
			Movable.DeviceButton_p.condition = enabledJudgeCounter
		end
		self:xy(MovableValues.JudgeCounterX, MovableValues.JudgeCounterY)
	end,
	OnCommand = function(self)
		for i = 1, #jdgT do
			jdgCounts[jdgT[i]] = self:GetChild(jdgT[i])
		end
	end,
	SpottedOffsetCommand = function(self)
		if jdgCur and jdgCounts[jdgCur] ~= nil then
			settext(jdgCounts[jdgCur], jdgct)
		end
	end,
	Def.Quad {
		-- bg
		InitCommand = function(self)
			self:zoomto(frameWidth, frameHeight):diffuse(color("0,0,0,0.4"))
		end
	},
	MovableBorder(frameWidth, frameHeight, 1, 0, 0)
}

local function makeJudgeText(judge, index) -- Makes text
	return LoadFont("Common normal") ..
		{
			InitCommand = function(self)
				self:xy(-frameWidth / 2 + 5, -frameHeight / 2 + (index * spacing)):zoom(judgeFontSize):halign(0)
			end,
			OnCommand = function(self)
				settext(self, getShortJudgeStrings(judge))
				diffuse(self, jcT[judge])
			end
		}
end

local function makeJudgeCount(judge, index) -- Makes county things for taps....
	return LoadFont("Common Normal") ..
		{
			Name = judge,
			InitCommand = function(self)
				self:xy(frameWidth / 2 - 5, -frameHeight / 2 + (index * spacing)):zoom(countFontSize):halign(1):settext(0)
			end,
			PracticeModeResetMessageCommand= function(self)
				self:settext(0)
			end
		}
end

-- Build judgeboard
for i = 1, #jdgT do
	j[#j + 1] = makeJudgeText(jdgT[i], i)
	j[#j + 1] = makeJudgeCount(jdgT[i], i)
end

-- Now add the completed judgment table to the primary actor frame t if enabled
if enabledJudgeCounter then
	t[#t + 1] = j
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**Player ErrorBar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Visual display of deviance MovableValues.
--]]
-- User Parameters
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local barcount = 30 -- Number of bars. Older bars will refresh if judgments/barDuration exceeds this value. You don't need more than 40.
local barWidth = 2 -- Width of the ticks.
local barDuration = 0.75 -- Time duration in seconds before the ticks fade out. Doesn't need to be higher than 1. Maybe if you have 300 bars I guess.
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local currentbar = 1 -- so we know which error bar we need to update
local ingots = {} -- references to the error bars
local alpha = 0.07 -- ewma alpha
local avg
local lastAvg

-- Makes the error bars. They position themselves relative to the center of the screen based on your dv and diffuse to your judgement value before disappating or refreshing
-- Should eventually be handled by the game itself to optimize performance
function smeltErrorBar(index)
	return Def.Quad {
		Name = index,
		InitCommand = function(self)
			self:xy(MovableValues.ErrorBarX, MovableValues.ErrorBarY):zoomto(barWidth, MovableValues.ErrorBarHeight):diffusealpha(
				0
			)
		end,
		UpdateErrorBarCommand = function(self) -- probably a more efficient way to achieve this effect, should test stuff later
			finishtweening(self) -- note: it really looks like shit without the fade out
			diffusealpha(self, 1)
			diffuse(self, jcT[jdgCur])
			x(self, MovableValues.ErrorBarX + dvCur * wscale)
			y(self, MovableValues.ErrorBarY)
			Zoomtoheight(self, MovableValues.ErrorBarHeight)
			linear(self, barDuration)
			diffusealpha(self, 0)
		end,
		PracticeModeResetMessageCommand = function(self)
			diffusealpha(self, 0)
		end
	}
end

local e =
	Def.ActorFrame {
	Name = "ErrorBar",
	InitCommand = function(self)
		if (allowedCustomization) then
			Movable.DeviceButton_5.element = self:GetChildren()
			Movable.DeviceButton_6.element = self:GetChildren()
			Movable.DeviceButton_5.condition = enabledErrorBar ~= 0
			Movable.DeviceButton_6.condition = enabledErrorBar ~= 0
			Movable.DeviceButton_5.Border = self:GetChild("Border")
			Movable.DeviceButton_6.Border = self:GetChild("Border")
			Movable.DeviceButton_6.DeviceButton_left.arbitraryFunction = arbitraryErrorBarValue
			Movable.DeviceButton_6.DeviceButton_right.arbitraryFunction = arbitraryErrorBarValue
		end
		if enabledErrorBar == 1 then
			for i = 1, barcount do -- basically the equivalent of using GetChildren() if it returned unnamed children numerically indexed
				ingots[#ingots + 1] = self:GetChild(i)
			end
		else
			avg = 0
			lastAvg = 0
		end
	end,
	SpottedOffsetCommand = function(self)
		if enabledErrorBar == 1 then
			if dvCur ~= nil then
				currentbar = ((currentbar) % barcount) + 1
				queuecommand(ingots[currentbar], "UpdateErrorBar") -- Update the next bar in the queue
			end
		end
	end,
	DootCommand = function(self)
		self:RemoveChild("DestroyMe")
		self:RemoveChild("DestroyMe2")

		-- basically we need the ewma version to exist inside this actor frame
		-- for customize gameplay stuff, however it seems silly to have it running
		-- visibility/nil/type checks if we aren't using it, so we can just remove
		-- it if we're outside customize gameplay and errorbar is set to normal -mina
		if not allowedCustomization and enabledErrorBar == 1 then
			self:RemoveChild("WeightedBar")
		end
	end,
	Def.Quad {
		Name = "WeightedBar",
		InitCommand = function(self)
			if enabledErrorBar == 2 then
				self:xy(MovableValues.ErrorBarX, MovableValues.ErrorBarY):zoomto(barWidth, MovableValues.ErrorBarHeight):diffusealpha(
					1
				):diffuse(getMainColor("enabled"))
			else
				self:visible(false)
			end
		end,
		SpottedOffsetCommand = function(self)
			if enabledErrorBar == 2 and dvCur ~= nil then
				avg = alpha * dvCur + (1 - alpha) * lastAvg
				lastAvg = avg
				self:x(MovableValues.ErrorBarX + avg * wscale)
			end
		end
	},
	Def.Quad {
		Name = "Center",
		InitCommand = function(self)
			self:diffuse(getMainColor("highlight")):xy(MovableValues.ErrorBarX, MovableValues.ErrorBarY):zoomto(
				2,
				MovableValues.ErrorBarHeight
			)
		end
	},
	-- Indicates which side is which (early/late) These should be destroyed after the song starts.
	LoadFont("Common Normal") ..
		{
			Name = "DestroyMe",
			InitCommand = function(self)
				self:xy(MovableValues.ErrorBarX + errorBarFrameWidth / 4, MovableValues.ErrorBarY):zoom(0.35)
			end,
			BeginCommand = function(self)
				self:settext("Late"):diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0)
			end
		},
	LoadFont("Common Normal") ..
		{
			Name = "DestroyMe2",
			InitCommand = function(self)
				self:xy(MovableValues.ErrorBarX - errorBarFrameWidth / 4, MovableValues.ErrorBarY):zoom(0.35)
			end,
			BeginCommand = function(self)
				self:settext("Early"):diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0):queuecommand(
					"Doot"
				)
			end,
			DootCommand = function(self)
				self:GetParent():queuecommand("Doot")
			end
		},
	MovableBorder(
		MovableValues.ErrorBarWidth,
		MovableValues.ErrorBarHeight,
		1,
		MovableValues.ErrorBarX,
		MovableValues.ErrorBarY
	)
}

-- Initialize bars
if enabledErrorBar == 1 then
	for i = 1, barcount do
		e[#e + 1] = smeltErrorBar(i)
	end
end

-- Add the completed errorbar frame to the primary actor frame t if enabled
if enabledErrorBar ~= 0 then
	t[#t + 1] = e
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															   **Player Info**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Avatar and such, now you can turn it off. Planning to have player mods etc exported similarly to the nowplaying, and an avatar only option
]]
if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).PlayerInfo then
	t[#t + 1] = LoadActor("playerinfo")
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														       **Full Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither.
]]
-- User params
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local width = SCREEN_WIDTH / 2 - 100
local height = 10
local alpha = 0.7
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local replaySlider =
	isReplay and
	Widg.SliderBase {
		width = width,
		height = height,
		min = 0,
		visible = true,
		max = GAMESTATE:GetCurrentSong():MusicLengthSeconds(),
		onInit = function(slider)
			slider.actor:diffusealpha(0)
		end,
		-- Change to onValueChangeEnd if this
		-- lags too much
		onValueChange = function(val)
			SCREENMAN:GetTopScreen():SetReplayPosition(val)
		end
	} or
	Def.Actor {}
local p =
	Def.ActorFrame {
	Name = "FullProgressBar",
	InitCommand = function(self)
		self:xy(MovableValues.FullProgressBarX, MovableValues.FullProgressBarY)
		self:zoomto(MovableValues.FullProgressBarWidth, MovableValues.FullProgressBarHeight)
		if (allowedCustomization) then
			Movable.DeviceButton_9.element = self
			Movable.DeviceButton_0.element = self
			Movable.DeviceButton_9.condition = enabledFullBar
			Movable.DeviceButton_0.condition = enabledFullBar
		end
	end,
	replaySlider,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(width, height):diffuse(color("#666666")):diffusealpha(alpha)
		end
	},
	Def.SongMeterDisplay {
		InitCommand = function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth = width,
		Stream = Def.Quad {
			InitCommand = function(self)
				self:zoomy(height):diffuse(getMainColor("highlight"))
			end
		}
	},
	LoadFont("Common Normal") ..
		{
			-- title
			InitCommand = function(self)
				self:zoom(0.35):maxwidth(width * 2)
			end,
			BeginCommand = function(self)
				self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
			end,
			DoneLoadingNextSongMessageCommand = function(self)
				self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
			end
		},
	LoadFont("Common Normal") ..
		{
			-- total time
			InitCommand = function(self)
				self:x(width / 2):zoom(0.35):maxwidth(width * 2):halign(1)
			end,
			BeginCommand = function(self)
				local ttime = GetPlayableTime()
				settext(self, SecondsToMMSS(ttime))
				diffuse(self, byMusicLength(ttime))
			end,
			DoneLoadingNextSongMessageCommand = function(self)
				local ttime = GetPlayableTime()
				settext(self, SecondsToMMSS(ttime))
				diffuse(self, byMusicLength(ttime))
			end,
			--- ???? uhhh
			CurrentRateChangedMessageCommand = function(self)
				local ttime = GetPlayableTime()
				settext(self, SecondsToMMSS(ttime))
				diffuse(self, byMusicLength(ttime))
			end
		},
	MovableBorder(width, height, 1, 0, 0)
}

if enabledFullBar then
	t[#t + 1] = p
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														      **Mini Progressbar**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Song Completion Meter that doesn't eat 100 fps. Courtesy of simply love. Decided to make the full progress bar and mini progress bar
separate entities. So you can have both, or one or the other, or neither.
]]
local width = 34
local height = 4
local alpha = 0.3

local mb =
	Def.ActorFrame {
	Name = "MiniProgressBar",
	InitCommand = function(self)
		self:xy(MovableValues.MiniProgressBarX, MovableValues.MiniProgressBarY)
		if (allowedCustomization) then
			Movable.DeviceButton_q.element = self
			Movable.DeviceButton_q.condition = enabledMiniBar
			Movable.DeviceButton_q.Border = self:GetChild("Border")
		end
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(width, height):diffuse(color("#666666")):diffusealpha(alpha)
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:x(1 + width / 2):zoomto(1, height):diffuse(color("#555555"))
		end
	},
	Def.SongMeterDisplay {
		InitCommand = function(self)
			self:SetUpdateRate(0.5)
		end,
		StreamWidth = width,
		Stream = Def.Quad {
			InitCommand = function(self)
				self:zoomy(height):diffuse(getMainColor("highlight"))
			end
		}
	},
	MovableBorder(width, height, 1, 0, 0)
}

if enabledMiniBar then
	t[#t + 1] = mb
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**Music Rate Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
]]
t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(SCREEN_CENTER_X, SCREEN_BOTTOM - 10):zoom(0.35):settext(getCurRateDisplayString())
		end,
		SetRateCommand = function(self)
			self:settext(getCurRateDisplayString())
		end,
		DoneLoadingNextSongMessageCommand = function(self)
			self:playcommand("SetRate")
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:playcommand("SetRate")
		end
	}

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														    	**BPM Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	Better optimized frame update bpm display.
]]
local BPM
local a = GAMESTATE:GetPlayerState(PLAYER_1):GetSongPosition()
local r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
local GetBPS = SongPosition.GetCurBPS

local function UpdateBPM(self)
	local bpm = GetBPS(a) * r
	settext(BPM, Round(bpm, 2))
end

t[#t + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		BPM = self:GetChild("BPM")
		if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then -- dont bother updating for single bpm files
			self:SetUpdateFunction(UpdateBPM)
			self:SetUpdateRate(0.5)
		else
			BPM:settextf("%5.2f", GetBPS(a) * r) -- i wasn't thinking when i did this, we don't need to avoid formatting for performance because we only call this once -mina
		end
	end,
	LoadFont("Common Normal") ..
		{
			Name = "BPM",
			InitCommand = function(self)
				self:x(SCREEN_CENTER_X):y(SCREEN_BOTTOM - 20):halign(0.5):zoom(0.40)
			end
		},
	DoneLoadingNextSongMessageCommand = function(self)
		self:queuecommand("Init")
	end,
	-- basically a copy of the init
	CurrentRateChangedMessageCommand = function(self)
		r = GAMESTATE:GetSongOptionsObject("ModsLevel_Current"):MusicRate() * 60
		if #GAMESTATE:GetCurrentSong():GetTimingData():GetBPMs() > 1 then
			self:SetUpdateFunction(UpdateBPM)
			self:SetUpdateRate(0.5)
		else
			BPM:settextf("%5.2f", GetBPS(a) * r)
		end
	end
}

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															**Combo Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

]]
local x = 0
local y = 60

-- CUZ WIDESCREEN DEFAULTS SCREAAAAAAAAAAAAAAAAAAAAAAAAAM -mina
if IsUsingWideScreen() then
	y = y - WIDESCREENWHY
	x = x + WIDESCREENWHX
end

--This just initializes the initial point or not idk not needed to mess with this any more
function ComboTransformCommand(self, params)
	self:x(x)
	self:y(y)
end

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
														  **Judgment Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	moving here eventually
]]
--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															 **NPS Display**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	re-enabling the old nps calc/graph for now
]]
t[#t + 1] = LoadActor("npscalc")

--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															  **NPS graph**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	ditto
]]
--[[~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
															  **Practice Mode**
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	stuff
]]
local prevZoom = 0.65
local musicratio = 1

-- hurrrrr nps quadzapalooza -mina
local wodth = capWideScale(get43size(240), 280)
local hidth = 40
local cd
local bookmarkPosition

local function duminput(event)
	if event.DeviceInput.button == "DeviceButton_left mouse button" and event.type == "InputEventType_Release" then
		MESSAGEMAN:Broadcast("MouseLeftClick")
	elseif event.DeviceInput.button == "DeviceButton_right mouse button" and event.type == "InputEventType_Release" then
		MESSAGEMAN:Broadcast("MouseRightClick")
	elseif event.DeviceInput.button == "DeviceButton_backspace" and event.type == "InputEventType_FirstPress" then
		if bookmarkPosition ~= nil then
			SCREENMAN:GetTopScreen():SetPreviewNoteFieldMusicPosition(bookmarkPosition)
		end
	elseif event.button == "EffectUp" and event.type == "InputEventType_FirstPress" then
		SCREENMAN:GetTopScreen():AddToPracticeRate(0.05)
	elseif event.button == "EffectDown" and event.type == "InputEventType_FirstPress" then
		SCREENMAN:GetTopScreen():AddToPracticeRate(-0.05)
	end
	return false
end

local function UpdatePreviewPos(self)
	local pos = SCREENMAN:GetTopScreen():GetSongPosition() / musicratio
	self:GetChild("Pos"):zoomto(math.min(math.max(0, pos), wodth), hidth)
	self:queuecommand("Highlight")
end

local pm =
	Def.ActorFrame {
	Name = "ChartPreview",
	InitCommand = function(self)
		self:xy(MovableValues.PracticeCDGraphX, MovableValues.PracticeCDGraphY)
		self:SetUpdateFunction(UpdatePreviewPos)
		cd = self:GetChild("ChordDensityGraph"):visible(true):draworder(1000):y(20)
		if (allowedCustomization) then
			Movable.DeviceButton_z.element = self
			Movable.DeviceButton_z.condition = practiceMode
		--Movable.DeviceButton_z.Border = self:GetChild("Border")
		--Movable.DeviceButton_x.element = self
		--Movable.DeviceButton_x.condition = practiceMode
		--Movable.DeviceButton_x.Border = self:GetChild("Border")
		end
		--self:zoomto(MovableValues.PracticeCDGraphWidth, MovableValues.PracticeCDGraphHeight)
	end,
	BeginCommand = function(self)
		musicratio = GAMESTATE:GetCurrentSong():GetLastSecond() / (wodth)
		SCREENMAN:GetTopScreen():AddInputCallback(duminput)
		cd:GetChild("cdbg"):diffusealpha(0)
		self:SortByDrawOrder()
		self:queuecommand("GraphUpdate")
	end,
	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:x(wodth / 2)
			self:diffuse(color("0.05,0.05,0.05,1"))
		end
	},
	Def.Quad {
		Name = "PosBG",
		InitCommand = function(self)
			self:zoomto(wodth, hidth):halign(0):diffuse(color("1,1,1,1")):draworder(900)
		end,
		HighlightCommand = function(self) -- use the bg for detection but move the seek pointer -mina
			if isOver(self) then
				self:GetParent():GetChild("Seek"):visible(true)
				self:GetParent():GetChild("Seektext"):visible(true)
				self:GetParent():GetChild("Seek"):x(INPUTFILTER:GetMouseX() - self:GetParent():GetX())
				self:GetParent():GetChild("Seektext"):x(INPUTFILTER:GetMouseX() - self:GetParent():GetX() - 4) -- todo: refactor this lmao -mina
				self:GetParent():GetChild("Seektext"):y(INPUTFILTER:GetMouseY() - self:GetParent():GetY())
				self:GetParent():GetChild("Seektext"):settextf(
					"%0.2f",
					self:GetParent():GetChild("Seek"):GetX() * musicratio / getCurRateValue()
				)
			else
				self:GetParent():GetChild("Seektext"):visible(false)
				self:GetParent():GetChild("Seek"):visible(false)
			end
		end
	},
	Def.Quad {
		Name = "Pos",
		InitCommand = function(self)
			self:zoomto(0, hidth):diffuse(color("0,1,0,.5")):halign(0):draworder(900)
		end
	}
	--MovableBorder(wodth+3, hidth+3, 1, (wodth)/2, 0)
}

-- Load the CDGraph with a forced width parameter.
pm[#pm + 1] = LoadActorWithParams("../_chorddensitygraph.lua", {width = wodth})

-- more draw order shenanigans
pm[#pm + 1] =
	LoadFont("Common Normal") ..
	{
		Name = "Seektext",
		InitCommand = function(self)
			self:y(8):valign(1):halign(1):draworder(1100):diffuse(color("0.8,0,0")):zoom(0.4)
		end
	}

pm[#pm + 1] =
	Def.Quad {
	Name = "Seek",
	InitCommand = function(self)
		self:zoomto(2, hidth):diffuse(color("1,.2,.5,1")):halign(0.5):draworder(1100)
	end,
	MouseLeftClickMessageCommand = function(self)
		if isOver(self) then
			SCREENMAN:GetTopScreen():SetPreviewNoteFieldMusicPosition(self:GetX() * musicratio)
		end
	end,
	MouseRightClickMessageCommand = function(self)
		if isOver(self) then
			bookmarkPosition = self:GetX() * musicratio
			self:GetParent():GetChild("BookmarkPos"):queuecommand("Set")
		else
			if not (allowedCustomization) then
				SCREENMAN:GetTopScreen():TogglePracticePause()
			end
		end
	end
}

pm[#pm + 1] =
	Def.Quad {
	Name = "BookmarkPos",
	InitCommand = function(self)
		self:zoomto(2, hidth):diffuse(color(".2,.5,1,1")):halign(0.5):draworder(1100)
		self:visible(false)
	end,
	SetCommand = function(self)
		self:visible(true)
		self:x(bookmarkPosition / musicratio)
	end
}

if practiceMode and not isReplay then
	t[#t + 1] = pm
end

return t
