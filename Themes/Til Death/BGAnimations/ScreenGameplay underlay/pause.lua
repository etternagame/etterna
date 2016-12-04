local t = Def.ActorFrame{
	Name="SpeedChange";
	CodeMessageCommand = function(self, params)
		if params.Name == "Pause" then
			pauseGame()
		end;
	end
}

return t