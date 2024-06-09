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
	Name = "BG",
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


local unhideWidth = 100 / 1920 * SCREEN_WIDTH
local unhideHeight = 55 / 1080 * SCREEN_HEIGHT


local translations = {
    Unhide = THEME:GetString("ScreenTextEntry", "Unhide"),
}


local boxWidth = SCREEN_WIDTH - 250
local boxHeight = 175
local sideMargin = 15 / 1920 * SCREEN_WIDTH
local bottomMargin = 15 / 1080 * SCREEN_HEIGHT

local textsize = 0.86


t[#t+1] = Def.ActorFrame {
	InitCommand = function(self)
		self:Center()
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(function(event)
			if event.type == "InputEventType_FirstPress" then
				if event.DeviceInput.button == "DeviceButton_left mouse button" then
					self:playcommand("PressyMyMouseButton")
				end
			elseif event.type == "InputEventType_Release" then
				if event.DeviceInput.button == "DeviceButton_left mouse button" then
					self:playcommand("ReleaseMouseButton")
				end
			end
		end)
		self:SetUpdateFunction(function(self) self:playcommand("HighlightyMyMouseHovering") end)
	end,
	Def.Quad {
		Name = "UnhideButton",
		InitCommand = function(self)
			self:halign(0):valign(1)
			self:zoomto(unhideWidth, unhideHeight)
			self:xy(-boxWidth/2 + sideMargin, boxHeight/2 - bottomMargin)
			self:diffusealpha(0.5)
			self:visible(false)
			self.isheld = false
		end,
		OnCommand = function(self)
			self:visible(SCREENMAN:GetTopScreen():IsInputHidden())
			if SCREENMAN:GetTopScreen():IsInputHidden() then
				local cc = self:GetParent():GetParent():GetChild("BG")
				cc:finishtweening()
				cc:zoomy(cc:GetZoomY() + 100)
			end
		end,
		PressyMyMouseButtonCommand = function(self)
			if isOver(self) then
				if self.isheld then return end
				self.isheld = true
				SCREENMAN:GetTopScreen():ToggleInputHidden()
			end
		end,
		ReleaseMouseButtonCommand = function(self)
			if self.isheld then
				self.isheld = false
				SCREENMAN:GetTopScreen():ToggleInputHidden()
			end
		end,
		HighlightyMyMouseHoveringCommand = function(self)
			if isOver(self) then
				self:diffusealpha(0.2)
			else
				self:diffusealpha(0.5)
			end
		end
	},
	LoadFont("Common Normal") .. {
		Name = "Unhide",
		InitCommand = function(self)
			self:zoom(textsize)
			self:maxwidth(unhideWidth / textsize)
			self:xy(-boxWidth/2 + sideMargin + unhideWidth/2, boxHeight/2 - bottomMargin - unhideHeight/1.7)
			self:settext(translations["Unhide"])
			self:visible(false)
		end,
		OnCommand = function(self)
			self:visible(SCREENMAN:GetTopScreen():IsInputHidden())
		end,
	}
}

return t
