-- A moving average NPS calculator

-- movable stuff
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
--still kept this here because idk man
local enabled = {
	NPSDisplay = {
		PlayerNumber_P1 = GAMESTATE:IsPlayerEnabled(PLAYER_1) and
			playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).NPSDisplay
	},
	NPSGraph = {
		PlayerNumber_P1 = GAMESTATE:IsPlayerEnabled(PLAYER_1) and playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).NPSGraph
	}
}

local debug = false
local countNotesSeparately = GAMESTATE:CountNotesSeparately()
-- Generally, a smaller window will adapt faster, but a larger window will have a more stable value.
local maxWindow = themeConfig:get_data().NPSDisplay.MaxWindow / 2 -- this will be the maximum size of the "window" in seconds.
local minWindow = themeConfig:get_data().NPSDisplay.MinWindow / 2 -- this will be the minimum size of the "window" in seconds. Unused for now.
local dynamicWindow = false -- set to false for now.

isCentered = PREFSMAN:GetPreference("Center1Player")
local CenterX = SCREEN_CENTER_X
local mpOffset = 0
if not isCentered then
	CenterX =
		THEME:GetMetric(
		"ScreenGameplay",
		string.format("PlayerP1%sX", ToEnumShortString(GAMESTATE:GetCurrentStyle():GetStyleType()))
	)
	mpOffset = SCREEN_CENTER_X + 60
end

--Graph related stuff
local initialPeak = 10 -- Initial height of the NPS graph.
local graphWidth = 140
local graphHeight = 100
local graphPos = {
	-- Position of the NPS graph
	PlayerNumber_P1 = {
		X = 0 + mpOffset,
		Y = SCREEN_BOTTOM - 160
	},
	PlayerNumber_P2 = {
		X = SCREEN_WIDTH - graphWidth,
		Y = 50
	}
}

local textPos = {
	-- Position of the NPS text
	PlayerNumber_P1 = {
		X = 5 + mpOffset,
		Y = SCREEN_BOTTOM - 170
	},
	PlayerNumber_P2 = {
		X = SCREEN_WIDTH - graphWidth,
		Y = 34
	}
}

local maxVerts = 100 -- Higher numbers allows for more detailed graph that spans for a longer duration. But may lead to performance issues
local graphFreq = 0.2 -- The frequency in which the graph updates in seconds.
local lifeGraph = false -- SHow lifegraph
--------------------

--These should be moved to 02 colors.lua
local judgeColor = {
	-- Colors of each Judgment types
	TapNoteScore_W1 = color("#99ccff"),
	TapNoteScore_W2 = HSV(48, 0.8, 0.95),
	TapNoteScore_W3 = HSV(160, 0.9, 0.8),
	TapNoteScore_W4 = HSV(200, 0.9, 1),
	TapNoteScore_W5 = HSV(320, 0.9, 1),
	TapNoteScore_Miss = HSV(0, 0.8, 0.8),
	HoldNoteScore_Held = HSV(48, 0.8, 0.95),
	HoldNoteScore_LetGo = HSV(0, 0.8, 0.8),
	TapNoteScore_None = Color.White
}

local npsWindow = {
	PlayerNumber_P1 = maxWindow,
	PlayerNumber_P2 = maxWindow
}

