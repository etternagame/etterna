local t = Def.ActorFrame{
		LoadActor( "toasty.png" )..{
			InitCommand=cmd(xy,SCREEN_WIDTH+100,SCREEN_CENTER_Y),
			StartTransitioningCommand=cmd(decelerate,0.25;x,SCREEN_WIDTH-100;sleep,1.75;accelerate,0.5;x,SCREEN_WIDTH+100)
			},
		LoadActor ( "toasty.ogg" )..{StartTransitioningCommand=cmd(play)}
	}
return t