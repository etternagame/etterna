local itsOn = false
local thesteps

local rowwidth = 60
local rowheight = 17
local cursorwidth = 6
local cursorheight = 17

local numshown = 5
local currentindex = 1
local displayindexoffset = 0

local ratios = {
    DiffItemWidth = 60 / 1920,
    DiffItemHeight = 40 / 1080,
    DiffFrameLeftGap = 429 / 1920,
    DiffFrameRightGap = 11 / 1920,
}

local actuals = {
	DiffItemWidth = ratios.DiffItemWidth * SCREEN_WIDTH,
    DiffItemHeight = ratios.DiffItemHeight * SCREEN_HEIGHT,
    DiffFrameLeftGap = ratios.DiffFrameLeftGap * SCREEN_WIDTH,
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

local t = Def.ActorFrame {
	Name = "StepsDisplayFile",
	SetCommand = function(self, params)
		if params.song then
			thesteps = params.song:GetChartsOfCurrentGameMode()
			self:visible(true)
			self:playcommand("Set", {song = params.song})
		else
			self:visible(false)
		end
	end
}

local function stepsRows(i)
	local o = Def.ActorFrame {
		InitCommand = function(self)
			self:y(rowheight * (i - 1))
		end,
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(rowwidth, rowheight):halign(0)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:visible(true)
					local diff = steps:GetDifficulty()
					self:diffuse(getDifficultyColor(diff))
					self:diffusealpha(0.4)
				else 
					self:visible(false)
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps and isOver(self) then
					SCREENMAN:GetTopScreen():ChangeSteps(i - currentindex)
					SCREENMAN:GetTopScreen():ChangeSteps(0)
				end
			end
		},
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(24, rowheight):halign(0)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:visible(true)
					local diff = steps:GetDifficulty()
					self:diffuse(byDifficulty(diff))					
				else 
					self:visible(false)
				end
			end
		},
		-- Chart defined "Meter" value, not msd (useful to have this for reference)
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(rowwidth - cursorwidth - 5):addy(-1):zoom(0.35):settext(""):halign(1)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then 
					self:settext(steps:GetMeter())
				else
					self:settext("")
				end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(12):zoom(0.2):settext(""):halign(0.5):valign(0)
			end,
			UpdateStepsRowsCommand = function(self)
				local steps = thesteps[i + displayindexoffset]
				if steps then
				local diff = steps:GetDifficulty()
					self:settext(getShortDifficulty(diff))
				else
					self:settext("")
				end
			end
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:x(12):addy(-9):zoom(0.2):settext(""):halign(0.5):valign(0):maxwidth(20 / 0.2)
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
-- cursor goes on top
t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:x(rowwidth):zoomto(cursorwidth, cursorheight):halign(1):valign(0.5):diffusealpha(0.6)
	end,
	CurrentStepsChangedMessageCommand = function(self, steps)
		for i = 1, 20 do
			if thesteps and thesteps[i] and thesteps[i] == steps.ptr then
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

		self:y(cursorheight * (currentindex - 1))
		self:GetParent():GetChild("StepsRows"):queuecommand("UpdateStepsRows")
	end
}

return t