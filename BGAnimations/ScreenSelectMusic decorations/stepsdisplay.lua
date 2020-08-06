local itsOn = false
local thesteps
local numshown = 5
local currentindex = 1
local displayindexoffset = 0

local ratios = {
    DiffItemWidth = 60 / 1920,
	DiffItemHeight = 40 / 1080,
	DiffFrameUpperGap = 257 / 1080, -- from top edge to top edge
    --DiffFrameLeftGap = 429 / 1920, -- this number is provided by the parent at this time
    DiffFrameRightGap = 11 / 1920,
}

local actuals = {
	DiffItemWidth = ratios.DiffItemWidth * SCREEN_WIDTH,
	DiffItemHeight = ratios.DiffItemHeight * SCREEN_HEIGHT,
	DiffFrameUpperGap = ratios.DiffFrameUpperGap * SCREEN_HEIGHT,
    --DiffFrameLeftGap = ratios.DiffFrameLeftGap * SCREEN_WIDTH, -- this number is provided by the parent at this time
    DiffFrameRightGap = ratios.DiffFrameRightGap * SCREEN_WIDTH,
}

-- scoping magic
do
    -- copying the provided ratios and actuals tables to have access to the sizing for the overall frame
    local rt = Var("ratios")
    for k,v in pairs(rt) do
        ratios[k] = v
    end
    local at = Var("actuals")
    for k,v in pairs(at) do
        actuals[k] = v
    end
end

local textSize = 0.75
local textzoomFudge = 5

local t = Def.ActorFrame {
	Name = "StepsDisplayFile",
	InitCommand = function(self)
		self:xy(actuals.DiffFrameLeftGap, actuals.DiffFrameUpperGap)
	end,
	SetCommand = function(self, params)
		if params.song then
			thesteps = params.song:GetChartsOfCurrentGameMode()
			self:visible(true)
		else
			self:visible(false)
		end
	end
}

local function stepsRows(i)
	local o = Def.ActorFrame {
		Name = "StepsFrame",
		InitCommand = function(self)
			self:x(actuals.DiffItemWidth * (i - 1) + actuals.DiffFrameRightGap * (i - 1))
		end,

		Def.Quad {
			Name = "BG",
			InitCommand = function(self)
				self:halign(0):valign(0)
				self:zoomto(actuals.DiffItemWidth, actuals.DiffItemHeight)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:visible(true)
					local diff = steps:GetDifficulty()
					self:diffuse(getDifficultyColor(diff))
					self:diffusealpha(1)
				else 
					self:visible(false)
				end
			end
		},
		Def.Quad {
			Name = "Lip",
			InitCommand = function(self)
				self:halign(0):valign(0)
				self:y(actuals.DiffItemHeight / 2)
				self:zoomto(actuals.DiffItemWidth, actuals.DiffItemHeight / 2)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:visible(true)
					self:diffuse(color("#111111"))
					self:diffusealpha(0.2)
				else 
					self:visible(false)
				end
			end
		},
		LoadFont("Common Normal") .. {
			Name = "StepsType",
			InitCommand = function(self)
				self:xy(actuals.DiffItemWidth / 2, actuals.DiffItemHeight / 4)
				self:zoom(textSize)
				self:maxwidth(actuals.DiffItemWidth / textSize - textzoomFudge)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then
				local st = THEME:GetString("StepsDisplay StepsType", ToEnumShortString(steps:GetStepsType()))
					self:settext(st)
				else
					self:settext("")
				end
			end
		},
		LoadFont("Common Normal") .. {
			Name = "NameAndMeter",
			InitCommand = function(self)
				self:xy(actuals.DiffItemWidth / 2, actuals.DiffItemHeight / 4 * 3)
				self:maxwidth(actuals.DiffItemWidth / textSize - textzoomFudge)
				self:zoom(textSize)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					local meter = steps:GetMeter()
					local diff = getShortDifficulty(steps:GetDifficulty())
					self:settextf("%s %s", diff, meter)
				else
					self:settext("")
				end
			end
		}

	}

	return o
end

local sdr =
	Def.ActorFrame {
	Name = "StepsRows"
}

for i = 1, numshown do
	sdr[#sdr + 1] = stepsRows(i)
end
t[#t + 1] = sdr

local center = math.ceil(numshown / 2)

t[#t + 1] = Def.Quad {
	Name = "Cursor",
	InitCommand = function(self)
		self:halign(0):valign(0)
		self:y(actuals.DiffItemHeight)
		self:zoomto(actuals.DiffItemWidth, 5)
		self:diffusealpha(0.6)
	end,
	SetCommand = function(self, params)
		for i = 1, 20 do
			if thesteps and thesteps[i] and thesteps[i] == params.song then
				currentindex = i
				break
			end
		end

		if currentindex <= center then
			displayindexoffset = 0
		elseif #thesteps - displayindexoffset > numshown then
			displayindexoffset = currentindex - center
			currentindex = center
		else
			currentindex = currentindex - displayindexoffset
		end

		if #thesteps > numshown and #thesteps - displayindexoffset < numshown then
			displayindexoffset = #thesteps - numshown 
		end

		self:x(actuals.DiffItemWidth * (currentindex - 1) + actuals.DiffFrameRightGap * (currentindex - 1))
		self:GetParent():GetChild("StepsRows"):queuecommand("UpdateStepsRows")
	end
}

return t