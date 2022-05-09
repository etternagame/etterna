-- A moving average NPS calculator

local enabledNPSDisplay = playerConfig:get_data().NPSDisplay
local enabledNPSGraph = playerConfig:get_data().NPSGraph

local countNotesSeparately = GAMESTATE:CountNotesSeparately()
-- Generally, a smaller window will adapt faster, but a larger window will have a more stable value.
local maxWindow = 1 -- this will be the maximum size of the "window" in seconds.
local minWindow = 1 -- this will be the minimum size of the "window" in seconds. Unused for now.

--Graph related stuff
local initialPeak = 10 -- Initial height of the NPS graph.
local graphWidth = GAMEPLAY:getItemWidth("npsGraph")
local graphHeight = GAMEPLAY:getItemHeight("npsGraph")
local npsDisplayTextSize = GAMEPLAY:getItemHeight("npsDisplayText")

local maxVerts = 100 -- Higher numbers allows for more detailed graph that spans for a longer duration. But may lead to performance issues
local graphFreq = 0.2 -- The frequency in which the graph updates in seconds.
--------------------

local graphColor = COLORS:getGameplayColor("NPSGraph")
local npsWindow = maxWindow

-- This table holds the timestamp of each judgment.
-- being considered for the moving average and the size of the chord.
-- (let's just call this notes for simplicity)
local noteTable = {}
local lastJudgment = "TapNoteScore_None"

-- Total sum of notes inside the moving average window.
-- The values are added/subtracted whenever we add/remove a note from the noteTable.
-- This allows us to get the total sum of notes that were hit without
-- iterating through the entire noteTable to get the sum.
local noteSum = 0
local peakNPS = 0

local translations = {
    Peak = THEME:GetString("ScreenGameplay", "NPSDisplayPeak"),
    NPS = THEME:GetString("ScreenGameplay", "NPSDisplayNPS"),
}

---------------
-- Functions --
---------------

-- This function will take the player, the timestamp,
-- and the size of the chord and append it to noteTable.
-- The function will also add the size of the chord to noteSum
-- This function is called whenever a JudgmentMessageCommand for regular tap note occurs.
-- (simply put, whenever you hit/miss a note)
local function addNote(time, size)
    if countNotesSeparately == true then
        size = 1
    end

    noteTable[#noteTable + 1] = {time, size}
    noteSum = noteSum + size
end

-- This function is called every frame to check if there are notes that
-- are old enough to remove from the table.
-- Every time it is called, the function will loop and remove all old notes
-- from noteTable and subtract the corresponding chord size from noteSum.
local function removeNote()
    local exit = false
    while not exit do
        if #noteTable >= 1 then
            if noteTable[1][1] + npsWindow < GetTimeSinceStart() then
                noteSum = noteSum - noteTable[1][2]
                table.remove(noteTable, 1)
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
local function getCurNPS()
    return noteSum / clamp(GAMESTATE:GetSongPosition():GetMusicSeconds(), minWindow, npsWindow)
end

-- This is an update function that is being called every frame while this is loaded.
local function Update(self)
    self.InitCommand = function(self)
        self:SetUpdateFunction(Update)
    end

    if enabledNPSDisplay or enabledNPSGraph then
        -- We want to constantly check for old notes to remove and update the NPS counter text.
        removeNote()

        curNPS = getCurNPS()

        -- Update peak nps. Only start updating after enough time has passed.
        if GAMESTATE:GetSongPosition():GetMusicSeconds() > npsWindow then
            peakNPS = math.max(peakNPS, curNPS)
        end
        -- the actor which called this update function passes itself down as "self".
        -- we then have "self" look for the child named "Text" which you can see down below.
        -- Then the settext function is called (or settextf for formatted ones) to set the text of the child "Text"
        -- every time this function is called.
        -- We don't display the decimal values due to lack of precision from having a relatively small time window.
        if enabledNPSDisplay then
            self:GetChild("NPSDisplay"):GetChild("Text"):settextf("%0.0f %s (%s %0.0f)", curNPS, translations["NPS"], translations["Peak"], peakNPS)
        end
    end
end

local function npsDisplay()
    local t = Def.ActorFrame {
        Name = "NPSDisplay",
        InitCommand = function(self)
            self:playcommand("SetUpMovableValues")
            registerActorToCustomizeGameplayUI({
                actor = self,
                coordInc = {5,1},
                zoomInc = {0.1,0.05},
            })
        end,
        SetUpMovableValuesMessageCommand = function(self)
            self:xy(MovableValues.NPSDisplayX, MovableValues.NPSDisplayY)
            self:zoom(MovableValues.NPSDisplayZoom)
        end,
        -- Whenever a MessageCommand is broadcasted,
        -- a table contanining parameters can also be passed along.
        JudgmentMessageCommand = function(self, params)
            local notes = params.Notes -- this is just one of those parameters.

            local chordsize = 0

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
                addNote(GetTimeSinceStart(), chordsize)
                lastJudgment = params.TapNoteScore
            end
        end,
    }
    -- the text that will be updated by the update function.
    if enabledNPSDisplay then
        t[#t + 1] = LoadFont("Common Normal") .. {
            Name = "Text",
            InitCommand = function(self)
                self:halign(0):valign(0)
                self:settext("0 NPS (Peak 0.0)")
                self:diffuse(COLORS:getGameplayColor("PrimaryText"))
                self:diffusealpha(1)
                self:zoom(npsDisplayTextSize)
            end,
        }
    end

    return t
end

local function npsGraph()
    local t = Def.ActorFrame {
        Name = "NPSGraph",
        InitCommand = function(self)
            self:playcommand("SetUpMovableValues")
            registerActorToCustomizeGameplayUI({
                actor = self,
                coordInc = {5,1},
                zoomInc = {0.1,0.05},
            })
        end,
        SetUpMovableValuesMessageCommand = function(self)
            self:xy(MovableValues.NPSGraphX, MovableValues.NPSGraphY)
            self:zoomto(MovableValues.NPSGraphWidth, MovableValues.NPSGraphHeight)
        end,
    }
    local verts = {
        {{0, 0, 0}, graphColor}
    }
    local total = 1
    local peakNPS = initialPeak
    local curNPS = 0
    t[#t + 1] = Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:zoomto(graphWidth, graphHeight)
            self:xy(0, graphHeight)
            self:diffuse(color("#333333")):diffusealpha(0.8)
            self:horizalign(0):vertalign(2)
            self:fadetop(1)
        end,
    }

    t[#t + 1] = Def.Quad {
        Name = "Floor",
        InitCommand = function(self)
            self:zoomto(graphWidth, 1)
            self:xy(0, graphHeight)
            self:diffusealpha(0.5)
            self:horizalign(0)
        end,
    }

    t[#t + 1] = Def.Quad {
        Name = "Ceiling",
        InitCommand = function(self)
            self:zoomto(graphWidth, 1)
            self:xy(0, 0)
            self:diffusealpha(0.2)
            self:horizalign(0)
        end,
    }

    t[#t + 1] = Def.ActorMultiVertex {
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
            curNPS = getCurNPS()
            curJudgment = lastJudgment

            if curNPS > peakNPS then -- update height if there's a new peak NPS value
                for i = 1, #verts do
                    verts[i][1][2] = verts[i][1][2] * (peakNPS / curNPS)
                end
                peakNPS = curNPS
            end

            verts[#verts + 1] = {{total * (graphWidth / maxVerts), -curNPS / peakNPS * graphHeight, 0}, graphColor}
            if #verts > maxVerts + 2 then -- Start removing unused verts. Otherwise RIP lag
                table.remove(verts, 1)
            end
            self:SetVertices(verts)
            self:addx(-graphWidth / maxVerts)
            self:SetDrawState {First = math.max(1, #verts - maxVerts), Num = math.min(maxVerts, #verts)}
            self:sleep(graphFreq)
            self:queuecommand("GraphUpdate")
        end,
    }
    return t
end

local t = Def.ActorFrame {
    Name = "NPSContainer",
    BeginCommand = function(self)
        if enabledNPSDisplay or enabledNPSGraph then
            self:SetUpdateFunction(Update)
        end
    end,
}

if enabledNPSDisplay then
    t[#t + 1] = npsDisplay()
end
if enabledNPSGraph then
    if not enabledNPSDisplay then
        t[#t + 1] = npsDisplay()
    end
    t[#t + 1] = npsGraph()
end

return t
