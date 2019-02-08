local enabled = themeConfig:get_data().global.SongBGEnabled
local brightness = 0.4
local t = Def.ActorFrame {}

if enabled then
	t[#t + 1] =
		Def.Sprite {
			OnCommand = function(self)
				if GAMESTATE:GetCurrentSong() and GAMESTATE:GetCurrentSong():GetBackgroundPath() then
					self:finishtweening()
					self:visible(true)
					self:LoadBackground(GAMESTATE:GetCurrentSong():GetBackgroundPath())
					self:scaletocover(0, 0, SCREEN_WIDTH, SCREEN_BOTTOM)
					self:sleep(0.5)
					self:decelerate(2)
					self:diffusealpha(brightness)
				else
					self:visible(false)
				end
			end
		}
end

t[#t + 1] =
	Def.Sprite {
	Name = "Banner",
	OnCommand = function(self)
		self:x(SCREEN_CENTER_X):y(38):valign(0)
		self:scaletoclipped(capWideScale(get43size(336), 336), capWideScale(get43size(105), 105))
		local bnpath = GAMESTATE:GetCurrentSong():GetBannerPath()
		if not bnpath then
			bnpath = THEME:GetPathG("Common", "fallback banner")
		end
		self:LoadBackground(bnpath)
	end
}

return t
