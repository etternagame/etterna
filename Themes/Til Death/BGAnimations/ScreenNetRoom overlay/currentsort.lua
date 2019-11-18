local t = Def.ActorFrame {}

local frameWidth = 280
local frameHeight = 20
local frameX = SCREEN_WIDTH - 5
local frameY = 15

t[#t + 1] =
	LoadFont("Common Normal") ..
	{
		InitCommand = function(self)
			self:xy(frameX - 5, frameY):halign(1):zoom(0.3):maxwidth(40 / 0.45)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			local top = SCREENMAN:GetTopScreen()
			if top:GetName() == "ScreenSelectMusic" or top:GetName() == "ScreenNetSelectMusic" then
				local wheel = top:GetMusicWheel()
				self:settextf("%d/%d", wheel:GetCurrentIndex() + 1, wheel:GetNumItems())
			end
		end,
		SortOrderChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end,
		CurrentSongChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

return t
