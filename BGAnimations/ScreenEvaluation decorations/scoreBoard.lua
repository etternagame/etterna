local t = Def.ActorFrame {
    Name = "ScoreBoardFrame",
}

local ratios = {
    VerticalDividerLeftGap = 131 / 1920, -- from beginning of frame to left edge of divider
    VerticalDividerUpperGap = 29 / 1080, -- from top of frame to top of divider
    VerticalDividerLength = 250 / 1080,
}

local actuals = {
    VerticalDividerLeftGap = ratios.VerticalDividerLeftGap * SCREEN_WIDTH,
    VerticalDividerUpperGap = ratios.VerticalDividerUpperGap * SCREEN_HEIGHT,
    VerticalDividerLength = ratios.VerticalDividerLength * SCREEN_HEIGHT,
}

-- we are expecting this file to be loaded with these params precalculated
-- in reference, Height is measured divider edge to divider edge
-- in reference, Width is the length of the horizontal divider
actuals.FrameWidth = Var("Width")
actuals.FrameHeight = Var("Height")
actuals.DividerThickness = Var("DividerThickness")


t[#t+1] = Def.Quad {
    Name = "VerticalDivider",
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:zoomto(actuals.DividerThickness, actuals.VerticalDividerLength)
        self:xy(actuals.VerticalDividerLeftGap, actuals.VerticalDividerUpperGap)
    end
}


return t