
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
	onInit = function(frame)
		local soundActor = frame.sound.actor
		local songs = SONGMAN:GetAllSongs()
		local idx = math.random(1, #songs)
		soundActor:stop()
		soundActor:load(songs[idx]:GetMusicPath())
		soundActor:play()
	end
}
--]]
local defaultIntervals = {}
for i=0,67 do
	defaultIntervals[#defaultIntervals+1] = i*i*5
end
	defaultIntervals[#defaultIntervals+1] = 99999
local toComplex = audioVisualizer.toComplex
local fft = audioVisualizer.fft
function audioVisualizer:new(params)
	local frame = Def.ActorFrame {
		InitCommand = function(self)
		self:xy(params.x or 0, params.y or 0)
		end
	}
	local freqIntervals
	do
		local rawFreqIntervals = params.freqIntervals or defaultIntervals --{40, 70, 100, 300, 500, 1000, 2000, 4000, 9000, 18000, 99999}
		freqIntervals = concat({0}, rawFreqIntervals)
	end
	frame.values = {}
	for i,v in ipairs(freqIntervals) do
		frame.values[i] = 0
	end
	frame.bars = {}
	do
		local prevVals = {}
		local scale = 600
		local maxHeight = 120
		frame.updater = params.barUpdater or function(actor, value)
			local height = 3+ math.min(math.abs(value)/3, scale)
			prevVals[actor] = height
			local finalHeight = (height + (prevVals[height] or 0))/(scale/maxHeight)
			actor:zoomtoheight(finalHeight)
		end
	end
	-- ignore 0-firstFreq, so bars start from value 2, and we start from 3 since we use 2 frequencies per bar (intervals)
	for i=3,#freqIntervals do
		frame[#frame+1] = (params.barBuilder or function() return Def.Quad {
			InitCommand = function(self)
				(frame.bars)[i-2] = self
				local width = (params.width or 300)/( (#freqIntervals-2))
				self:valign(1):x((width+(params.spacing or 1))*(i-2)):diffuse(getMainColor('positive')):zoomtowidth(width)
			end
			} 
		end)()
	end
	local updateAll = function()
		for i,v in ipairs(frame.bars) do
			frame.updater(v, frame.values[i+1])
		end
	end
	local soundActor
	frame.sound = Def.Sound {}
	frame.sound.InitCommand = function(self)
		frame.sound.actor = self
		local rSound = self:get()
		rSound:SetPlayBackCallback(function(fft)
			local values = frame.values
			for i=1,#values do
				values[i] = 0
			end
			do
				function addToBin(magnitude, freq)
					for i=2,#freqIntervals do
						if freq > freqIntervals[i-1] and freq <= freqIntervals[i] then
							values[i-1] = values[i-1] + magnitude
							return
						end
					end
				end
				do
					local samplingRate = rSound:GetSampleRate()
					local count = #fft
					local m = count*2 -- Amount of samples. N samples yield n/2+1 useful fft values
					for i=1,count do
						addToBin(math.sqrt(fft[i]), i * samplingRate /m)
					end
				end
			end
			local updater = frame.updater
			local bars = frame.bars
			for i=1,#bars do
				updater(bars[i], values[i+1])
			end
		end, 4096)
		if params.onInit then params.onInit(frame) end
	end
	frame[#frame+1] = frame.sound
	return frame
end