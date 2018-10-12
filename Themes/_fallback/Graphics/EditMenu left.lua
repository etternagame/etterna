return Def.ActorFrame {
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(10, 10):y(-5):skewx(-1)
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(10, 10):y(5):skewx(1)
		end
	}
}
