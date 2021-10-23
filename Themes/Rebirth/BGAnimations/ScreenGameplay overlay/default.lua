local t = Def.ActorFrame {Name = "GameplayOverlayDefaultFile"}

t[#t+1] = LoadActor("elements")
t[#t+1] = LoadActor("titlesplash")

return t