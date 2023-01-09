local showVisualizer = themeConfig:get_data().global.ShowVisualizer

local function input(event)
	-- mouse click events left here to let anything in selectmusic react to them
	if event.DeviceInput.button == "DeviceButton_left mouse button" then 
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
			MESSAGEMAN:Broadcast("MouseUp", {event = event})
		elseif event.type == "InputEventType_FirstPress" then
			MESSAGEMAN:Broadcast("MouseDown", {event = event})
		end
	elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("MouseRightClick")
			MESSAGEMAN:Broadcast("MouseUp", {event = event})
		elseif event.type == "InputEventType_FirstPress" then
			MESSAGEMAN:Broadcast("MouseDown", {event = event})
		end
	end
	return false
end

local hoverAlpha = 0.6

local t = Def.ActorFrame {
	BeginCommand = function(self)
		local s = SCREENMAN:GetTopScreen()
		s:AddInputCallback(input)
		setenv("NewOptions","Main")
	end
}

t[#t + 1] = Def.Actor {
	CodeMessageCommand = function(self, params)
		if params.Name == "AvatarShow" and getTabIndex() == 0 and not SCREENMAN:get_input_redirected(PLAYER_1) then
			SCREENMAN:SetNewScreen("ScreenAssetSettings")
		end
	end,
	OnCommand = function(self)
		inScreenSelectMusic = true
	end,
	EndCommand = function(self)
		inScreenSelectMusic = nil
	end,
}

t[#t + 1] = LoadActor("../_frame")
t[#t + 1] = LoadActor("../_PlayerInfo")

if showVisualizer then
	local vis = audioVisualizer:new {
		x = 175,
		y = 30,
		maxHeight = 30,
		freqIntervals = audioVisualizer.multiplyIntervals(audioVisualizer.defaultIntervals, 5),
		color = getMainColor("positive"),
		onBarUpdate = function(self)
			--[
			self:diffusetopedge(getMainColor("frames"))
			self:diffusebottomedge(getMainColor("positive"))
			--]]
			--[[
			self:diffuselowerleft()
			self:diffuseupperleft()
			self:diffuselowerright()
			self:diffuseupperright()
			--]]
		end
	}
	t[#t + 1] = vis
end


t[#t + 1] = LoadActor("currentsort")
t[#t + 1] = UIElements.TextToolTip(1, 1, "Common Large") .. {
	Name="rando",
	InitCommand = function(self)
		self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("positive"))
		self:settextf("%s:", THEME:GetString("ScreenSelectMusic", "Title"))
	end,
	MouseOverCommand = function(self)
		self:diffusealpha(hoverAlpha)
	end,
	MouseOutCommand = function(self)
		self:diffusealpha(1)
	end,
	MouseDownCommand = function(self, params)
		if params.event == "DeviceButton_left mouse button" then
			local w = SCREENMAN:GetTopScreen():GetMusicWheel()

			if INPUTFILTER:IsShiftPressed() and self.lastlastrandom ~= nil then

				-- if the last random song wasnt filtered out, we can select it
				-- so end early after jumping to it
				if w:SelectSong(self.lastlastrandom) then
					return
				end
				-- otherwise, just pick a new random song
			end

			local t = w:GetSongs()
			if #t == 0 then return end
			local random_song = t[math.random(#t)]
			w:SelectSong(random_song)
			self.lastlastrandom = self.lastrandom
			self.lastrandom = random_song
		end
	end
}

t[#t + 1] = LoadActor("../_cursor")
t[#t + 1] = LoadActor("../_halppls")

updateDiscordStatusForMenus()
updateNowPlaying()

return t
