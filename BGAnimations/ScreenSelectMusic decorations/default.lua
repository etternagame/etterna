local t = Def.ActorFrame {}

t[#t+1] = LoadActor("wheel")

t[#t+1] = Def.ActorFrame {
    Name = "RightFrame",
    LoadActor("curSongBox"),
    LoadActor("generalBox"),
}

return t