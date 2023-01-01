local allowedCustomization = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local c
local enabledJudgment = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgmentText
local JTEnabled = JudgementTweensEnabled()

--[[
-- old commands from metrics [Judgment]:
JudgmentW1Command=shadowlength,0;diffusealpha,1;zoom,1.3;linear,0.05;zoom,1;sleep,0.8;smooth,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;glowblink;effectperiod,0.05;effectcolor1,color("1,1,1,0");effectcolor2,color("1,1,1,0.25")
JudgmentW2Command=shadowlength,0;diffusealpha,1;zoom,1.2;linear,0.05;zoom,1;sleep,0.8;smooth,0.1;zoomy,0.5;zoomx,2;diffusealpha,0
JudgmentW3Command=shadowlength,0;diffusealpha,1;zoom,1.2;linear,0.05;zoom,1;sleep,0.8;smooth,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;
JudgmentW4Command=shadowlength,0;diffusealpha,1;zoom,1.1;linear,0.05;zoom,1;sleep,0.8;smooth,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;
JudgmentW5Command=shadowlength,0;diffusealpha,1;zoom,1.05;linear,0.05;zoom,1;sleep,0.8;smooth,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;
JudgmentMissCommand=shadowlength,0;diffusealpha,1;zoom,1.05;linear,0.05;zoom,1;sleep,0.8;smooth,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;
]]

local JudgeCmds = {
	TapNoteScore_W1 = function(self)
		local jz = MovableValues.JudgeZoom
		self:shadowlength(0):diffusealpha(1):zoom(1.3 * jz)
		self:linear(0.05)
		self:zoom(1 * jz)
		self:sleep(0.8):smooth(0.1)
		self:zoomy(0.5 * jz):zoomx(2 * jz):diffusealpha(0)
		self:glowblink():effectperiod(0.05):effectcolor(color("1,1,1,0")):effectcolor2(color("1,1,1,0.25"))
	end,
	TapNoteScore_W2 = function(self)
		local jz = MovableValues.JudgeZoom
		self:shadowlength(0):diffusealpha(1):zoom(1.2 * jz)
		self:linear(0.05)
		self:zoom(1 * jz)
		self:sleep(0.8):smooth(0.1)
		self:zoomy(0.5 * jz):zoomx(2 * jz):diffusealpha(0)
	end,
	TapNoteScore_W3 = function(self)
		local jz = MovableValues.JudgeZoom
		self:shadowlength(0):diffusealpha(1):zoom(1.2 * jz)
		self:linear(0.05)
		self:zoom(1 * jz)
		self:sleep(0.8):smooth(0.1)
		self:zoomy(0.5 * jz):zoomx(2 * jz):diffusealpha(0)
	end,
	TapNoteScore_W4 = function(self)
		local jz = MovableValues.JudgeZoom
		self:shadowlength(0):diffusealpha(1):zoom(1.1 * jz)
		self:linear(0.05)
		self:zoom(1 * jz)
		self:sleep(0.8):smooth(0.1)
		self:zoomy(0.5 * jz):zoomx(2 * jz):diffusealpha(0)
	end,
	TapNoteScore_W5 = function(self)
		local jz = MovableValues.JudgeZoom
		self:shadowlength(0):diffusealpha(1):zoom(1.05 * jz)
		self:linear(0.05)
		self:zoom(1 * jz)
		self:sleep(0.8):smooth(0.1)
		self:zoomy(0.5 * jz):zoomx(2 * jz):diffusealpha(0)
	end,
	TapNoteScore_Miss = function(self)
		local jz = MovableValues.JudgeZoom
		self:shadowlength(0):diffusealpha(1):zoom(1.05 * jz)
		self:linear(0.05)
		self:zoom(1 * jz)
		self:sleep(0.8):smooth(0.1)
		self:zoomy(0.5 * jz):zoomx(2 * jz):diffusealpha(0)
	end,
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

local t = Def.ActorFrame {
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
			Movable.DeviceButton_1.propertyOffsets = {self:GetTrueX() , self:GetTrueY()}	-- centered to screen/valigned
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
		if JTEnabled then
			JudgeCmds[param.TapNoteScore](c.Judgment)
		end
	end,
	MovableBorder(0, 0, 1, MovableValues.JudgeX, MovableValues.JudgeY)
}

if enabledJudgment then
	return t
end

return Def.ActorFrame {}
