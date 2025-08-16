local minidoots = {"Beginner", "Novice", "Intermediate", "Advanced", "Expert"}
local diffcolors = {"#66ccff", "#099948", "#ddaa00", "#ff6666", "#c97bff"}
local packsy
local packspacing = 54
local ind = 7

local translated_info = {
	Alert = THEME:GetString("ScreenCoreBundleSelect", "Alert"),
	Task = THEME:GetString("ScreenCoreBundleSelect", "Task"),
	Explanation = THEME:GetString("ScreenCoreBundleSelect", "Explanation"),
    RefreshSongs = THEME:GetString("GeneralInfo", "DifferentialReloadTrigger")
}

local o = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(SCREEN_WIDTH / 2, 50):halign(0.5)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(function(event)
			if event.type == "InputEventType_FirstPress" then
				if event.button == "MenuUp" then
					ind = ind - 1
					if ind < 1 or ind > 5 then
						ind = 5
					end
					self:queuecommand("SelectionChanged")
				elseif event.button == "MenuDown" then
					ind = ind + 1
					if ind > 5 then
						ind = 1
					end
					self:queuecommand("SelectionChanged")
				elseif event.button == "Start" then
					if ind < 6 and ind > 0 then
						DLMAN:DownloadCoreBundle(minidoots[ind]:lower())
						SCREENMAN:GetTopScreen():StartTransitioningScreen("SM_GoToNextScreen")
					end
				end
			end
			return false
		end)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:y(200):zoomto(500, 500):diffuse(getMainColor("frames")):diffusealpha(1)
		end
	},
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:zoom(0.5)
		end,
		OnCommand = function(self)
			self:settext(translated_info["Alert"])
		end
	},
	LoadFont("Common normal") .. {
		InitCommand = function(self)
			self:y(24):zoom(0.5)
		end,
		OnCommand = function(self)
			self:settext(translated_info["Task"])
		end
	},
	LoadFont("Common normal") .. {
		InitCommand = function(self)
			self:y(330):zoom(0.4)
		end,
		OnCommand = function(self)
			self:settext(translated_info["Explanation"])
		end
	}
}

local function makedoots(i)
	local packinfo
	local t = Def.ActorFrame {
		InitCommand = function(self)
			self:y(packspacing * i)
		end,
		UIElements.QuadButton(1, 1) .. {
			Name = "Doot",
			InitCommand = function(self)
				self:y(-12):zoomto(400, 48):valign(0):diffuse(color(diffcolors[i]))
			end,
			OnCommand = function(self)
				self:queuecommand("SelectionChanged")
			end,
			SelectionChangedCommand = function(self)
				if i == ind then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.5)
				end
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" and ind == i then
					DLMAN:DownloadCoreBundle(minidoots[i]:lower())
					SCREENMAN:GetTopScreen():StartTransitioningScreen("SM_GoToNextScreen")
				elseif isOver(self) then
					ind = i
					self:diffusealpha(1)
				end
			end,
			MouseOverCommand = function(self)
				if ind ~= i then
					self:diffusealpha(0.75)
				else
					self:diffusealpha(0.5)
				end
			end,
			MouseOutCommand = function(self)
				if ind == i then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.5)
				end
			end,
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:zoom(0.5)
			end,
			OnCommand = function(self)
				self:settext(minidoots[i])
			end
		},
		LoadFont("Common normal") .. {
			InitCommand = function(self)
				self:y(24):zoom(0.5)
			end,
			OnCommand = function(self)
				local bundle = DLMAN:GetCoreBundle(minidoots[i]:lower())
				self:settextf("(%dMB)", bundle["TotalSize"])
			end
		}
	}
	return t
end

o[#o + 1] = Def.ActorFrame {
    UIElements.TextToolTip(1, 1, "Common Normal") .. {
		Name = "refreshbutton",
		InitCommand = function(self)
			self:xy(0, 400):zoom(1):diffuse(getMainColor("positive")) -- curse of ra
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:settextf(translated_info["RefreshSongs"])
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(0.6)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(1)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				SONGMAN:DifferentialReload()
                SCREENMAN:GetTopScreen():StartTransitioningScreen("SM_GoToNextScreen")
			end
		end
	}
}

for i = 1, #minidoots do
	o[#o + 1] = makedoots(i)
end

return o
