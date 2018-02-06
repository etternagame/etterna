local t = Def.ActorFrame{
		LoadActor( "toasty.png" )..{
			InitCommand=function(self)
				self:xy(SCREEN_WIDTH+100,SCREEN_CENTER_Y)
			end,
			StartTransitioningCommand=function(self)
				self:decelerate(0.25):x(SCREEN_WIDTH-100):sleep(1.75):accelerate(0.5):x(SCREEN_WIDTH+100)
			end	
			},
		LoadActor ( "toasty.ogg" )..{StartTransitioningCommand=function(self)
			self:play()
		end}
	}
return t