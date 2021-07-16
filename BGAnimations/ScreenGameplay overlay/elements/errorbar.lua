local barcount = playerConfig:get_data().ErrorBarCount -- Number of bars. Older bars will refresh if judgments/barDuration exceeds this value.
local barWidth = 2 -- Width of the ticks.
local barDuration = 0.75 -- Time duration in seconds before the ticks fade out. Doesn't need to be higher than 1. Maybe if you have 300 bars I guess.
if barcount > 50 then barDuration = barcount / 50 end -- just procedurally set the duration if we pass 50 bars
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--
local currentbar = 1 -- so we know which error bar we need to update
local ingots = {} -- references to the error bars
local alpha = 0.07 -- ewma alpha
local avg
local lastAvg

-- Makes the error bars. They position themselves relative to the center of the screen based on your dv and diffuse to your judgment value before disappating or refreshing
local function smeltErrorBar(index)
	return Def.Quad {
		Name = index,
		InitCommand = function(self)
			self:xy(MovableValues.ErrorBarX, MovableValues.ErrorBarY):zoomto(barWidth, MovableValues.ErrorBarHeight):diffusealpha(0)
		end,
		UpdateErrorBarCommand = function(self)
			self:hurrytweening(0.1)
			self:diffusealpha(1)
			self:diffuse(jcT[jdgCur])
			if MovableValues and MovableValues.ErrorBarX then
				self:x(MovableValues.ErrorBarX + dvCur * wscale)
				self:y(MovableValues.ErrorBarY)
				self:zoomtoheight(MovableValues.ErrorBarHeight)
			end
			self:linear(barDuration)
			self:diffusealpha(0)
		end,
		PracticeModeResetMessageCommand = function(self)
			self:diffusealpha(0)
		end
	}
end

local e = Def.ActorFrame {
	Name = "ErrorBar",
	InitCommand = function(self)
		if enabledErrorBar == 1 then
			for i = 1, barcount do
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
				ingots[currentbar]:playcommand("UpdateErrorBar") -- Update the next bar in the queue
			end
		end
	end,
	DootCommand = function(self)
		self:RemoveChild("DestroyMe")
		self:RemoveChild("DestroyMe2")

		if enabledErrorBar == 1 then
			self:RemoveChild("WeightedBar")
		end
	end,
	Def.Quad {
		Name = "WeightedBar",
		InitCommand = function(self)
			if enabledErrorBar == 2 then
				self:xy(MovableValues.ErrorBarX, MovableValues.ErrorBarY):zoomto(barWidth, MovableValues.ErrorBarHeight):diffusealpha(1):diffuse(getMainColor("enabled"))
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
			self:diffuse(getMainColor("highlight")):xy(MovableValues.ErrorBarX, MovableValues.ErrorBarY):zoomto(2, MovableValues.ErrorBarHeight)
		end
	},
	-- Indicates which side is which (early/late) These should be destroyed after the song starts.
	LoadFont("Common Normal") .. {
        Name = "DestroyMe",
        InitCommand = function(self)
            self:xy(MovableValues.ErrorBarX + errorBarFrameWidth / 4, MovableValues.ErrorBarY):zoom(0.35)
        end,
        BeginCommand = function(self)
            self:settext(translated_info["ErrorLate"])
            self:diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0)
        end
    },
	LoadFont("Common Normal") .. {
        Name = "DestroyMe2",
        InitCommand = function(self)
            self:xy(MovableValues.ErrorBarX - errorBarFrameWidth / 4, MovableValues.ErrorBarY):zoom(0.35)
        end,
        BeginCommand = function(self)
            self:settext(translated_info["ErrorEarly"])
            self:diffusealpha(0):smooth(0.5):diffusealpha(0.5):sleep(1.5):smooth(0.5):diffusealpha(0):queuecommand("Doot")
        end,
        DootCommand = function(self)
            self:GetParent():queuecommand("Doot")
        end
    },
}

if enabledErrorBar == 1 then
	for i = 1, barcount do
		t[#t+1] = smeltErrorBar(i)
	end
end

return t