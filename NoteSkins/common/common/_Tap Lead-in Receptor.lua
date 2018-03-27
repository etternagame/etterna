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

		InitCommand=function(self)
			self:playcommand( "Set")
		end;
		GameplayLeadInChangedMessageCommand=function(self)
			self:playcommand("Set")
		end;
		SetCommand=function(self)
			self:visible(GAMESTATE:GetGameplayLeadIn())
		end;
	};

	LoadActor( "_Tap Receptor", NOTESKIN:LoadActor(Var "Button", "Go Receptor") ) .. {
		Frames = {
			{ Frame = 0; };
			{ Frame = 1; Delay = 0.8; };
			{ Frame = 2; };
		};
	
		InitCommand=function(self)
			self:playcommand( "Set")
		end;
		GameplayLeadInChangedMessageCommand=function(self)
			self:playcommand("Set")
		end;
		SetCommand=function(self)
			self:visible(not GAMESTATE:GetGameplayLeadIn())
		end;
	};
}
