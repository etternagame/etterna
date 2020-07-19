-- Controls the BPMDisplay if loaded on ScreenSelectMusic

return Def.BPMDisplay {
	Name = "BPMDisplay",
	InitCommand = function(self)
		self:horizalign(left):zoom(0.50)
	end,
	SetCommand = function(self)
		self:SetFromGameState()
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}
