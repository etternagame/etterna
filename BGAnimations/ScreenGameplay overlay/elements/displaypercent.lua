local t = Def.ActorFrame {
	Name = "DisplayPercent",
	InitCommand = function(self)
		self:zoom(MovableValues.DisplayPercentZoom):x(MovableValues.DisplayPercentX):y(MovableValues.DisplayPercentY)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(60, 13):diffuse(color("0,0,0,0.4")):halign(1):valign(0)
		end
	},
	LoadFont("Common Large") .. {
        Name = "DisplayPercent",
        InitCommand = function(self)
            self:zoom(0.3):halign(1):valign(0)
        end,
        OnCommand = function(self)
            self:settextf("%05.2f%%", 0)
        end,
        SpottedOffsetCommand = function(self)
            self:settextf("%05.2f%%", wifey)
        end
    },
}

return t