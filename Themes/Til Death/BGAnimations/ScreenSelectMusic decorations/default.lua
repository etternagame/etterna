local t = Def.ActorFrame {}

t[#t + 1] = LoadActor("tabs")
t[#t + 1] = LoadActor("wifetwirl")
t[#t + 1] = LoadActor("msd")
t[#t + 1] = LoadActor("songsearch")
t[#t + 1] = LoadActor("score")
t[#t + 1] = LoadActor("profile")
t[#t + 1] = LoadActor("filter")
t[#t + 1] = LoadActor("goaltracker")
t[#t + 1] = LoadActor("playlists")
t[#t + 1] = LoadActor("downloads")
t[#t + 1] = LoadActor("tags")
t[#t + 1] = LoadActor("stepsdisplay")

t[#t + 1] = LoadActor("../_mousewheelscroll")
collectgarbage()
return t
