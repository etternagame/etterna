-- Everything relating to the gameplay screen is gradually moved to WifeJudgmentSpotting.lua
local t = Def.ActorFrame{}
t[#t+1] = LoadActor("WifeJudgmentSpotting")
t[#t+1] = LoadActor("titlesplash")
return t