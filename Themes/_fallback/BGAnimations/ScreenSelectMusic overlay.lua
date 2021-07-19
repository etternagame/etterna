local t = Def.ActorFrame {
	InitCommand = function(self)
		setenv("NewOptions","Main")
		setenv("DifferentOptionsScreen",false)
	end
}

return t
