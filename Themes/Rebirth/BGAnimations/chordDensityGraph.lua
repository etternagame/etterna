local sizing = Var("sizing") -- specify init sizing
if sizing == nil then sizing = {} end
--[[
    We are expecting the sizing table to be provided on file load.
    It should contain these attributes:
    Width
    Height
    NPSThickness
    TextSize
]]
-- the bg is placed relative to top left: 0,0 alignment
-- the bars are placed relative to bottom left: 1 valign 0 halign
local stepsinuse = nil
local lowDensityColor = COLORS:getColor("chartPreview", "GraphLowestDensityBar")
local highDensityColor = COLORS:getColor("chartPreview", "GraphHighestDensityBar")

local translations = {
    NPS = THEME:GetString("ChordDensityGraph", "NPS"),
    BPM = THEME:GetString("ChordDensityGraph", "BPM"),
}

local t = Def.ActorFrame {
    Name = "ChordDensityGraphFile",
    InitCommand = function(self)
        self:playcommand("UpdateSizing", {sizing = sizing})
        self:finishtweening()
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("LoadDensityGraph", {steps = stepsinuse})
    end,
    LoadDensityGraphCommand = function(self, params)
        stepsinuse = params.steps or stepsinuse
    end,
    UpdateSizingCommand = function(self, params)
        local sz = params.sizing
        if sz ~= nil then
            if sz.Height ~= nil then
                sizing.Height = sz.Height
            end
            if sz.Width ~= nil then
                sizing.Width = sz.Width
            end
            if sz.NPSThickness ~= nil then
                sizing.NPSThickness = sz.NPSThickness
            end
            if sz.TextSize ~= nil then
                sizing.TextSize = sz.TextSize
            end
        end
        local bar = self:GetChild("Progress")
        local bg = self:GetChild("BG")
        local seek = self:GetChild("SeekBar")
        local tipOn = false
        self:SetUpdateFunction(function(self)
            if self:IsInvisible() then return end

            local top = SCREENMAN:GetTopScreen()
            local song = GAMESTATE:GetCurrentSong()
            local musicpositionratio = 1
            local r = getCurRateValue()
            if stepsinuse ~= nil and top ~= nil and song then
                local length = stepsinuse:GetLengthSeconds()
                musicpositionratio = (stepsinuse:GetFirstSecond() / r + length) / sizing.Width * r
                if top.GetSampleMusicPosition then
                    local pos = top:GetSampleMusicPosition() / musicpositionratio
                    bar:zoomx(clamp(pos, 0, sizing.Width))
                elseif top.GetSongPosition then
                    local pos = top:GetSongPosition() / musicpositionratio
                    bar:zoomx(clamp(pos, 0, sizing.Width))
                else
                    bar:zoomx(0)
                end
            else
                bar:zoomx(0)
            end
            if song and isOver(bg) then
                local mx = INPUTFILTER:GetMouseX()
                local my = INPUTFILTER:GetMouseY()
                local lx, ly = bg:GetLocalMousePos(mx, my, 0)
                seek:diffusealpha(1)
                seek:x(lx)

                if stepsinuse ~= nil then
                    local perc = lx / bg:GetZoomedWidth()
                    local dist = perc * (stepsinuse:GetLengthSeconds())
                    local postext = SecondsToHHMMSS(dist)
                    local stro = postext
                    if self.npsVector ~= nil and #self.npsVector > 0 then
                        local percent = clamp(lx / bg:GetZoomedWidth(), 0, 1)
                        local hoveredIndex = clamp(math.ceil(self.finalNPSVectorIndex * percent), math.min(1, self.finalNPSVectorIndex), self.finalNPSVectorIndex)
                        local hoveredNPS = self.npsVector[hoveredIndex]
                        local td = stepsinuse:GetTimingData()
                        local bpm = td:GetBPMAtBeat(td:GetBeatFromElapsedTime(dist * r)) * r
                        stro = string.format("%s\n%d %s\n%d %s", postext, hoveredNPS, translations["NPS"], bpm, translations["BPM"])
                    end
                    TOOLTIP:SetText(stro)
                    TOOLTIP:Show()
                    tipOn = true
                end

            else
                seek:diffusealpha(0)

                -- have to micromanage the state of the tooltip here
                -- turning it off over and over will override any other use of the tooltip
                if tipOn then
                    tipOn = false
                    TOOLTIP:Hide()
                end
            end
        end)
    end
}

