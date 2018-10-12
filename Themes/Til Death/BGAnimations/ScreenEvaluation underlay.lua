local enabled = themeConfig:get_data().global.SongBGEnabled
local brightness = 0.4
local t = Def.ActorFrame {}

if enabled then
	t[#t + 1] =
		LoadSongBackground() ..
		{
			BeginCommand = function(self)
				self:scaletocover(0, 0, SCREEN_WIDTH, SCREEN_BOTTOM)
				self:diffusealpha(brightness)
			end
		}
end

return t
