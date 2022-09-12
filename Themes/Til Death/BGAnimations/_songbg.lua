local enabled = PREFSMAN:GetPreference("ShowBackgrounds")
local brightness = 0.3

local t = Def.ActorFrame {}

-- bg
if enabled then
	t[#t + 1] = Def.Sprite {
		Name = "BGSprite",
		InitCommand = function(self)
			self:diffusealpha(0)
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:stoptweening():smooth(0.5):diffusealpha(0)
			self:sleep(0.2):queuecommand("ModifySongBackground")
		end,
		ModifySongBackgroundCommand = function(self)
			if GAMESTATE:GetCurrentSong() and GAMESTATE:GetCurrentSong():GetBackgroundPath() then
				self:finishtweening()
				self:visible(true)
				self:LoadBackground(GAMESTATE:GetCurrentSong():GetBackgroundPath())
				self:scaletocover(0, 0, SCREEN_WIDTH, SCREEN_BOTTOM)
				self:sleep(0.05)
				self:smooth(0.4):diffusealpha(brightness)
			else
				self:visible(false)
			end
		end,
		OffCommand = function(self)
			self:smooth(0.6):diffusealpha(0)
		end,
	}
end

--black dim behind songwheel text
t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:xy(SCREEN_WIDTH, 0):halign(1):valign(0):zoomto(capWideScale(get43size(350), 350), SCREEN_HEIGHT)
		self:diffuse(0.1,0.1,0.1,0.4)
	end
}
--vertical bar left of songwheel
t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:xy(SCREEN_WIDTH - capWideScale(get43size(350), 350), 0):halign(0):valign(0):zoomto(4, SCREEN_HEIGHT)
		self:diffuse(getMainColor("highlight")):diffusealpha(0.5)
	end
}

t[#t+1] = UIElements.QuadButton(-1000, 1) .. {-- a fullscreen button for right click pausing so your right clicks dont pause accidentally
	InitCommand = function(self)
		self:valign(0):halign(0)
		self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
		self:diffusealpha(0)
	end,
	MouseDownCommand = function(self, params)
		if params.event == "DeviceButton_right mouse button" then
			SCREENMAN:GetTopScreen():PauseSampleMusic()
			MESSAGEMAN:Broadcast("MusicPauseToggled")
		end
	end
}

return t
