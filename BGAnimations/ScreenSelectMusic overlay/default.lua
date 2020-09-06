local t = Def.ActorFrame {}

t[#t+1] = LoadActor("../_mouse.lua", "ScreenSelectMusic")

-- header
t[#t+1] = LoadActorWithParams("../playerInfoFrame.lua", {visualizer = true})

return t