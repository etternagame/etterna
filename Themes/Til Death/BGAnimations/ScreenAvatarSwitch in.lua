return Def.ActorFrame {
	InitCommand = function(self)
		self:visible(false)
		SCREENMAN:SystemMessage("Avatar Switcher Activated")
	end
}
