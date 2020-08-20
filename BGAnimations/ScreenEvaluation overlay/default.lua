local t = Def.ActorFrame {Name = "OverlayFile"}

t[#t+1] = LoadActor("../_mouse.lua", "ScreenEvaluation")

-- header
t[#t+1] = LoadActor("../playerInfoFrame.lua")

-- footer
-- []

return t