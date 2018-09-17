return LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(0, 10):halign(0):zoom(0.4)
		end,
		BeginCommand = function(self)
			self:settext("AG")
		end
	}
