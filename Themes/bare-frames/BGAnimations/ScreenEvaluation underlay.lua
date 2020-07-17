local t = Def.ActorFrame {}
-- Controls the lowermost layer of ScreenEvaluation
-- Just making the background full black in case something weird is going on

t[#t+1] = Def.Quad {
    InitCommand = function(self)
        self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
        self:valign(0):halign(0)
        self:diffuse(color("0,0,0,1"))
    end
}

return t