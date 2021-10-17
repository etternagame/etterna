-- meant to define the outward screen transition for a screen
-- this is a full screen fade into black (0.1 seconds long) after waiting 0.1 seconds
return Def.Quad {
    InitCommand = function(self)
        self:FullScreen():diffuse(color("#00000000"))
    end,
    OnCommand = function(self)
        self:sleep(0.1):linear(0.1):diffusealpha(1)
    end
}
