local t = Def.ActorFrame {}

t[#t+1] = Def.Quad {
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
        self:diffuse(color("0.5,0.1,0.5"))
    end
}

return t