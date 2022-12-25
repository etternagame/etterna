local sizing = Var("sizing") -- specify init sizing
if sizing == nil then sizing = {} end
local extraFeatures = Var("extraFeatures") -- toggle offset hovering and input events for highlighting
if extraFeatures == nil then extraFeatures = false end
--[[
    We are expecting the sizing table to be provided on file load.
    It should contain these attributes:
    Width
    Height
]]
-- all elements are placed relative to default valign - 0 halign
-- this means relatively to center vertically and relative to the left end horizontally

local translations = {
    StandardDeviation = THEME:GetString("OffsetPlot", "StandardDeviation"),
    Mean = THEME:GetString("OffsetPlot", "Mean"),
    Time = THEME:GetString("OffsetPlot", "Time"),
    Milliseconds = THEME:GetString("OffsetPlot", "Milliseconds"),
    Late = THEME:GetString("OffsetPlot", "Late"),
    Early = THEME:GetString("OffsetPlot", "Early"),
    Instructions = THEME:GetString("OffsetPlot", "Instructions"),
    CurrentColumnHighlights = THEME:GetString("OffsetPlot", "CurrentColumnHighlights"),
    OffsetWarning = THEME:GetString("OffsetPlot", "UsingReprioritized"),
}

local judgeSetting = (PREFSMAN:GetPreference("SortBySSRNormPercent") and 4 or GetTimingDifficulty())
local timingScale = ms.JudgeScalers[judgeSetting]
local usingCustomWindows = false

-- cap the graph to this
local maxOffset = 180
local lineThickness = 2
local lineAlpha = 0.2
local textPadding = 5
local textSize = Var("textsize") or 0.65
local instructionTextSize = 0.55

local dotAnimationSeconds = 1
local resizeAnimationSeconds = 0.1
local unHighlightedAlpha = 0.2

-- the dot sizes
-- the "classic" default is 1.0
local dotLineLength = 0.75
local dotLineUpperBound = 1.2
local dotLineLowerBound = 0.7
-- length of the dot lines for the mine X
local mineXSize = 3
local mineXThickness = 1

-- judgment windows to display on the plot
local barJudgments = {
    "TapNoteScore_W2",
    "TapNoteScore_W3",
    "TapNoteScore_W4",
    "TapNoteScore_W5",
}

-- tracking the index of dot highlighting for each column
local highlightIndex = 1
-- each index corresponds to a type of column setup to highlight
local highlightTable = {}

local function columnIsHighlighted(column)
    return column == nil or #highlightTable == 0 or highlightTable[highlightIndex][column] == true
end

