local log = math.log
local concat = function(...)
    local arg = {...}
    local t = {}
    for i = 1, #arg do
        for i, v in ipairs(arg[i]) do
            t[#t + 1] = v
        end
    end
    return t
end
local function dump(o)
    if type(o) == "table" then
        local s = "{ "
        for k, v in pairs(o) do
            if type(k) ~= "number" then
                k = '"' .. k .. '"'
            end
            s = s .. "[" .. k .. "] = " .. dump(v) .. ",\n"
        end
        return s .. "} "
    else
        return tostring(o)
    end
end
audioVisualizer = {}
--[[
Note: This is relatively barebones, and not very customizable.
Ex:
    t[#t+1] = audioVisualizer:new {
        x = SCREEN_RIGHT*1/3,
        y = SCREEN_BOTTOM*3/11,
        color = getMainColor('positive'),
        onInit = function(frame)
            local soundActor = frame.sound.actor
            local songs = SONGMAN:GetAllSongs()
            local idx = math.random(1, #songs)
            soundActor:stop()
            soundActor:load(songs[idx]:GetMusicPath())
            soundActor:play()
        end
    }
Using SOUND:
    local vis = audioVisualizer:new {
        x = SCREEN_RIGHT*1/3,
        y = SCREEN_BOTTOM*3/11,
        color = getMainColor('positive')
    }
    t[#t+1] = vis
    SOUND:SetPlayBackCallback(vis.playbackFunction)
--]]
function audioVisualizer.multiplyIntervals(ints, n)
    local t = {}
    for i, v in ipairs(ints) do
        local a = v
        local b = ints[i + 1]
        if b then
            for j = 1, n do
                t[#t + 1] = a + (b - a) * j / n
            end
        end
    end
    return t
end
audioVisualizer.defaultIntervals = {0, 19.0, 35, 60, 90, 140, 240, 400, 800, 1600, 2600, 3900, 5200}
local defaultIntervals = audioVisualizer.defaultIntervals
-- for i = 0, 67 do
--   defaultIntervals[#defaultIntervals + 1] = i * i * 5
-- end
--defaultIntervals[#defaultIntervals + 1] = 99999
--[[
    params = {
        x=0,
        y=0,
        color = red,
        minHeight = 3,
        maxHeight = 120,
        width = 300, -- Of the whole thing
        spacing = 1, -- Between bars
        barBuilder = quad, -- Function that takes params
        sampleCount = 4096, -- number of samples per update
        onBarUpdate = nil, -- function(barActor, value) called whenever theyre updated
            (value is in the [0,1] range)
        onInit = nil
    }
]]
function audioVisualizer:new(params)
    --[[
        Structure:
        ActorFrame {
            bars = {Quad,Quad,etc},
            values = {0, valueQuad1,valueQuad2,etc},
            sound = ActorSound
        }
    --]]
    local frame =
        Def.ActorFrame {
        InitCommand = function(self)
            self:xy(params.x or 0, params.y or 0)
        end,
        CurrentSongChangedMessageCommand = function(self)
            self:RunCommandsOnChildren(
                function(self)
                    self:finishtweening():smooth(0.3):zoomtoheight(0)
                end
            )
        end
    }
    -- Freq intervals for each bar
    local freqIntervals
    do
        local rawFreqIntervals = params.freqIntervals or defaultIntervals
        freqIntervals = concat({0}, rawFreqIntervals)
    end
    -- Values for the visualizer
    frame.values = {}
    for i, v in ipairs(freqIntervals) do
        frame.values[i] = 0
    end
    -- bar actor updater
    do
        local minHeight = params.minHeight or 3
        local maxHeight = (params.maxHeight or 120) - minHeight
        if params.onBarUpdate then
            frame.updater = params.barUpdater or function(actor, value)
                    actor:hurrytweening(0.15):smooth(0.22):zoomtoheight(minHeight + value * maxHeight)
                    params.onBarUpdate(actor, value)
                end
        else
            frame.updater = params.barUpdater or function(actor, value)
                    actor:hurrytweening(0.15):smooth(0.22):zoomtoheight(minHeight + value * maxHeight)
                end
        end
    end
    -- Bar actors
    frame.bars = {}
    -- ignore 0-firstFreq, so bars start from value 2, and we start from 3
    -- since we use 2 frequencies per bar (intervals)
    params.width = params.width or 300
    params.spacing = params.spacing or 1
    do
        local color = params.color or color("#FF00000")
        local intCount = #freqIntervals
        local width = (params.width - intCount * params.spacing) / (#freqIntervals - 2)
        local pos = width + params.spacing
        for i = 3, intCount do
            frame[#frame + 1] =
                (params.barBuilder or
                function()
                    return Def.Quad {
                        InitCommand = function(self)
                            (frame.bars)[i - 2] = self
                            self:valign(1):x(pos * (i - 2)):diffuse(color):zoomtowidth(width)
                        end
                    }
                end)(params)
        end
    end
    local soundActor
    frame.sound = Def.Sound {}
    local addToBin
    do
        local values = frame.values
        addToBin = function(magnitude, freq)
            for i = 2, #freqIntervals do
                if freq > freqIntervals[i - 1] and freq <= freqIntervals[i] then
                    values[i - 1] = values[i - 1] + magnitude
                    return
                end
            end
        end
    end
    -- Add magnitude to the appropiate bar's value (aka falls in the freq interval)
    local screen
    local values = frame.values
    frame.playbackFunction = function(fft, ss)
        -- cleanup
        if screen ~= SCREENMAN:GetTopScreen() then
            SOUND:ClearPlayBackCallback()
            return
        end
        local samplingRate = ss:GetSampleRate()
        local count = #fft

        for i = 1, count do
            -- samples = count*2
            -- nyquist limit => freq=i * (samplingRate/2) / samples
            addToBin(math.sqrt(fft[i]), i * samplingRate / (4 * count))
        end

        SCREENMAN:GetTopScreen():setTimeout(
            function()
                local updater = frame.updater
                local bars = frame.bars
                local max = math.max(100.0, unpack(values, 2))
                for i = 1, #bars do
                    -- turn into linear scale
                    local x = math.min(values[i + 1] / max, 1)
                    -- turn into log scale
                    x = log(x + 1) / log(2)
                    values[i + 1] = x
                    updater(bars[i], x)
                    values[i + 1] = 0
                end
            end,
            (count / 2) / samplingRate
        )
    end
    frame.sampleCount = params.sampleCount or 8192
    frame.sound.InitCommand = function(self)
        frame.sound.actor = self
        local rSound = self:get()
        rSound:SetPlayBackCallback(frame.playbackFunction, frame.sampleCount)
        if params.onInit then
            params.onInit(frame)
        end
    end
    frame.BeginCommand = function(self)
        screen = SCREENMAN:GetTopScreen()
        SOUND:SetPlayBackCallback(frame.playbackFunction, frame.sampleCount)
    end
    frame[#frame + 1] = frame.sound
    return frame
end
