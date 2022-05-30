-- the literal background is handled by the c++
-- so here we micromanage underlay layer stuff

-- reset context manager as early as possible in the gameplay init process
-- this should be a safe place to do it, between all context manager registrations (if they take place)
CONTEXTMAN:Reset()

-- also permamirror and receptorsize/mini because this is early in gameplay init again
local modslevel = "ModsLevel_Preferred"
local playeroptions = GAMESTATE:GetPlayerState():GetPlayerOptions(modslevel)
local profile = PROFILEMAN:GetProfile(PLAYER_1)
local replaystate = GAMESTATE:GetPlayerState():GetPlayerController() == "PlayerController_Replay"

-- turn on mirror if song is flagged as perma mirror
if profile:IsCurrentChartPermamirror() and not replaystate then
	playeroptions:Mirror(true)
end

-- dont apply the player defined receptor size mini if viewing an emulated replay
local emulating = PREFSMAN:GetPreference("ReplaysUseScoreMods") and replaystate
if not emulating then
	playeroptions:Mini(2 - playerConfig:get_data().ReceptorSize / 50)
end

-- we can set staticbg prefs here and somehow that works out to be early enough to matter
local staticbg = themeConfig:get_data().global.StaticBackgrounds
local songoptions = GAMESTATE:GetSongOptionsObject("ModsLevel_Preferred")
if staticbg then
	songoptions:StaticBackground(true)
	songoptions:RandomBGOnly(false)
else
	songoptions:StaticBackground(false)
	songoptions:RandomBGOnly(false)
end

local showbgs = themeConfig:get_data().global.ShowBackgrounds
if not showbgs then
	return Def.ActorFrame {
		Def.Quad {
			Name = "SCUFFEDBACKGROUND",
			InitCommand = function(self)
				self:valign(0):halign(0)
				self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
				self:diffuse(color("#000000"))
				self:diffusealpha(1)
			end,
		},
	}
end

return Def.ActorFrame {}