-- This table holds the timestamp of each judgment for each player.
-- being considered for the moving average and the size of the chord.
-- (let's just call this notes for simplicity)
local noteTable = {
	PlayerNumber_P1 = {},
	PlayerNumber_P2 = {}
}

local lastJudgment = {
	PlayerNumber_P1 = "TapNoteScore_None",
	PlayerNumber_P2 = "TapNoteScore_None"
}

-- Total sum of notes inside the moving average window for each player.
-- The values are added/subtracted whenever we add/remove a note from the noteTable.
-- This allows us to get the total sum of notes that were hit without
-- iterating through the entire noteTable to get the sum.
local noteSum = {
	PlayerNumber_P1 = 0,
	PlayerNumber_P2 = 0
}

local peakNPS = {
	PlayerNumber_P1 = 0,
	PlayerNumber_P2 = 0
}

local translated_info = {
	Peak = THEME:GetString("ScreenGameplay", "NPSGraphPeakNPS"),
	NPS = THEME:GetString("ScreenGameplay", "NPSGraphNPS")
}

---------------
-- Functions --
---------------

-- This function will take the player, the timestamp,
-- and the size of the chord and append it to noteTable.
-- The function will also add the size of the chord to noteSum
-- This function is called whenever a JudgmentMessageCommand for regular tap note occurs.
-- (simply put, whenever you hit/miss a note)
local function addNote(pn, time, size)
	if countNotesSeparately == true then
		size = 1
	end

	noteTable[pn][#noteTable[pn] + 1] = {time, size}
	noteSum[pn] = noteSum[pn] + size
end

-- This function is called every frame to check if there are notes that
-- are old enough to remove from the table.
-- Every time it is called, the function will loop and remove all old notes
-- from noteTable and subtract the corresponding chord size from noteSum.
local function removeNote(pn)
	local exit = false
	while not exit do
		if #noteTable[pn] >= 1 then
			if noteTable[pn][1][1] + npsWindow[pn] < GetTimeSinceStart() then
				noteSum[pn] = noteSum[pn] - noteTable[pn][1][2]
				table.remove(noteTable[pn], 1)
			else
				exit = true
			end
		else
			exit = true
		end
	end
end

-- The function simply Calculates the moving average NPS
-- Generally this is just nps = noteSum/window.
local function getCurNPS(pn)
	return noteSum[pn] / clamp(GAMESTATE:GetSongPosition():GetMusicSeconds(), minWindow, npsWindow[pn])
end

-- This is an update function that is being called every frame while this is loaded.
local function Update(self)
	self.InitCommand = function(self)
		self:SetUpdateFunction(Update)
	end

	for _, pn in pairs(GAMESTATE:GetEnabledPlayers()) do
		if enabled.NPSDisplay[pn] or enabled.NPSGraph[pn] then
			-- We want to constantly check for old notes to remove and update the NPS counter text.
			removeNote(pn)

			curNPS = getCurNPS(pn)

			-- Update peak nps. Only start updating after enough time has passed.
			if GAMESTATE:GetSongPosition():GetMusicSeconds() > npsWindow[pn] then
				peakNPS[pn] = math.max(peakNPS[pn], curNPS)
			end
			-- the actor which called this update function passes itself down as "self".
			-- we then have "self" look for the child named "Text" which you can see down below.
			-- Then the settext function is called (or settextf for formatted ones) to set the text of the child "Text"
			-- every time this function is called.
			-- We don't display the decimal values due to lack of precision from having a relatively small time window.
			if enabled.NPSDisplay[pn] then
				if debug then
					self:GetChild("NPSDisplay"):GetChild("Text"):settextf(
						"%0.1f NPS (Peak %0.1f)\n%0.1fs Window\n%d notes in table\nDynamic Window:%s",
						curNPS,
						peakNPS[pn],
						npsWindow[pn],
						noteSum[pn],
						tostring(dynamicWindow)
					)
				else
					self:GetChild("NPSDisplay"):GetChild("Text"):settextf("%0.0f %s (%s %0.0f)", curNPS, translated_info["NPS"], translated_info["Peak"], peakNPS[pn])
				end
			end
			-- update the window size.
			-- This isn't needed at all but it helps the counter
			-- adapt quickly to high-nps bursts.
			if dynamicWindow then
				npsWindow[pn] = clamp(15 / math.sqrt(getCurNPS(pn)), 1, maxWindow)
			end
		end
	end
end

local function npsDisplay(pn)
	local t =
		Def.ActorFrame {
		Name = "NPSDisplay",
		InitCommand = function(self)
			self:xy(MovableValues.NPSDisplayX, MovableValues.NPSDisplayY):zoom(MovableValues.NPSDisplayZoom)
			if allowedCustomization then
				Movable.DeviceButton_y.element = self
				Movable.DeviceButton_u.element = self
				Movable.DeviceButton_y.condition = enabled.NPSDisplay.PlayerNumber_P1
				Movable.DeviceButton_u.condition = enabled.NPSDisplay.PlayerNumber_P1
				Movable.DeviceButton_u.Border = self:GetChild("Border")
			end
		end,
		OnCommand = function(self)
			if allowedCustomization then
				setBorderAlignment(self:GetChild("Border"), 0, 0)
				setBorderToText(self:GetChild("Border"), self:GetChild("Text"))
			end
		end,
		-- Whenever a MessageCommand is broadcasted,
		-- a table contanining parameters can also be passed along.
		JudgmentMessageCommand = function(self, params)
			local notes = params.Notes -- this is just one of those parameters.

			local chordsize = 0

			if params.Player == pn then
				if params.Type == "Tap" then
					-- The notes parameter contains a table where the table indices
					-- correspond to the columns in game.
					-- The items in the table either contains a TapNote object (if there is a note)
					-- or be simply nil (if there are no notes)

					-- Since we only want to count the number of notes in a chord,
					-- we just iterate over the table and count the ones that aren't nil.
					-- Set chordsize to 1 if notes are counted separately.
					if GAMESTATE:GetCurrentGame():CountNotesSeparately() then
						chordsize = 1
					else
						for i = 1, GAMESTATE:GetCurrentStyle():ColumnsPerPlayer() do
							if notes ~= nil and notes[i] ~= nil then
								chordsize = chordsize + 1
							end
						end
					end

					-- add the note to noteTable
					addNote(pn, GetTimeSinceStart(), chordsize)
					lastJudgment[pn] = params.TapNoteScore
				end
			end
		end,
		MovableBorder(100, 200, 1, 0, 0),
	}
	-- the text that will be updated by the update function.
	if enabled.NPSDisplay[pn] then
		t[#t + 1] =
			LoadFont("Common Normal") ..
			{
				Name = "Text", -- sets the name of this actor as "Text". this is a child of the actor "t".
				InitCommand = function(self)
					self:halign(0):valign(0):settext("0 NPS (Peak 0.0)")
				end
			}
	end

	return t
end

local function PLife(pn)
	return STATSMAN:GetCurStageStats():GetPlayerStageStats(pn):GetCurrentLife() or 0
end

local function npsGraph(pn)
	local t =
		Def.ActorFrame {
		Name = "NPSGraph",
		InitCommand = function(self)
			self:xy(MovableValues.NPSGraphX, MovableValues.NPSGraphY):zoomtoheight(MovableValues.NPSGraphHeight):zoomtowidth(MovableValues.NPSGraphWidth)
			Movable.DeviceButton_i.element = self
			Movable.DeviceButton_o.element = self
			if allowedCustomization then
				Movable.DeviceButton_i.element = self
				Movable.DeviceButton_o.element = self
				Movable.DeviceButton_i.condition = enabled.NPSGraph.PlayerNumber_P1
				Movable.DeviceButton_o.condition = enabled.NPSGraph.PlayerNumber_P1
				setBorderAlignment(self:GetChild("Border"), 0, 0)
			end
		end,
	}
	local verts = {
		{{0, 0, 0}, Color.White}
	}
	local lifeverts = {
		{{0, 0, 0}, color("#00000000")}
	}
	local total = 1
	local peakNPS = initialPeak
	local curNPS = 0
	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:zoomto(graphWidth, graphHeight)
			self:xy(0, graphHeight)
			self:diffuse(color("#333333")):diffusealpha(0.8)
			self:horizalign(0):vertalign(2)
			self:fadetop(1)
		end
	}

	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:zoomto(graphWidth, 1)
			self:xy(0, graphHeight)
			self:diffusealpha(0.5)
			self:horizalign(0)
		end
	}

	t[#t + 1] =
		Def.Quad {
		InitCommand = function(self)
			self:zoomto(graphWidth, 1)
			self:xy(0, 0)
			self:diffusealpha(0.2)
			self:horizalign(0)
		end
	}

	t[#t + 1] =
		Def.ActorMultiVertex {
		Name = "AMV_QuadStrip",
		InitCommand = function(self)
			self:visible(true)
			self:xy(graphWidth, graphHeight)
			self:SetDrawState {Mode = "DrawMode_LineStrip"}
		end,
		BeginCommand = function(self)
			self:SetDrawState {First = 1, Num = -1}
			self:SetVertices(verts)
			self:queuecommand("GraphUpdate")
		end,
		GraphUpdateCommand = function(self)
			total = total + 1
			curNPS = getCurNPS(pn)
			curJudgment = lastJudgment[pn]

			if curNPS > peakNPS then -- update height if there's a new peak NPS value
				for i = 1, #verts do
					verts[i][1][2] = verts[i][1][2] * (peakNPS / curNPS)
				end
				peakNPS = curNPS
			end

			verts[#verts + 1] = {{total * (graphWidth / maxVerts), -curNPS / peakNPS * graphHeight, 0}, Color.White}
			if #verts > maxVerts + 2 then -- Start removing unused verts. Otherwise RIP lag
				table.remove(verts, 1)
			end
			self:SetVertices(verts)
			self:addx(-graphWidth / maxVerts)
			self:SetDrawState {First = math.max(1, #verts - maxVerts), Num = math.min(maxVerts, #verts)}
			self:sleep(graphFreq)
			self:queuecommand("GraphUpdate")
		end
	}
	t[#t + 1] = MovableBorder(graphWidth, graphHeight, 1, 0, 0)
	return t
end

local t =
	Def.ActorFrame {
	OnCommand = function(self)
		if enabled.NPSDisplay[PLAYER_1] or enabled.NPSDisplay[PLAYER_2] or enabled.NPSGraph[PLAYER_1] or
				enabled.NPSGraph[PLAYER_2]
		 then
			self:SetUpdateFunction(Update)
		end
	end
}

for _, pn in pairs({PLAYER_1, PLAYER_2}) do
	if enabled.NPSDisplay[pn] then
		t[#t + 1] = npsDisplay(pn)
	end
	if enabled.NPSGraph[pn] then
		if not enabled.NPSDisplay[pn] then
			t[#t + 1] = npsDisplay(pn)
		end
		t[#t + 1] = npsGraph(pn)
	end
end

return t
