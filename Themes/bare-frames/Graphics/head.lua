return LoadFont("Common Normal") .. {
    InitCommand = function(self)
        self:x(SCREEN_CENTER_X):valign(0):zoom(0.5)
        self:y(10)
    end,
    OnCommand = function(self)
        self:settext(SCREENMAN:GetTopScreen():GetName())
    end
}