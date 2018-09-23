local update = false
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0) -- visible(false)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == 0 then
			self:queuecommand("On")
			update = true
		else
			self:queuecommand("Off")
			update = false
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

t[#t + 1] =
	Def.Banner {
	InitCommand = function(self)
		self:x(10):y(61):halign(0):valign(0):scaletoclipped(
			capWideScale(get43size(384), 384),
			capWideScale(get43size(120), 120)
		)
	end,
	SetMessageCommand = function(self)
		if update then
			local top = SCREENMAN:GetTopScreen()
			if top:GetName() == "ScreenSelectMusic" or top:GetName() == "ScreenNetSelectMusic" then
				local song = GAMESTATE:GetCurrentSong()
				local group = top:GetMusicWheel():GetSelectedSection()
				if song then
					self:LoadFromSong(song)
				elseif group then
					self:LoadFromSongGroup(group)
				end
			end
		end
		self:scaletoclipped(capWideScale(get43size(384), 384), capWideScale(get43size(120), 120))
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}

return t
