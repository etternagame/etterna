local t = Def.ActorFrame {
    Name = "DisplayMean",
    InitCommand = function(self)
        self:zoom(MovableValues.DisplayMeanZoom):x(MovableValues.DisplayMeanX):y(MovableValues.DisplayMeanY)
    end,
    Def.Quad {
        InitCommand = function(self)
            self:zoomto(60, 13):diffuse(color("0,0,0,0.4")):halign(1):valign(0)
        end
    },
    -- Displays your current mean
    LoadFont("Common Large") ..
        {
            Name = "DisplayMeanText",
            InitCommand = function(self)
                self:zoom(0.3):halign(1):valign(0)
            end,
            OnCommand = function(self)
                self:settextf("%5.2fms", 0)
            end,
            SpottedOffsetCommand = function(self)
                self:settextf("%5.2fms", curMeanSum / curMeanCount)
            end
        },
}

return t