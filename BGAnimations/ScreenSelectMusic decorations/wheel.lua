local t = Def.ActorFrame {Name = "WheelFile"}
ms.ok(1)
t[#t+1] = Def.ActorFrame {
    Name = "WheelContainer",
    InitCommand = function(self)
        self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
        SCREENMAN:set_input_redirected(PLAYER_1, true)
    end,
    OnCommand = function(self)
    end,
    MusicWheel:new()
}





return t