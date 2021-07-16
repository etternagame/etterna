return Def.ActorFrame {
	Name = "MusicRate",
	InitCommand = function(self)
		self:xy(MovableValues.MusicRateX, MovableValues.MusicRateY):zoom(MovableValues.MusicRateZoom)
	end,
	LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:zoom(0.35):settext(getCurRateDisplayString())
		end,
		SetRateCommand = function(self)
			self:settext(getCurRateDisplayString())
		end,
		DoneLoadingNextSongMessageCommand = function(self)
			self:playcommand("SetRate")
		end,
		CurrentRateChangedMessageCommand = function(self)
			self:playcommand("SetRate")
		end
	},
}