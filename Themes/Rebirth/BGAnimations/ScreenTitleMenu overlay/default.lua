local t = Def.ActorFrame {Name = "OverlayFile"}

t[#t+1] = LoadActor("profileSelect")
t[#t+1] = LoadActor(THEME:GetPathG("", "_crashUploadOptIn"))
t[#t+1] = LoadActorWithParams("../_mouse.lua", {waitForMouseMovement = true})
t[#t+1] = EGGMAN.snowyboy()
t[#t+1] = EGGMAN.foole()

return t