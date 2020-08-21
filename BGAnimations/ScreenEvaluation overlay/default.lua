local t = Def.ActorFrame {Name = "OverlayFile"}

t[#t+1] = LoadActor("../_mouse.lua", "ScreenEvaluation")

-- header
t[#t+1] = LoadActor("../playerInfoFrame.lua")

-- footer
t[#t+1] = LoadActorWithParams("../footer.lua", {
    Width = SCREEN_WIDTH,
    Height = 50 / 1080 * SCREEN_HEIGHT,
})

return t