local function getColorForDensity(density, nColumns)
    -- Generically (generally? intelligently? i dont know) set a range
    -- The value var describes the level of density.
    -- Beginning at lowVal for 0, to highVal for nColumns.
    local interval = 1 / nColumns
    local value = density * interval
    return lerp_color(value, lowDensityColor, highDensityColor)
end

local function makeABar(vertices, x, y, barWidth, barHeight, thecolor)
    -- These bars are vertical, progressively going across the screen
    -- Their corners are: (x,y), (x, y-barHeight), (x-barWidth, y-barHeight), (x-barWidth, y)
    vertices[#vertices + 1] = {{x,y-barHeight,0},thecolor}
    vertices[#vertices + 1] = {{x-barWidth,y-barHeight,0},thecolor}
    vertices[#vertices + 1] = {{x-barWidth,y,0},thecolor}
    vertices[#vertices + 1] = {{x,y,0},thecolor}
end

local function updateGraphMultiVertex(parent, self, steps)
    if steps then
        local ncol = steps:GetNumColumns()
        local rate = math.max(1, getCurRateValue())
        local graphVectors = steps:GetCDGraphVectors(rate)
        local txt = parent:GetChild("NPSText")
        if graphVectors == nil then
            -- reset everything if theres nothing to show
            self:SetVertices({})
            self:SetDrawState( {Mode = "DrawMode_Quads", First = 0, Num = 0} )
            txt:settext("")
            return
        end

        local npsVector = graphVectors[1] -- refers to the cps vector for 1 (tap notes)
        local numberOfColumns = #npsVector
        local columnWidth = sizing.Width / numberOfColumns * rate

        -- set height scale of graph relative to the max nps
        local heightScale = 0
        for i=1,#npsVector do
            if npsVector[i] * 2 > heightScale then
                heightScale = npsVector[i] * 2
            end
        end

        txt:settext(heightScale / 2 * 0.7 .. translations["NPS"])
        heightScale = sizing.Height / heightScale

        local verts = {} -- reset the vertices for the graph
        local yOffset = 0 -- completely unnecessary, just a Y offset from the graph
        local lastIndex = 1
        for density = 1,ncol do
            for column = 1,numberOfColumns do
                if graphVectors[density][column] > 0 then
                    local barColor = getColorForDensity(density, ncol)
                    makeABar(verts, math.min(column * columnWidth, sizing.Width), yOffset, columnWidth, graphVectors[density][column] * 2 * heightScale, barColor)
                    if column > lastIndex then
                        lastIndex = column
                    end
                end
            end
        end

        parent.npsVector = npsVector
        parent.finalNPSVectorIndex = lastIndex -- massive hack because npsVector is padded with 0s on uprates

        self:SetVertices(verts)
        self:SetDrawState( {Mode = "DrawMode_Quads", First = 1, Num = #verts} )
    else
        -- reset everything if theres nothing to show
        self:SetVertices({})
        self:SetDrawState( {Mode = "DrawMode_Quads", First = 0, Num = 0} )
        parent:GetChild("NPSText"):settext("")
    end
end

local textzoomFudge = 5
local resizeAnimationSeconds = 0.1

t[#t+1] = UIElements.QuadButton(1, 1) .. {
    Name = "BG",
    InitCommand = function(self)
        self:halign(0):valign(0)
        registerActorToColorConfigElement(self, "chartPreview", "GraphBackground")
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:zoomto(sizing.Width, sizing.Height)
    end,
    MouseDownCommand = function(self, params)
        if self:IsInvisible() then return end
        local lx = params.MouseX - self:GetParent():GetX()
        local top = SCREENMAN:GetTopScreen()
        if top and top.SetSongPosition then
            local song = GAMESTATE:GetCurrentSong()
            -- logic for gameplay chord density graph
            if stepsinuse and song then
                local r = getCurRateValue()
                local length = stepsinuse:GetLengthSeconds()
                local musicpositionratio = (stepsinuse:GetFirstSecond() / r + length) / sizing.Width * r
                if params.event == "DeviceButton_left mouse button" then
                    local withCtrl = INPUTFILTER:IsControlPressed()
                    if withCtrl then
                        -- this command is defined remotely by _gameplaypractice.lua
                        self:playcommand("HandleRegionSetting", {positionGiven = lx * musicpositionratio})
                    else
                        top:SetSongPosition(lx * musicpositionratio, 0, false)
                    end
                elseif params.event == "DeviceButton_right mouse button" then
                    -- this command is defined remotely by _gameplaypractice.lua
                    self:playcommand("HandleRegionSetting", {positionGiven = lx * musicpositionratio})
                end
            end
        elseif top and top.SetSampleMusicPosition then
            -- logic for selectmusic chord density graph
            if params.event == "DeviceButton_left mouse button" then
                local song = GAMESTATE:GetCurrentSong()
                if stepsinuse and song then
                    local r = getCurRateValue()
                    local length = stepsinuse:GetLengthSeconds()
                    local musicpositionratio = (stepsinuse:GetFirstSecond() / r + length) / sizing.Width * r
                    top:SetSampleMusicPosition(lx * musicpositionratio)
                end
            else
                top:PauseSampleMusic()
            end
        end
    end
}

t[#t+1] = Def.Quad {
    Name = "Progress",
    InitCommand = function(self)
        self:halign(0):valign(0)
        self:diffusealpha(0.5)
        registerActorToColorConfigElement(self, "chartPreview", "GraphProgress")
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:zoomto(0, sizing.Height)
    end
}

t[#t+1] = Def.ActorMultiVertex {
    Name = "ChordDensityGraphAMV",
    UpdateSizingCommand = function(self)
        -- this will position the plot relative to the bottom left of the area
        -- less math, more easy, progarming fun
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:y(sizing.Height)
    end,
    ColorConfigUpdatedMessageCommand = function(self)
        lowDensityColor = COLORS:getColor("chartPreview", "GraphLowestDensityBar")
        highDensityColor = COLORS:getColor("chartPreview", "GraphHighestDensityBar")
        updateGraphMultiVertex(self:GetParent(), self, self.s)
    end,
    LoadDensityGraphCommand = function(self, params)
        self.s = params.steps
        updateGraphMultiVertex(self:GetParent(), self, params.steps)
    end,
}

t[#t+1] = Def.Quad {
    Name = "NPSLine",
    InitCommand = function(self)
        self:halign(0)
        registerActorToColorConfigElement(self, "chartPreview", "GraphNPSLine")
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        -- the NPS Line represents 75% of the actual NPS
        -- the position is relative to top left, so move it 25% of the way down
        -- do not valign this (this means the center of this line is 75%)
        self:y(sizing.Height * 0.25)
        self:zoomto(sizing.Width, sizing.NPSThickness)
    end,
}

t[#t+1] = LoadFont("Common Normal") .. {
    Name = "NPSText",
    InitCommand = function(self)
        self:halign(0):valign(1)
        registerActorToColorConfigElement(self, "chartPreview", "GraphNPSText")
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        local zs = sizing.TextSize or 1
        self:zoom(zs)
        -- this text is positioned above the NPS line, basically half the thickness above it
        self:y(sizing.Height * 0.25 - sizing.NPSThickness * 0.75)
        self:maxwidth(sizing.Width / zs - textzoomFudge)
    end,
}

t[#t+1] = Def.Quad {
    Name = "SeekBar",
    InitCommand = function(self)
        self:valign(0)
        self:diffusealpha(0)
        registerActorToColorConfigElement(self, "chartPreview", "GraphSeekBar")
    end,
    UpdateSizingCommand = function(self)
        self:finishtweening()
        self:smooth(resizeAnimationSeconds)
        self:zoomto(sizing.NPSThickness, sizing.Height)
    end
}

return t
