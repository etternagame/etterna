local gc = Var("GameCommand")

return Def.ActorFrame {
	LoadFont("Common Normal") .. {
		Text=THEME:GetString("ScreenTitleMenu",gc:GetText()),
		OnCommand=cmd(halign,0),
		GainFocusCommand=cmd(zoom,0.6;diffusealpha,1;diffuse,getMainColor('positive')),
		LoseFocusCommand=cmd(diffuse,getMainColor('positive');diffusealpha,0.7;zoom,0.5),
 	}
}

