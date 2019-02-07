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
audioVisualizer.defaultIntervals = {}
local defaultIntervals = audioVisualizer.defaultIntervals
for i = 0, 67 do
    defaultIntervals[#defaultIntervals + 1] = i * i * 5
end
defaultIntervals[#defaultIntervals + 1] = 99999
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
        end
    }
    -- Freq intervals for each bar
    local freqIntervals
    do
        local rawFreqIntervals = params.freqIntervals or defaultIntervals --{40, 70, 100, 300, 500, 1000, 2000, 4000, 9000, 18000, 99999}
        freqIntervals = concat({0}, rawFreqIntervals)
    end
    -- Values for the visualizer
    frame.values = {}
    for i, v in ipairs(freqIntervals) do
        frame.values[i] = 0
    end
    -- bar actor updater
    do
        local prevVals = {}
        local scale = 1000
        local maxHeight = params.maxHeight or 120
        local minHeight = params.minHeight or 3
		frame.updater = params.barUpdater or function(actor, value)
			actor:zoomtoheight(value)
		end
    end
    -- Bar actors
    frame.bars = {}
    -- ignore 0-firstFreq, so bars start from value 2, and we start from 3
    -- since we use 2 frequencies per bar (intervals)
    do
        local color = params.color or color("#FF00000")
        for i = 3, #freqIntervals do
            frame[#frame + 1] =
                (params.barBuilder or
                function()
                    return Def.Quad {
                        InitCommand = function(self)
                            (frame.bars)[i - 2] = self
                            local width = (params.width or 300) / (#freqIntervals - 2)
                            self:valign(1):x((width + (params.spacing or 1)) * (i - 2)):diffuse(color):zoomtowidth(
                                width
                            )
                        end
                    }
                end)(params)
        end
    end
    local updateAll = function()
        for i, v in ipairs(frame.bars) do
            frame.updater(v, frame.values[i + 1])
        end
    end
    local soundActor
    frame.sound = Def.Sound {}
    -- Add magnitude to the appropiate bar's value (aka falls in the freq interval)
	frame.playbackFunction = function(fft, ss)
		-- reset values
        local values = frame.values
		for i = 1, #values do
            values[i] = 0
		end

		local function addToBin(magnitude, freq)
			for i = 2, #freqIntervals do
				if freq/2 > freqIntervals[i - 1] and freq/2 <= freqIntervals[i] then
					values[i - 1] = values[i - 1] + magnitude
					return
				end
			end
		end

			local samplingRate = ss:GetSampleRate()
			local count = #fft
			local m = count * 2 -- Amount of samples. N samples yield n/2+1 useful fft values
			for i = 1, count do
            	addToBin(math.sqrt(fft[i]), i * samplingRate / m)
			end
            local updater = frame.updater
			local bars = frame.bars
            for i = 1, #bars do
                updater(bars[i], values[i + 1])
            end
    end
    frame.sampleCount = params.sampleCount or 4096
    frame.sound.InitCommand = function(self)
        frame.sound.actor = self
        local rSound = self:get()
        rSound:SetPlayBackCallback(frame.playbackFunction, frame.sampleCount)
        if params.onInit then
            params.onInit(frame)
        end
    end
    frame[#frame + 1] = frame.sound
    return frame
end