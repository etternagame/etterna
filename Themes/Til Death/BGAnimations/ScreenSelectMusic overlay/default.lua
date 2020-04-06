local showVisualizer = themeConfig:get_data().global.ShowVisualizer

local function input(event)
	if event.DeviceInput.button == "DeviceButton_left mouse button" and event.type == "InputEventType_Release" then
		MESSAGEMAN:Broadcast("MouseLeftClick")
	elseif event.DeviceInput.button == "DeviceButton_right mouse button" and event.type == "InputEventType_Release" then
		MESSAGEMAN:Broadcast("MouseRightClick")
	end
	return false
end

local function highlight(self)
	self:GetChild("rando"):queuecommand("Highlight")
end

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end

local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:SetUpdateFunction(highlight)
		self:SetUpdateFunctionInterval(0.025)
		local s = SCREENMAN:GetTopScreen()
		s:AddInputCallback(input)
	end
}

t[#t + 1] =
	Def.Actor {
	CodeMessageCommand = function(self, params)
		if params.Name == "AvatarShow" and getTabIndex() == 0 and not SCREENMAN:get_input_redirected(PLAYER_1) then
			SCREENMAN:SetNewScreen("ScreenAssetSettings")
		end
	end
}

t[#t + 1] = LoadActor("../_frame")
t[#t + 1] = LoadActor("../_PlayerInfo")

if showVisualizer then
	local vis =
		audioVisualizer:new {
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
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		Name="rando",
		InitCommand = function(self)
			self:xy(5, 32):halign(0):valign(1):zoom(0.55):diffuse(getMainColor("positive"))
			self:settextf("%s:", THEME:GetString("ScreenSelectMusic", "Title"))
		end,
		HighlightCommand=function(self)
			highlightIfOver(self)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				local w = SCREENMAN:GetTopScreen():GetMusicWheel()
				local t = w:GetSongs()
				if #t == 0 then return end
				local random_song = t[math.random(#t)]
				w:SelectSong(random_song)
			end
		end
	}

t[#t + 1] = LoadActor("../_cursor")
t[#t + 1] = LoadActor("currenttime")
t[#t + 1] = LoadActor("../_halppls")

GAMESTATE:UpdateDiscordMenu(
	GetPlayerOrMachineProfile(PLAYER_1):GetDisplayName() ..
		": " .. string.format("%5.2f", GetPlayerOrMachineProfile(PLAYER_1):GetPlayerRating())
)

File.Write("nowplaying.txt", " ")
return t