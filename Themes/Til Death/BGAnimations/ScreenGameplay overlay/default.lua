-- Everything relating to the gameplay screen is gradually moved to WifeJudgmentSpotting.lua
local inReplay = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerController() == "PlayerController_Replay"
local inCustomize = playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CustomizeGameplay
local isPractice = GAMESTATE:IsPracticeMode()

if not inReplay and not inCustomize and not isPractice then
	HOOKS:ShowCursor(false)
end

local t = Def.ActorFrame {}
t[#t + 1] = LoadActor("WifeJudgmentSpotting")
t[#t + 1] = LoadActor("titlesplash")
t[#t + 1] = LoadActor("leaderboard")
if inReplay then
	t[#t + 1] = LoadActor("replayscrolling")
end
if inCustomize then
	t[#t + 1] = LoadActor("messagebox")
end
if inReplay or inCustomize or isPractice then
	t[#t + 1] = LoadActor("../_cursor")
end
return t
