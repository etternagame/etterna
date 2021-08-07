local t = Def.ActorFrame {}

t[#t+1] = LoadActor("../_mouse.lua")

-- header
t[#t+1] = LoadActorWithParams("../playerInfoFrame/main.lua", {visualizer = true, screen = "ScreenSelectMusic"})

return t