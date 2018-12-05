return Def.BPMDisplay {
	Name = "BPMDisplay",
	InitCommand = function(self)
		self:horizalign(left):zoom(0.50)
	end,
	SetCommand = function(self)
		self:SetFromGameState()
		if getTabIndex() == 0 then
			self:diffusealpha(1)
		else
			self:diffusealpha(0)
		end
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}
