local t = Def.ActorFrame {
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(function(event)
			if event.DeviceInput.button == "DeviceButton_right mouse button" then
				SCREENMAN:GetTopScreen():Cancel()
			end
		end)
	end,
}

t[#t + 1] = LoadActor(THEME:GetPathG("", "_OptionsScreen")) .. {
	OnCommand = function(self)
		self:FullScreen():zoom(1):visible(0):diffusealpha(0)
		if not inScreenSelectMusic then
			self:visible(1):decelerate(0.1):diffusealpha(1)
		end
	end
}

t[#t+1] = Def.Quad {
	InitCommand = function(self)
		local profileName = GetPlayerOrMachineProfile(PLAYER_1):GetDisplayName()
		if profileName == "Default Profile" or profileName == "" then
			self:zoomto(SCREEN_WIDTH-250,110):Center():addy(-18)
		else
			self:zoomto(SCREEN_WIDTH-250,90):Center():addy(-8)
		end
		self:diffuse(0,0,0,0)
	end,
	OnCommand = function(self)
		if inScreenSelectMusic then
			self:decelerate(0.1):diffusealpha(0.8)
		end
	end,
	OffCommand = function(self)
		self:decelerate(0.1):diffusealpha(0)
	end
}

return t
