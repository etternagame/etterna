-- load this before screenfilter gets sized so it's right the first time
local modslevel = topscreen  == "ScreenEditOptions" and "ModsLevel_Stage" or "ModsLevel_Preferred"
local playeroptions = GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptions(modslevel)
playeroptions:Mini( 2 - playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).ReceptorSize/50 )

local t = Def.ActorFrame{}
t[#t+1] = LoadActor("bg")
t[#t+1] = LoadActor("ScreenFilter")
--t[#t+1] = LoadActor("cbHighlight")
--t[#t+1] = LoadActor("SpeedChange")
--t[#t+1] = LoadActor("pause")
return t --hurr almost everything here is useless