-- the literal background is handled by the c++
-- so here we micromanage underlay layer stuff

-- also permamirror and receptorsize/mini because this is early in gameplay init again
local modslevel = "ModsLevel_Preferred"
local playeroptions = GAMESTATE:GetPlayerState():GetPlayerOptions(modslevel)
playeroptions:Mini(2 - playerConfig:get_data().ReceptorSize / 50)
local profile = PROFILEMAN:GetProfile(PLAYER_1)
local replaystate = GAMESTATE:GetPlayerState():GetPlayerController() == "PlayerController_Replay"
if profile:IsCurrentChartPermamirror() and not replaystate then -- turn on mirror if song is flagged as perma mirror
	playeroptions:Mirror(true)
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