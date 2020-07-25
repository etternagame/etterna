local t = Def.ActorFrame {Name = "WheelFile"}
ms.ok(1)
t[#t+1] = Def.ActorFrame {
    Name = "WheelContainer",
    InitCommand = function(self)
        self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
    end,
    MusicWheel:new()
}





return t