local t = Def.ActorFrame {}

t[#t+1] = LoadActor("wheel")

t[#t+1] = Def.ActorFrame {
    Name = "RightFrame",
    GeneralTabSetMessageCommand = function(self)
        self:finishtweening()
        self:smooth(0.1)
        self:x(0)
    end,
    PlayerInfoFrameTabSetMessageCommand = function(self)
        self:finishtweening()
        self:smooth(0.1)
        self:x(SCREEN_WIDTH)
    end,

    LoadActor("curSongBox"),
    LoadActor("generalBox"),
}

return t