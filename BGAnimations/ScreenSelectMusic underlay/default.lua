local t = Def.ActorFrame {
    Name = "UnderlayFile",
    WheelSettledMessageCommand = function(self, params)
        -- cascade visual update to everything
        self:playcommand("Set", {song = params.song, group = params.group, hovered = params.hovered, steps = params.steps})
    end
}

t[#t+1] = Def.Sprite {
    Name = "BG",
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:scaletocover(0, 0, SCREEN_WIDTH, SCREEN_BOTTOM)
        self:diffusealpha(0)
    end,
    SetCommand = function(self, params)
        self:finishtweening()
        if params.song and params.song:GetBackgroundPath() then
            self:visible(true)
            self:LoadBackground(params.song:GetBackgroundPath())
            self:scaletocover(0, 0, SCREEN_WIDTH, SCREEN_BOTTOM)
            self:smooth(0.5)
            self:diffusealpha(0.3)
        else
            self:visible(false)
        end
    end
}

t[#t+1] = Def.Quad {
    Name = "AverageColor",
    InitCommand = function(self)
        self:valign(0):halign(0)
        self:zoomto(SCREEN_WIDTH, SCREEN_BOTTOM)
        self:diffuse(color("0,0,0"))
        self:diffusealpha(0)
    end,
    SetCommand = function(self, params)
        self:finishtweening()
        if params.song and params.song:GetBackgroundPath() then
            self:visible(false)
        end
    end,
    SetAverageColorMessageCommand = function(self, params)
        self:finishtweening()
        local bn = params.actor
        if bn:GetVisible() then
            self:visible(true)
            local c = bn:GetTexture():GetAverageColor(14)
            self:diffuse(c)
            self:diffusealpha(0.7)
        else
            self:visible(false)
        end
    end
}

return t