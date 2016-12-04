local t = Def.ActorFrame {}

t[#t+1] = Def.ActorFrame {
  InitCommand=cmd(Center),
	LoadActor("woop") .. {
		OnCommand=cmd(zoomto,SCREEN_WIDTH,150;diffusealpha,0;linear,1;diffusealpha,1;sleep,1.75;linear,2;diffusealpha,0)
	},
	Def.ActorFrame {
	  OnCommand=cmd(playcommandonchildren,"ChildrenOn"),
	  ChildrenOnCommand=cmd(diffusealpha,0;sleep,0.5;linear,0.5;diffusealpha,1),
		LoadFont("Common Normal") .. {
			Text=getThemeName(),
			InitCommand=cmd(y,-24),
			OnCommand=cmd(sleep,1;linear,3;diffuse,getDifficultyColor("Difficulty_Couple");diffusealpha,0)
		},
		LoadFont("Common Normal") .. {
			Text="Created by " .. "Irate Platypusaurusean",
			InitCommand=cmd(y,16;zoom,0.75),
			OnCommand=cmd(sleep,1;linear,3;diffuse,getDifficultyColor("Difficulty_Couple");diffusealpha,0)
		},
	}
}

return t
