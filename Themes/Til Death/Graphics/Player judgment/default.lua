-- Removed all the protiming junk, it's obsoleted
local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local c
local enabledJudgment = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgmentText

local JudgeCmds = {
	TapNoteScore_W1 = THEME:GetMetric("Judgment", "JudgmentW1Command"),
	TapNoteScore_W2 = THEME:GetMetric("Judgment", "JudgmentW2Command"),
	TapNoteScore_W3 = THEME:GetMetric("Judgment", "JudgmentW3Command"),
	TapNoteScore_W4 = THEME:GetMetric("Judgment", "JudgmentW4Command"),
	TapNoteScore_W5 = THEME:GetMetric("Judgment", "JudgmentW5Command"),
	TapNoteScore_Miss = THEME:GetMetric("Judgment", "JudgmentMissCommand")
}

local TNSFrames = {
	TapNoteScore_W1 = 0,
	TapNoteScore_W2 = 1,
	TapNoteScore_W3 = 2,
	TapNoteScore_W4 = 3,
	TapNoteScore_W5 = 4,
	TapNoteScore_Miss = 5
}

local function judgmentZoom(value)
    c.Judgment:zoom(value)
    if allowedCustomization then
	    c.Border:playcommand("ChangeWidth", {val = c.Judgment:GetZoomedWidth()})
	    c.Border:playcommand("ChangeHeight", {val = c.Judgment:GetZoomedHeight()})
	end
end

local t =
	Def.ActorFrame {
	Def.Sprite {
		Texture = "../../../../" .. getAssetPath("judgment"),
		Name = "Judgment",
		InitCommand = function(self)
			self:pause():visible(false):xy(MovableValues.JudgeX, MovableValues.JudgeY)
		end,
		ResetCommand = function(self)
			self:finishtweening():stopeffect():visible(false)
		end
	},
	OnCommand = function(self)
		c = self:GetChildren()
		judgmentZoom(MovableValues.JudgeZoom)
		if allowedCustomization then
			Movable.DeviceButton_1.element = c
			Movable.DeviceButton_2.element = c
			Movable.DeviceButton_1.condition = enabledJudgment
			Movable.DeviceButton_2.condition = enabledJudgment
			Movable.DeviceButton_2.DeviceButton_up.arbitraryFunction = judgmentZoom
			Movable.DeviceButton_2.DeviceButton_down.arbitraryFunction = judgmentZoom
			Movable.DeviceButton_1.propertyOffsets = {self:GetTrueX() , self:GetTrueY() - c.Judgment:GetHeight()}	-- centered to screen/valigned
		end
	end,
	JudgmentMessageCommand = function(self, param)
		if param.HoldNoteScore or param.FromReplay then
			return
		end
		local iNumStates = c.Judgment:GetNumStates()
		local iFrame = TNSFrames[param.TapNoteScore]
		if not iFrame then
			return
		end
		if iNumStates == 12 then
			iFrame = iFrame * 2
			if not param.Early then
				iFrame = iFrame + 1
			end
		end

		self:playcommand("Reset")
		c.Judgment:visible(true)
		c.Judgment:setstate(iFrame)
		JudgeCmds[param.TapNoteScore](c.Judgment)
	end,
	MovableBorder(0, 0, 1, MovableValues.JudgeX, MovableValues.JudgeY)
}

if enabledJudgment then
	return t
end

return Def.ActorFrame {}
