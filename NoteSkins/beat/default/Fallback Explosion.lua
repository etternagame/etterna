local t = Def.ActorFrame { };
t[#t+1] = NOTESKIN:LoadActor( Var "Button", "Hold Explosion" ) .. {
	HoldingOnCommand=function(self)
		self:visible(true)
	end;
	HoldingOffCommand=function(self)
		self:visible(false)
	end;
	InitCommand=function(self)
		self:visible(false):finishtweening():blend("BlendMode_Add")
	end;
	Frames = Sprite.LinearFrames(
		NOTESKIN:GetMetricF( "GhostArrowDim", "HoldFrames" ),
		NOTESKIN:GetMetricF( "GhostArrowDim", "HoldSeconds" ) );
};

t[#t+1] = NOTESKIN:LoadActor( Var "Button", "Roll Explosion" ) .. {
	RollOnCommand=function(self)
		self:visible(true)
	end;
	RollOffCommand=function(self)
		self:visible(false)
	end;
	InitCommand=function(self)
		self:visible(false):finishtweening():blend("BlendMode_Add")
	end;
	Frames = Sprite.LinearFrames(
		NOTESKIN:GetMetricF( "GhostArrowDim", "HoldFrames" ),
		NOTESKIN:GetMetricF( "GhostArrowDim", "HoldSeconds" ) );
};

t[#t+1] = NOTESKIN:LoadActor( Var "Button", "Tap Explosion Dim" ) .. {
	InitCommand=function(self)
		self:diffusealpha(0):blend("BlendMode_Add")
	end;
	HeldCommand=function(self)
		self:zoom(1):linear(0.06):zoom(1.1):linear(0.06):diffusealpha(0)
	end;
	ColumnJudgmentCommand=function(self, params)
		if params.TapNoteScore == "TapNoteScore_HitMine" then return; end

		self:finishtweening():loop(0):diffusealpha(1):setstate(0):sleep(self:GetAnimationLengthSeconds()-0.001):diffusealpha(0);
	end;
	Frames = Sprite.LinearFrames(
		NOTESKIN:GetMetricF( "GhostArrowDim", "JudgmentFrames" ),
		NOTESKIN:GetMetricF( "GhostArrowDim", "JudgmentSeconds" ) );
};

local mine = NOTESKIN:LoadActor( Var "Button", "HitMine Explosion" ) .. {
	InitCommand=function(self)
		self:diffusealpha(0)
	end;
	Frames = Sprite.LinearFrames(
		NOTESKIN:GetMetricF( "GhostArrowDim", "MineFrames" ),
		NOTESKIN:GetMetricF( "GhostArrowDim", "MineSeconds" ) );
};

local Next = "1";
t[#t+1] = Def.ActorFrame {
	mine .. { Name="1"; };
	mine .. { Name="2"; };

	ColumnJudgmentCommand=function(self, params)
		if params.TapNoteScore ~= "TapNoteScore_HitMine" then return; end

		local c = self:GetChild(Next);
		Next = Next == "1" and "2" or "1";
		c:stoptweening():setstate(0):diffusealpha(1):sleep(self:GetAnimationLengthSeconds()-0.001):diffusealpha(0);
	end;
};

return t;
