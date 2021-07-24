-- the wifepercent display. it displays the wifepercent

return Def.ActorFrame {
	Name = "DisplayPercent",
	InitCommand = function(self)
        self:xy(MovableValues.DisplayPercentX, MovableValues.DisplayPercentY)
		self:zoom(MovableValues.DisplayPercentZoom)
	end,

	Def.Quad {
        Name = "PercentBacking",
		InitCommand = function(self)
            self:halign(1):valign(0)
			self:zoomto(60, 13)
            self:diffuse(color("0,0,0,0.4"))
		end
	},
	LoadFont("Common Large") .. {
        Name = "DisplayPercent",
        InitCommand = function(self)
            self:halign(1):valign(0)
            self:zoom(0.3)
        end,
        BeginCommand = function(self)
            self:settextf("%05.2f%%", 0)
        end,
        SpottedOffsetCommand = function(self, params)
            if params.wifePercent ~= nil then
                self:settextf("%05.2f%%", wifey)
            else
                self:settextf("%05.2f%%", 0)
            end
        end
    },
}