-- allow moving the highlightIndex in either direction and loop around if under/overflow
local function moveHighlightIndex(direction)
    local newg = (((highlightIndex) + direction) % (#highlightTable + 1))
    if newg == 0 then
        newg = direction > 0 and 1 or #highlightTable
    end
    highlightIndex = newg
end

local function getMineColor(column)
    local mineColor = COLORS:getColor("offsetPlot", "MineHit")
    -- cant highlight or currently set to highlight this column
    if columnIsHighlighted(column) then
        return mineColor
    else
        -- not highlighting this column
        local c = {}
        for i,v in ipairs(mineColor) do
            c[i] = v
        end
        c[4] = unHighlightedAlpha
        return c
    end
end

local function getHoldColor(column, type)
    local color = color("#FFFFFF")
    if type == "TapNoteSubType_Roll" then
        color = COLORS:getColor("offsetPlot", "RollDrop")
    elseif type == "TapNoteSubType_Hold" then
        color = COLORS:getColor("offsetPlot", "HoldDrop")
    end

    -- cant highlight or currently set to highlight this column
    if columnIsHighlighted(column) then
        return color
    else
        -- not highlighting this column
        local c = {}
        for i,v in ipairs(color) do
            c[i] = v
        end
        c[4] = unHighlightedAlpha
        return c
    end
end

-- produces the highlightTable in the format:
-- { {x,y,z...}, {x,y,z...} ... } where each subtable is a list of columns to highlight, the keys are the columns
local function calcDotHighlightTable(tracks, columns)
    local out = {}
    if tracks ~= nil and #tracks ~= 0 then
        -- all columns
        out = {{}}
        for i = 1, columns do
            out[1][i] = true
        end

        if columns % 2 == 0 then
            out[#out+1] = {}
            out[#out+1] = {}
            -- even columns, 1 per hand
            for i = 1, columns / 2 do
                out[2][i] = true
            end
            for i = columns / 2 + 1, columns do
                out[3][i] = true
            end

        else
            out[#out+1] = {}
            out[#out+1] = {}
            out[#out+1] = {}
            -- odd columns, 1 left - 1 middle - 1 right
            for i = 1, math.floor(columns / 2) do
                out[2][i] = true
            end
            out[3][math.ceil(columns / 2)] = true
            for i = math.ceil(columns / 2) + 1, columns do
                out[4][i] = true
            end
        end
        -- add single highlights for each column
        for i = 1, columns do
            out[#out+1] = {[i] = true}
        end
    end
    return out
end

-- convert number to another number out of a given width
-- relative to left side of the graph
local function fitX(x, maxX)
    -- dont let the x go way off the end of the graph
    x = clamp(x, x, maxX)
    return x / maxX * sizing.Width
end

-- convert millisecond values to a y position in the graph
-- relative to vertical center
local function fitY(y, maxY)
    return -1 * y / maxY * sizing.Height / 2 + sizing.Height / 2
end

-- 4 xyz coordinates are given to make up the 4 corners of a quad to draw
local function placeDotVertices(vertList, x, y, color)
    vertList[#vertList + 1] = {{x - dotLineLength, y + dotLineLength, 0}, color}
    vertList[#vertList + 1] = {{x + dotLineLength, y + dotLineLength, 0}, color}
    vertList[#vertList + 1] = {{x + dotLineLength, y - dotLineLength, 0}, color}
    vertList[#vertList + 1] = {{x - dotLineLength, y - dotLineLength, 0}, color}
end

-- 2 pairs of 4 coordinates to draw a big X
local function placeMineVertices(vertList, x, y, color)
    vertList[#vertList + 1] = {{x - mineXSize - mineXThickness / 2, y - mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x + mineXSize - mineXThickness / 2, y + mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x - mineXSize + mineXThickness / 2, y - mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x + mineXSize + mineXThickness / 2, y + mineXSize, 0}, color}

    vertList[#vertList + 1] = {{x + mineXSize + mineXThickness / 2, y - mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x - mineXSize + mineXThickness / 2, y + mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x + mineXSize - mineXThickness / 2, y - mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x - mineXSize - mineXThickness / 2, y + mineXSize, 0}, color}
end

-- 2 pairs of 4 coordinates to draw a ^
local function placeNoodleVertices(vertList, x, y, color)
    vertList[#vertList + 1] = {{x - mineXThickness / 2, y + mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x + mineXThickness / 2, y, 0}, color}
    vertList[#vertList + 1] = {{x + mineXThickness / 2, y + mineXSize, 0}, color}
    vertList[#vertList + 1] = {{x - mineXThickness / 2, y, 0}, color}
end

local t = Def.ActorFrame {
    Name = "OffsetPlotFile",
    InitCommand = function(self)
        local hid = false
        if not extraFeatures then return end -- no extra features: dont add the hover
        self:SetUpdateFunction(function()
            local bg = self:GetChild("BG")
            if isOver(bg) then
                local top = SCREENMAN:GetTopScreen()
                -- dont break if it will break (we can only do this from the eval screen)
                if not top.RescoreReplay then
                    return
                end

                TOOLTIP:Show()

                local x, y = bg:GetLocalMousePos(INPUTFILTER:GetMouseX(), INPUTFILTER:GetMouseY(), 0)
                local percent = clamp(x / bg:GetZoomedWidth(), 0, 1)
                -- 48 rows per beat, multiply the current beat by 48 to get the current row
                local td = GAMESTATE:GetCurrentSteps():GetTimingData()
                local lastsec = GAMESTATE:GetCurrentSteps():GetLastSecond()
                local row = td:GetBeatFromElapsedTime(percent * lastsec) * 48

                local replay = REPLAYS:GetActiveReplay()
                local snapshot = replay:GetReplaySnapshotForNoterow(row)
                local judgments = snapshot:GetJudgments()
                local wifescore = snapshot:GetWifePercent() * 100
                local time = SecondsToHHMMSS(td:GetElapsedTimeFromNoteRow(row))
                local mean = snapshot:GetMean()
                local sd = snapshot:GetStandardDeviation()

                local marvCount = judgments["W1"]
                local perfCount = judgments["W2"]
                local greatCount = judgments["W3"]
                local goodCount = judgments["W4"]
                local badCount = judgments["W5"]
                local missCount = judgments["Miss"]

                -- excessively long string format for translation support
                local txt = string.format(
                    "%5.6f%%\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %d\n%s: %0.2f%s\n%s: %0.2f%s\n%s: %s",
                    wifescore,
                    getJudgeStrings("TapNoteScore_W1"), marvCount,
                    getJudgeStrings("TapNoteScore_W2"), perfCount,
                    getJudgeStrings("TapNoteScore_W3"), greatCount,
                    getJudgeStrings("TapNoteScore_W4"), goodCount,
                    getJudgeStrings("TapNoteScore_W5"), badCount,
                    getJudgeStrings("TapNoteScore_Miss"), missCount,
                    translations["StandardDeviation"], sd, translations["Milliseconds"],
                    translations["Mean"], mean, translations["Milliseconds"],
                    translations["Time"], time
                )

                local mp = self:GetChild("MousePosition")
                mp:visible(true)
                mp:x(x)
                TOOLTIP:SetText(txt)
                hid = false
            else
                if not hid then
                    self:GetChild("MousePosition"):visible(false)
                    TOOLTIP:Hide()
                    hid = true
                end
            end
        end)
    end,
    BeginCommand = function(self)
        if not extraFeatures then return end -- no extra features: dont add the input highlight
        SCREENMAN:GetTopScreen():AddInputCallback(function(event)
            if #highlightTable ~= 0 then
                if event.type == "InputEventType_FirstPress" then
                    if event.button == "MenuDown" or event.button == "Down" then
                        moveHighlightIndex(1)
                        self:playcommand("DrawOffsets")
                        self:hurrytweening(0.2)
                    elseif event.button == "MenuUp" or event.button == "Up" then
                        moveHighlightIndex(-1)
                        self:playcommand("DrawOffsets")
                        self:hurrytweening(0.2)
                    end
                end
            end
        end)
    end,
    UpdateSizingCommand = function(self, params)
        if params.sizing ~= nil then
            sizing = params.sizing
        end
        if params.judgeSetting ~= nil then
            judgeSetting = params.judgeSetting
            timingScale = ms.JudgeScalers[judgeSetting]
        end
    end
}

t[#t+1] = Def.Quad {
    Name = "BG",
    InitCommand = function(self)
        self:halign(0)
        self:diffusealpha(1)
        registerActorToColorConfigElement(self, "offsetPlot", "Background")
        self:playcommand("UpdateSizing")
        self:finishtweening()
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:y(sizing.Height / 2)
        self:zoomto(sizing.Width, sizing.Height)
    end
}

if extraFeatures then
    t[#t+1] = Def.Quad {
        Name = "MousePosition",
        InitCommand = function(self)
            self:valign(0)
            self:diffusealpha(1)
            registerActorToColorConfigElement(self, "offsetPlot", "HoverLine")
            self:zoomx(lineThickness)
            self:playcommand("UpdateSizing")
            self:finishtweening()
        end,
        UpdateSizingCommand = function(self)
            self:finishtweening()
            self:smooth(resizeAnimationSeconds)
            self:zoomy(sizing.Height)
        end
    }
end

t[#t+1] = Def.Quad {
    Name = "CenterLine",
    InitCommand = function(self)
        self:halign(0)
        self:diffusealpha(lineAlpha)
        registerActorToColorConfigElement(self, "judgment", "TapNoteScore_W1")
        self:playcommand("UpdateSizing")
        self:finishtweening()
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:y(sizing.Height / 2)
        self:zoomto(sizing.Width, lineThickness)
    end
}

for i, j in ipairs(barJudgments) do
    t[#t+1] = Def.Quad {
        Name = j.."_Late",
        InitCommand = function(self)
            self:halign(0)
            self:diffusealpha(lineAlpha)
            registerActorToColorConfigElement(self, "judgment", j)
            self:playcommand("UpdateSizing")
            self:finishtweening()
        end,
        UpdateSizingCommand = function(self)
            self:finishtweening()
            self:smooth(resizeAnimationSeconds)
            local window = ms.getLowerWindowForJudgment(j, timingScale)
            if usingCustomWindows then window = getCustomWindowConfigJudgmentWindowLowerBound(j) end
            self:y(fitY(window, maxOffset))
            self:zoomto(sizing.Width, lineThickness)
        end
    }
    t[#t+1] = Def.Quad {
        Name = j.."_Early",
        InitCommand = function(self)
            self:halign(0)
            self:diffusealpha(lineAlpha)
            registerActorToColorConfigElement(self, "judgment", j)
            self:playcommand("UpdateSizing")
            self:finishtweening()
        end,
        UpdateSizingCommand = function(self)
            self:finishtweening()
            self:smooth(resizeAnimationSeconds)
            local window = ms.getLowerWindowForJudgment(j, timingScale)
            if usingCustomWindows then window = getCustomWindowConfigJudgmentWindowLowerBound(j) end
            self:y(fitY(-window, maxOffset))
            self:zoomto(sizing.Width, lineThickness)
        end
    }
end

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "LateText",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:zoom(textSize)
        registerActorToColorConfigElement(self, "offsetPlot", "Text")
        self:playcommand("UpdateSizing")
        self:finishtweening()
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        local bound = ms.getUpperWindowForJudgment(barJudgments[#barJudgments], timingScale)
        self:xy(textPadding, textPadding)
        self:settextf("%s (+%d%s)", translations["Late"], bound, translations["Milliseconds"])
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "EarlyText",
    InitCommand = function(self)
        self:halign(0):valign(1)
        self:zoom(textSize)
        registerActorToColorConfigElement(self, "offsetPlot", "Text")
        self:playcommand("UpdateSizing")
        self:finishtweening()
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        local bound = ms.getUpperWindowForJudgment(barJudgments[#barJudgments], timingScale)
        self:xy(textPadding, sizing.Height - textPadding)
        self:settextf("%s (-%d%s)", translations["Early"], bound, translations["Milliseconds"])
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "InstructionText",
    InitCommand = function(self)
        self:valign(1)
        self:zoom(instructionTextSize)
        self:settext("")
        registerActorToColorConfigElement(self, "offsetPlot", "Text")
        self:playcommand("UpdateSizing")
        self:finishtweening()
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:xy(sizing.Width / 2, sizing.Height - textPadding)
        self:maxwidth((sizing.Width - self:GetParent():GetChild("EarlyText"):GetZoomedWidth() * 2) / instructionTextSize - textPadding)
    end,
    UpdateTextCommand = function(self)
        local cols = {}
        if #highlightTable == 0 or highlightTable[highlightIndex] == nil or not extraFeatures then
            self:settext("")
        else
            for col, _ in pairs(highlightTable[highlightIndex]) do
                cols[#cols+1] = col
                cols[#cols+1] = " "
            end
            cols[#cols] = nil
            cols = table.concat(cols)
            self:settextf("%s (%s: %s)", translations["Instructions"], translations["CurrentColumnHighlights"], cols)
        end
    end
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "OffsetWarningText",
    InitCommand = function(self)
        self:valign(0)
        self:zoom(instructionTextSize)
        self:settext(translations["OffsetWarning"])
        registerActorToColorConfigElement(self, "offsetPlot", "Text")
        self:playcommand("UpdateSizing")
        self:finishtweening()
    end,
    UpdateSizingCommand = function(self)
        self:visible(usingCustomWindows and currentCustomWindowConfigUsesOldestNoteFirst())

        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:xy(sizing.Width / 2, textPadding)
        self:maxwidth((sizing.Width - self:GetParent():GetChild("LateText"):GetZoomedWidth() * 2) / instructionTextSize - textPadding)
    end,
}

-- keeping track of stuff for persistence dont look at this
local lastOffsets = {}
local lastTracks = {}
local lastTiming = {}
local lastTimingData = nil
local lastTypes = {}
local lastHolds = {}
local lastMaxTime = 0
local lastColumns = nil

t[#t+1] = Def.ActorMultiVertex {
    Name = "Dots",
    InitCommand = function(self)
        --self:zoomto(0, 0)
        self:playcommand("UpdateSizing")
    end,
    UpdateSizingCommand = function(self)

    end,
    ColorConfigUpdatedMessageCommand = function(self)
        self:finishtweening()
        self:linear(0.5)
        self:queuecommand("DrawOffsets")
    end,
    LoadOffsetsCommand = function(self, params)
        usingCustomWindows = params.usingCustomWindows or false

        -- makes sure all sizes are updated
        self:GetParent():playcommand("UpdateSizing", params)

        lastOffsets = params.offsetVector
        lastTracks = params.trackVector
        lastTimingData = params.timingData
        lastTypes = params.typeVector
        lastHolds = params.holdVector
        lastTiming = {}
        for i, row in ipairs(params.noteRowVector) do
            lastTiming[i] = lastTimingData:GetElapsedTimeFromNoteRow(row)
        end

        lastMaxTime = params.maxTime
        lastColumns = params.columns
        if not params.rejudged then
            highlightTable = calcDotHighlightTable(lastTracks, lastColumns)
            highlightIndex = 1
        end

        -- draw dots
        self:playcommand("DrawOffsets")
    end,
    DrawOffsetsCommand = function(self)
        local vertices = {}
        local offsets = lastOffsets
        local tracks = lastTracks
        local timing = lastTiming
        local types = lastTypes
        local holds = lastHolds
        local maxTime = lastMaxTime
        self:GetParent():playcommand("UpdateText")

        if offsets == nil or #offsets == 0 then
            self:SetVertices(vertices)
            self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = 0}
            return
        end

        -- dynamically change the dot size depending on the number of dots
        -- for clarity on ultra dense scores
        dotLineLength = clamp(scale(#offsets, 1000, 5000, dotLineUpperBound, dotLineLowerBound), dotLineLowerBound, dotLineUpperBound)

        -- taps and mines
        for i, offset in ipairs(offsets) do
            local x = fitX(timing[i], maxTime)
            local y = fitY(offset, maxOffset)
            local column = tracks ~= nil and tracks[i] ~= nil and tracks[i] + 1 or nil

            local cappedY = math.max(maxOffset, (maxOffset) * timingScale)
            if y < 0 or y > sizing.Height then
                y = fitY(cappedY, maxOffset)
            end


            if types[i] ~= "TapNoteType_Mine" then
                -- handle highlighting logic
                local dotColor = colorByTapOffset(offset, timingScale)
                if usingCustomWindows then
                    dotColor = colorByTapOffsetCustomWindow(offset, getCurrentCustomWindowConfigJudgmentWindowTable())
                end
                if not columnIsHighlighted(column) then
                    dotColor[4] = unHighlightedAlpha
                end
                placeDotVertices(vertices, x, y, dotColor)
            else
                -- this function handles the highlight logic
                local mineColor = getMineColor(column)
                placeMineVertices(vertices, x, fitY(-maxOffset, maxOffset), mineColor)
            end
        end

        -- holds and rolls
        --[[
        if holds ~= nil and #holds > 0 then
            for i, h in ipairs(holds) do
                local row = h.row
                local holdtype = h.TapNoteSubType
                local column = h.track + 1
                local holdColor = getHoldColor(column, holdtype)
                local rowtime = lastTimingData:GetElapsedTimeFromNoteRow(row) 
                local x = fitX(rowtime, maxTime)
                placeNoodleVertices(vertices, x, fitY(-maxOffset, maxOffset), holdColor)
            end
        end
        ]]

        -- animation breaks if we start from nothing
        if self:GetNumVertices() ~= 0 then
            self:finishtweening()
            self:smooth(dotAnimationSeconds)
        end
        self:SetVertices(vertices)
        self:SetDrawState {Mode = "DrawMode_Quads", First = 1, Num = #vertices}
    end
}



return t