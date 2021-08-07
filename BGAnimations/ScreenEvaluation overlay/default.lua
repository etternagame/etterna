local t = Def.ActorFrame {Name = "OverlayFile"}

t[#t+1] = LoadActor("../_mouse.lua")

-- header
t[#t+1] = LoadActorWithParams("../playerInfoFrame/main.lua", {screen = "ScreenEvaluationNormal"})

-- footer
t[#t+1] = LoadActorWithParams("../footer.lua", {
    Width = SCREEN_WIDTH,
    Height = 50 / 1080 * SCREEN_HEIGHT,
})

return t