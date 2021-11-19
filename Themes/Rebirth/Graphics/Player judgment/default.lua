local c
local enabledJudgment = playerConfig:get_data().JudgmentText
local enabledAnimations = playerConfig:get_data().JudgmentTweens

--[[
Removed from metrics and translated to lua here:
JudgmentW1Command=shadowlength,0;diffusealpha,1;zoom,1.3;linear,0.05;zoom,1;sleep,0.8;linear,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;glowblink;effectperiod,0.05;effectcolor1,color("1,1,1,0");effectcolor2,color("1,1,1,0.25")
JudgmentW2Command=shadowlength,0;diffusealpha,1;zoom,1.3;linear,0.05;zoom,1;sleep,0.8;linear,0.1;zoomy,0.5;zoomx,2;diffusealpha,0
JudgmentW3Command=shadowlength,0;diffusealpha,1;zoom,1.2;linear,0.05;zoom,1;sleep,0.8;linear,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;
JudgmentW4Command=shadowlength,0;diffusealpha,1;zoom,1.1;linear,0.05;zoom,1;sleep,0.8;linear,0.1;zoomy,0.5;zoomx,2;diffusealpha,0;
JudgmentW5Command=shadowlength,0;diffusealpha,1;zoom,1.0;vibrate;effectmagnitude,4,8,8;sleep,0.8;linear,0.1;zoomy,0.5;zoomx,2;diffusealpha,0
JudgmentMissCommand=shadowlength,0;diffusealpha,1;zoom,1;linear,0.8;sleep,0.8;linear,0.1;zoomy,0.5;zoomx,2;diffusealpha,0
]]
local JudgeCmds = {
	TapNoteScore_W1 = function(self)
		self:shadowlength(0):diffusealpha(1):zoom(1.3 * MovableValues.JudgmentZoom)
		self:linear(0.05):zoom(1 * MovableValues.JudgmentZoom)
		self:sleep(0.8):linear(0.1)
		self:zoomx(0.5 * MovableValues.JudgmentZoom)
		self:zoomy(2 * MovableValues.JudgmentZoom)
		self:diffusealpha(0)
		self:glowblink():effectperiod(0.05):effectcolor1(color("1,1,1,0")):effectcolor2(color("1,1,1,0.25"))
	end,
	TapNoteScore_W2 = function(self)
		self:shadowlength(0):diffusealpha(1):zoom(1.3 * MovableValues.JudgmentZoom)
		self:linear(0.05):zoom(1 * MovableValues.JudgmentZoom)
		self:sleep(0.8):linear(0.1)
		self:zoomx(0.5 * MovableValues.JudgmentZoom)
		self:zoomy(2 * MovableValues.JudgmentZoom)
		self:diffusealpha(0)
	end,
	TapNoteScore_W3 = function(self)
		self:shadowlength(0):diffusealpha(1):zoom(1.2 * MovableValues.JudgmentZoom)
		self:linear(0.05):zoom(1 * MovableValues.JudgmentZoom)
		self:sleep(0.8):linear(0.1)
		self:zoomx(0.5 * MovableValues.JudgmentZoom)
		self:zoomy(2 * MovableValues.JudgmentZoom)
		self:diffusealpha(0)
	end,
	TapNoteScore_W4 = function(self)
		self:shadowlength(0):diffusealpha(1):zoom(1.1 * MovableValues.JudgmentZoom)
		self:linear(0.05):zoom(1 * MovableValues.JudgmentZoom)
		self:sleep(0.8):linear(0.1)
		self:zoomx(0.5 * MovableValues.JudgmentZoom)
		self:zoomy(2 * MovableValues.JudgmentZoom)
		self:diffusealpha(0)
	end,
	TapNoteScore_W5 = function(self)
		self:shadowlength(0):diffusealpha(1):zoom(1.0 * MovableValues.JudgmentZoom)
		self:vibrate()
		self:effectmagnitude(0.01,0.02,0.02)
		self:sleep(0.8)
		self:linear(0.1)
		self:zoomx(0.5 * MovableValues.JudgmentZoom)
		self:zoomy(2 * MovableValues.JudgmentZoom)
		self:diffusealpha(0)
	end,
	TapNoteScore_Miss = function(self)
		self:shadowlength(0):diffusealpha(1):zoom(1.0 * MovableValues.JudgmentZoom)
		self:linear(0.8)
		self:sleep(0.8):linear(0.1)
		self:zoomx(0.5 * MovableValues.JudgmentZoom)
		self:zoomy(2 * MovableValues.JudgmentZoom)
		self:diffusealpha(0)
	end
}

local TNSFrames = {
    TapNoteScore_W1 = 0,
    TapNoteScore_W2 = 1,
    TapNoteScore_W3 = 2,
    TapNoteScore_W4 = 3,
    TapNoteScore_W5 = 4,
    TapNoteScore_Miss = 5
}

local t = Def.ActorFrame {
    Name = "Judgment", -- c++ renames this to "Judgment"
    BeginCommand = function(self)
        c = self:GetChildren()
        -- queued to run slightly late
        self:queuecommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.JudgmentX, MovableValues.JudgmentY)
        self:zoom(MovableValues.JudgmentZoom)
    end,
    Def.Sprite {
        Texture = "../../../../" .. getAssetPath("judgment"),
        Name = "Judgment",
        InitCommand = function(self)
            self:pause()
            self:visible(false)
        end,
        ResetCommand = function(self)
            self:finishtweening()
            self:stopeffect()
            self:visible(false)
        end,
    },

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
        if enabledAnimations then
            JudgeCmds[param.TapNoteScore](c.Judgment)
        end
    end,
}

if enabledJudgment then
    return t
end

return Def.ActorFrame {}
