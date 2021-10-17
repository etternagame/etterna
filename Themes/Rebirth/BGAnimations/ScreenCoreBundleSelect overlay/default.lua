local t = Def.ActorFrame {Name = "OverlayFile"}

t[#t+1] = LoadActor("bundleDisplay")
t[#t+1] = LoadActor("../_mouse.lua")

return t