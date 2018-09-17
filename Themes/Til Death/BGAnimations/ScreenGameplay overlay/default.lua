-- Everything relating to the gameplay screen is gradually moved to WifeJudgmentSpotting.lua
local t = Def.ActorFrame {}
t[#t + 1] = LoadActor("WifeJudgmentSpotting")
t[#t + 1] = LoadActor("titlesplash")
if GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerController() == "PlayerController_Replay" then
	t[#t + 1] = LoadActor("replayscrolling")
end
return t
