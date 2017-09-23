local sButton = Var "Button";

return Def.ActorFrame {
	NOTESKIN:LoadActor(Var "Button", "Underlay Receptor") .. {
		OnCommand=function() fReceptor[sButton]:queuecommand("On") end;
		PressCommand=function() fReceptor[sButton]:queuecommand("Press") end;
		LiftCommand=function() fReceptor[sButton]:queuecommand("Lift") end;
	};
	LoadActor( "_Tap Receptor", NOTESKIN:LoadActor(Var "Button", "Ready Receptor") ) .. {
		Frames = {
			{ Frame = 2; Delay = 1; };
		};

		InitCommand=cmd(playcommand, "Set");
		GameplayLeadInChangedMessageCommand=cmd(playcommand,"Set");
		SetCommand=cmd(visible,GAMESTATE:GetGameplayLeadIn());
	};

	LoadActor( "_Tap Receptor", NOTESKIN:LoadActor(Var "Button", "Go Receptor") ) .. {
		Frames = {
			{ Frame = 0; };
			{ Frame = 1; Delay = 0.8; };
			{ Frame = 2; };
		};
	
		InitCommand=cmd(playcommand, "Set");
		GameplayLeadInChangedMessageCommand=cmd(playcommand,"Set");
		SetCommand=cmd(visible,not GAMESTATE:GetGameplayLeadIn());
	};
}
