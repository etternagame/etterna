local t = Def.ActorFrame {Name = "OverlayFile"}

t[#t+1] = LoadActor("helpDisplay")
t[#t+1] = LoadActor("../_mouse.lua")

return t