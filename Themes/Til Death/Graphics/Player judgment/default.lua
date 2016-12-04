-- Removed all the protiming junk, it's obsoleted
local c
local player = Var "Player"

local JudgeCmds = {
	TapNoteScore_W1 = THEME:GetMetric( "Judgment", "JudgmentW1Command" ),
	TapNoteScore_W2 = THEME:GetMetric( "Judgment", "JudgmentW2Command" ),
	TapNoteScore_W3 = THEME:GetMetric( "Judgment", "JudgmentW3Command" ),
	TapNoteScore_W4 = THEME:GetMetric( "Judgment", "JudgmentW4Command" ),
	TapNoteScore_W5 = THEME:GetMetric( "Judgment", "JudgmentW5Command" ),
	TapNoteScore_Miss = THEME:GetMetric( "Judgment", "JudgmentMissCommand" ),
}

local TNSFrames = {
	TapNoteScore_W1 = 0;
	TapNoteScore_W2 = 1;
	TapNoteScore_W3 = 2;
	TapNoteScore_W4 = 3;
	TapNoteScore_W5 = 4;
	TapNoteScore_Miss = 5;
}

local t = Def.ActorFrame {
	LoadActor(THEME:GetPathG("Judgment","Normal")) .. {
		Name="Judgment",
		InitCommand=cmd(pause;visible,false),
		OnCommand=THEME:GetMetric("Judgment","JudgmentOnCommand"),
		ResetCommand=cmd(finishtweening;stopeffect;visible,false),
	},
	
	InitCommand = function(self)
		c = self:GetChildren()
	end,

	JudgmentMessageCommand=function(self, param)
		if param.HoldNoteScore then return end
		local iNumStates = c.Judgment:GetNumStates()
		local iFrame = TNSFrames[param.TapNoteScore]
		if not iFrame then return end
		
		self:playcommand("Reset")
		c.Judgment:visible(true)
		c.Judgment:setstate(iFrame)
		JudgeCmds[param.TapNoteScore](c.Judgment)
	end
}

if playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).JudgmentText then
	return t
end

return {}


