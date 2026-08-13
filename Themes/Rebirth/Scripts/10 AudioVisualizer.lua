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

-- fix 0s in the table
local function smoothZeros(table)
    for i = 1, #table do
        if table[i] == 0 then
            if i == 1 then
                if #table > 1 then
                    table[i] = table[i+1] / 2
                end
            elseif i == #table then
                table[i] = table[i-1] / 2
            else
                table[i] = (table[i-1] + table[i+1]) / 2
            end
        end
    end
end

local function weightedAverage(values, weights)
    local weightedSum = 0
    for i,v in ipairs(values) do
        weightedSum = weightedSum + (v * weights[i])
    end
    return weightedSum / table.sum(weights)
end

local function dummy_weights(count)
    local o = {}
    for i = 1, count do
        o[#o+1] = 1
    end
    return o
end

-- simply one of the ways to do binning
-- https://www.dlbeer.co.nz/articles/fftvis.html
local function dlbBins(bars, fft, values, lastframevals)
    --local smoothingval = math.pow(0.00000000001, count / samplingRate)
    local smoothingval = 0.20
    local scaleval = 0.95
    local freq_start = 0
    local gamma_correction_val = 2

    local freq_count = #fft
    local unique_freqs = math.floor(freq_count / 2)

    for i = 1, #bars do
        local freq_end = math.ceil(math.pow((i+1) / #bars, gamma_correction_val) * unique_freqs)
        if freq_end > unique_freqs then freq_end = unique_freqs end

        freq_end = clamp(freq_end, 1, freq_count)
        freq_start = clamp(freq_start, 1, freq_count)

        local bigval = 0
        for j = freq_start, freq_end do
            local fftval = fft[j]
            if fftval > bigval then bigval = fftval end
        end

        -- horrible deception (mostly matters for higher freq)
        local logval = log(bigval)
        if logval > 0 then bigval = logval end

        --ms.ok("for bar "..i .. " --- range ".. freq_start .. " - "..freq_end .. " placed val " .. bigval)

        local smoothened = lastframevals[i] * smoothingval + (bigval * scaleval * (1 - smoothingval))
        lastframevals[i] = values[i]
        values[i] = smoothened

        freq_start = freq_end
    end
end

-- bins based on how the bark scale works
-- https://en.wikipedia.org/wiki/Bark_scale
local function barkBins(bars, fft, values, lastframevals, samplingRate)

    -- each bin represents a frequency range
    -- a particular frequency fits into a bark
    -- unless we used 24 bins, there is bleed across bins

    local fft_bins = #bars
    local nyq = samplingRate / 2
    local fft_val_count = #fft

    -- percentage of lastframevals to use for this frame
    local smoothingval = 0.5

    local function bark_at_freq(x)
        --return 13 * math.atan(0.00076 * x) + 3.5 * math.atan(math.pow(x / 7500, 2))
        return ((26.81 * x) / (1960 + x)) - 0.53 
    end

    local barks = {}
    local function emplace_bark_val(v, i)
        if barks[i] == nil then
            barks[i] = {}
        end
        barks[i][#barks[i]+1] = v
    end

    for i = 1, fft_val_count do

        local freq_at_i = (i / fft_val_count) * nyq
        local val = fft[i]

        local bark = bark_at_freq(freq_at_i) / 24 * fft_bins
        local bark_i = math.round(bark)
        
        emplace_bark_val(val, bark_i)
    end

    for bark_i = 1, fft_bins do
        if barks[bark_i] == nil then
            values[bark_i] = 0
        else

            -- preferable but not in use
            --values[bark_i] = log(weightedAverage(barks[bark_i], dummy_weights(#barks[bark_i])))

            values[bark_i] = log(table.average(barks[bark_i]))

            if values[bark_i] < 0 then values[bark_i] = 0 end
        end
        local smoothened = lastframevals[bark_i] * smoothingval + (values[bark_i] * (1 - smoothingval))
        lastframevals[bark_i] = values[bark_i]
        values[bark_i] = smoothened
    end
end

local function aweight(frequency)
    local f2 = frequency * frequency
    local f4 = f2 * f2
    local a2 = 20.6 * 20.6
    local b2 = 107.7 * 107.7
    local c2 = 737.9 * 737.9
    local d2 = 12194 * 12194
    local RA = (d2 * f4) / ((f2 + a2) * math.sqrt((f2 + b2) * (f2 + c2)) * (f2 + d2))
    -- conversion to dB is done outside this function
    -- raw aweighting is too strong, so sqrt + blend towards 1 to tone it down
    return 0.303 + 0.707*math.sqrt(RA)
end

local function perceptualBins(bars, fft, values, lastframevals, samplingRate, nfftbins)

    -- tries to look perceptually Right using various doodads

    local B = #bars
    local N = nfftbins
    local nyq = samplingRate / 2
    local T = N / nyq

    local function remap(x, a, b, A, B)
        local t = (x - a) / (b - a)
        return (1 - t)*A + t*B
    end

    local function fft_bin_to_freq(b)
        return nyq * (b / N)
    end

    local function freq_to_fft_bin(x)
        return (x / nyq) * N
    end

    -- assign semitoneesque spacing to bars
    local first_bin_freq = 20
    local first_mids_bin_freq = 120
    local last_bin_freq = math.min(7360, fft_bin_to_freq(#fft))
    local log_first_bin_freq = math.log(first_bin_freq, 2)
    local log_first_mids_bin_freq = math.log(first_mids_bin_freq, 2)
    local log_last_bin_freq = math.log(last_bin_freq, 2)

    local mids_bar = clamp(B / 8, 2, B)
    local function bar_to_freq(b)
        -- bass takes up too many bars for the resolution we have
        -- this stitches together two domain warps with a kink at the boundary
        -- you can see it if you look for it
        local logx = 0
        if b < mids_bar then
            logx = remap(b, 1, mids_bar, log_first_bin_freq, log_first_mids_bin_freq)
        else
            logx = remap(b, mids_bar, B, log_first_mids_bin_freq, log_last_bin_freq)
        end

        return math.pow(2, logx)
    end

    local function sample(xs, n)
        local clamped = clamp(n, 1, #xs - 1)
        local i = math.floor(clamped)
        local t = clamped - i
        return xs[i] * (1 - t) + xs[i+1]*t
    end

    local bass_nrg = 1e-8
    local mids_nrg = 1e-8
    local mids_bin = math.ceil(freq_to_fft_bin(first_mids_bin_freq))
    local upper_bin = math.min(#fft, math.ceil(freq_to_fft_bin(last_bin_freq / 2)))
    for i = 1, mids_bin do
        bass_nrg = bass_nrg + fft[i]
    end
    for i = mids_bin, upper_bin do
        mids_nrg = mids_nrg + fft[i]
    end
    bass_nrg = bass_nrg / mids_bin
    mids_nrg = mids_nrg / (upper_bin - mids_bin)
    -- boosts the entire frequency range when something is going on in the bass
    -- but looks super suspicious when a sound is mostly bass or has no bass at all, so add in a mids correction
    -- pow by 0.25 dampens the effect
    -- this only really works well because aweight is going to kill off bass frequencies almost entirely,
    -- so the fact that the entire spectrum is being lifted by the bass is not obvious
    local nrg = math.pow(bass_nrg + bass_nrg/mids_nrg, 0.25)

    for i = 1, B do
        local fft_bin_lo = clamp(freq_to_fft_bin(bar_to_freq(i - 0.5)), 1, #fft - 1)
        local fft_bin_hi = clamp(freq_to_fft_bin(bar_to_freq(i + 0.5)), 2, #fft)
        local freq = bar_to_freq(i)
        local fft_bin_here = clamp(freq_to_fft_bin(freq), 1, #fft)

        -- ms.ok("bar " .. i .. " " .. bar_to_freq(i) .. " "  .. fft_bin_here .. " " .. fft_bin_lo .. " " .. fft_bin_hi)

        local val = 0
        local weight_sum = 0
        local n_window = clamp(math.ceil(fft_bin_hi) - math.floor(fft_bin_lo), 2, 12)
        for j = 0, n_window do
            local fft_bin = remap(j, 0, n_window, fft_bin_lo, fft_bin_hi)
            local bin_delta = (fft_bin_here - fft_bin) / n_window
            local weight = math.exp(bin_delta*bin_delta)
            val = val + sample(fft, fft_bin) * weight
            weight_sum = weight_sum + weight
        end

        -- vaguely dBish
        val = aweight(freq) * 2 * math.log(1.0 + nrg * val / (nfftbins * weight_sum), 10)

        -- percentage of val to use for this frame
        -- sample rate and fft size independent
        -- faster decay for high freqs
        -- faster decay when louder than last frame
        -- slower decay when much quieter than last frame, makes salient musical events linger a bit
        local last_val = values[i]
        local base_rate = 8
        local freq_strength = 2
        local vol_strength = 1
        local freq_rate = (1 + freq_strength * freq / last_bin_freq)
        local vol_rate = math.exp(vol_strength * math.tanh(val - last_val))
        local rate = base_rate * freq_rate * vol_rate
        local alpha = math.pow(2, -rate*T)

        values[i] = (last_val * alpha) + (val * (1 - alpha))
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
                    if GAMESTATE:GetCurrentSong() ~= nil then
                        self:finishtweening():smooth(0.3):zoomtoheight(0)
                    end
                end
            )
        end
    }
    params.barcount = params.barcount or 16

    -- Values for the visualizer
    frame.values = {}
    frame.lastframevals = {}
    frame.barcount = params.barcount
    for i=1, frame.barcount do
        frame.values[i] = 0
        frame.lastframevals[i] = 0
    end
    -- bar actor updater
    do
        local minHeight = params.minHeight or 3
        local maxHeight = (params.maxHeight or 120) - minHeight
        if params.onBarUpdate then
            frame.updater = params.barUpdater or function(actor, value)
                    actor
                        :stoptweening()
                        :zoomtoheight(minHeight + value * maxHeight)
                    params.onBarUpdate(actor, value)
                end
        else
            frame.updater = params.barUpdater or function(actor, value)
                    actor
                        :stoptweening()
                        :zoomtoheight(minHeight + value * maxHeight)
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
        local intCount = params.barcount
        local width = (params.width - intCount * params.spacing) / (params.barcount - 2)
        local pos = width + params.spacing
        for i = 3, intCount do
            frame[#frame + 1] =
                (params.barBuilder or
                function()
                    return Def.Quad {
                        Name = "VisualizerBar_"..i-2,
                        InitCommand = function(self)
                            (frame.bars)[i - 2] = self
                            self:valign(1):x(pos * (i - 2)):diffuse(color):zoomtowidth(width)
                        end,
                        ResetWidthCommand = function(self, given)
                            local width = (given.width - intCount * params.spacing) / (params.barcount - 2)
                            local pos = width + params.spacing
                            self:x(pos * (i-2))
                            self:zoomtowidth(width)
                        end
                    }
                end)(params)
        end
    end
    local soundActor
    frame.sound = Def.Sound {}
    -- Add magnitude to the appropiate bar's value (aka falls in the freq interval)
    local screen
    local values = frame.values
    local lastframevals = frame.values

    frame.playbackFunction = function(fft, ss)

        ----------- INIT --------------
        local samplingRate = ss:GetSampleRate()
        local updater = frame.updater
        local bars = frame.bars
        local nbins = #fft


        ----------- CLEAN DATA ---------
        -- cut out the upper part of the fft values
        -- because it makes the output look so much better and i dont know why
        -- (sometimes really cool stuff is cut off here but like 95% of the time its empty data)
        for i = #fft/3, #fft do
            fft[math.floor(i)] = nil
        end


        ----------- BINNING ------------
        -- pick one binning function to use
        -- and it handles inserting into the values table

        --dlbBins(bars, fft, values, lastframevals)
        --barkBins(bars, fft, values, lastframevals, samplingRate)
        perceptualBins(bars, fft, values, lastframevals, samplingRate, nbins)

        ----------- FINISH AND DISPLAY --------
        smoothZeros(values)
        for i = 1, #bars do
            -- the values are already logarithmd and probably arent much more than 10
            -- if it goes higher then it looks cooler doesnt it? probably not
            local x = values[i] / 10
            updater(bars[i], x)
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
    frame.BeginCommand = function(self)
        screen = SCREENMAN:GetTopScreen()
        SOUND:SetPlayBackCallback(frame.playbackFunction, frame.sampleCount)
    end
    frame.EndCommand = function(self)
        -- triggers if the screen the visualizer is loaded on gets deleted (a transition or exit)
        SOUND:ClearPlayBackCallback()
    end
    frame.ResetVisualizerMessageCommand = function(self)
        screen = SCREENMAN:GetTopScreen()
        SOUND:SetPlayBackCallback(frame.playbackFunction, frame.sampleCount)
    end

    frame[#frame + 1] = frame.sound
    return frame
end
