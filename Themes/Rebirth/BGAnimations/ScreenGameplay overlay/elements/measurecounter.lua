-- a counter which measures measures
-- displays the length and current progress of runs of the highest nps sections in the file

local measures = {}

-- current real beat within the current measure
local beatcounter = 0

-- current real measure index
local measure = 1

-- last active measure counter display run index
local thingy = 1

-- is the display visible
local active = false

-- percentage of the nps peak of a file a run can reach before being forced to end
-- (a burst stops the current run)
local peakNPSThreshold = 0.625

-- amount of current measure NPS to be different from current run NPS, causing an end to the current run
-- (a break stops the current run)
local runNPSLossThreshold = 2

local totalmeasures = 0

local t = Def.ActorFrame {
    Name = "MeasureCounter",
    InitCommand = function(self)

        local steps = GAMESTATE:GetCurrentSteps()
        local loot = steps:GetNPSPerMeasure(1)

        -- determine peak
        local peak = 0
        for i = 1, #loot do
            if loot[i] > peak then
                peak = loot[i]
            end
        end

        local m_len = 0
        local m_spd = 0
        local m_start = 0
        for i = 1, #loot do
            -- run init
            -- set speed and start measure
            if m_len == 0 then
                m_spd = loot[i]
                m_start = i
            end

            if math.abs(m_spd - loot[i]) < runNPSLossThreshold then
                -- if the past measure is not too much different from the current, continue the run
                m_len = m_len + 1
                m_spd = (m_spd + loot[i]) / 2
            elseif m_len > 1 and m_spd > peak * peakNPSThreshold then
                -- if this measure bursts above the nps threshold, end the run
                measures[#measures + 1] = { m_start, m_len, m_spd }
                m_len = 0
            else
                m_len = 0
            end
        end

        totalmeasures = #measures

        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    OnCommand = function(self)
        self:visible(false)

        if totalmeasures > 0 then
            -- if the start measure is a measure to display, go
            if measure == measures[thingy][1] then
                self:playcommand("Dootz")
            end
        end

        if allowedCustomization then
            self:visible(true)
            self:GetChild("Text"):settext("99 / 99")
        end
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.MeasureCounterX, MovableValues.MeasureCounterY)
        self:zoom(MovableValues.MeasureCounterZoom)
    end,
    DootzCommand = function(self)
        active = true
        self:visible(true)

        local txt = self:GetChild("Text")
        local bg = self:GetChild("Background")

        txt:settextf("%d / %d", measure - measures[thingy][1], measures[thingy][2])
        bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight())
    end,
    UnDootzCommand = function(self)
        active = false
        self:visible(false)
    end,
    BeatCrossedMessageCommand = function(self)
        if totalmeasures > 0 and thingy <= totalmeasures then
            beatcounter = beatcounter + 1
            if beatcounter == 4 then
                measure = measure + 1
                beatcounter = 0

                if measure == measures[thingy][1] then
                    self:playcommand("Dootz")
                end

                if measure > measures[thingy][1] + measures[thingy][2] then
                    self:playcommand("UnDootz")
                    thingy = thingy + 1
                end

                if active then
                    self:playcommand("MeasureCrossed")
                end
            end
        end
    end,
    MeasureCrossedCommand = function(self)
        local txt = self:GetChild("Text")
        local bg = self:GetChild("Background")

        if totalmeasures > 0 then
            txt:settextf("%d / %d", measure - measures[thingy][1], measures[thingy][2])
            bg:zoomto(txt:GetZoomedWidth(), txt:GetZoomedHeight())
        end
    end,

    Def.Quad {
        Name = "Background",
        InitCommand = function(self)
            self:diffusealpha(0.6)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryBackground")
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Text",
        InitCommand = function(self)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
        end,
    }
}

return t
