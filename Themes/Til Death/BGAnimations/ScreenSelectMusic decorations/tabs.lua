local active = true
local numericinputactive = false
local whee

local function IgnoreTabInputs()
	local IgnoreTabInput = themeConfig:get_data().global.IgnoreTabInput
	if IgnoreTabInput == 3 then
		return true
	elseif IgnoreTabInput == 2 and getTabIndex() == 3 then
		return true
	else
		return false
	end
end

local tabNames = {"General", "MSD", "Scores", "Search", "Profile", "Filters", "Goals", "Playlists", "Packs", "Tags"} -- this probably should be in tabmanager.

local function input(event)
	if event.type ~= "InputEventType_Release" and active then
		if numericinputactive == false then
			if
				not (INPUTFILTER:IsBeingPressed("left ctrl") or INPUTFILTER:IsBeingPressed("right ctrl") or
					(SCREENMAN:GetTopScreen():GetName() ~= "ScreenSelectMusic" and
					 SCREENMAN:GetTopScreen():GetName() ~= "ScreenNetSelectMusic"))
			 then
				if event.DeviceInput.button == "DeviceButton_0" then
					local tind = getTabIndex()
					-- if on the search tab dont let them press 0 if currently holding shift...
					-- prevents attempting to enter ')' causing page change
					-- (HACK) (HACK) (HACK) (HACK) (HACK) (HACK) (HACK) (HACK) (HACK) (HACK)
					if tind == 3 and (INPUTFILTER:IsBeingPressed("left shift") or INPUTFILTER:IsBeingPressed("right shift")) then
						return false
					end

					if not IgnoreTabInputs() then
						setTabIndex(9)
						MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = 9})
					end
				else
					for i = 1, #tabNames do
						local numpad = event.DeviceInput.button == "DeviceButton_KP "..event.char	-- explicitly ignore numpad inputs for tab swapping (doesn't care about numlock) -mina
						if not numpad and event.char and tonumber(event.char) and tonumber(event.char) == i and not IgnoreTabInputs() then
							local tind = getTabIndex()
							setTabIndex(i - 1)
							MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = i-1})
						end
					end
				end
			end
		end
	end
	return false
end
local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(MPinput)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
		resetTabIndex()
	end,
	NumericInputActiveMessageCommand = function(self)
		numericinputactive = true
	end,
	NumericInputEndedMessageCommand = function(self)
		numericinputactive = false
	end
}

local frameWidth = capWideScale(get43size(450), 450) / (#tabNames - 1)
local frameX = frameWidth / 2 + 2
local frameY = SCREEN_HEIGHT - 70

function tabs(index)
	local t =
		Def.ActorFrame {
		Name = "Tab" .. index,
		InitCommand = function(self)
			self:xy(frameX + ((index - 1) * frameWidth), frameY)
		end,
		BeginCommand = function(self)
			self:queuecommand("Set")
		end,
		SetCommand = function(self)
			self:finishtweening()
			self:smooth(0.1)
			--show tab if it's the currently selected one
			if getTabIndex() == index - 1 then
				self:diffusealpha(1):y(frameY - 1)
				self:GetChild("TabBG"):diffusecolor(Brightness(getMainColor("positive"),0.3)):diffusealpha(0.5)
			else -- otherwise "Hide" them
				self:diffusealpha(0.7):y(frameY)
				self:GetChild("TabBG"):diffusecolor(getMainColor("frames")):diffusealpha(0.7)
			end
		end,
		TabChangedMessageCommand = function(self)
			self:queuecommand("Set")
		end
	}

	t[#t + 1] =
		Def.Quad {
		Name = "TabBG",
		InitCommand = function(self)
			self:y(2):valign(0):zoomto(frameWidth, 20):diffusecolor(getMainColor("frames")):diffusealpha(0.7)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				local tind = getTabIndex()
				setTabIndex(index - 1)
				MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = index - 1})
			end
		end
	}

	t[#t + 1] =
		LoadFont("Common Normal") ..
		{
			Name = "TabText",
			InitCommand = function(self)
				self:y(4):valign(0):zoom(0.4):diffuse(getMainColor("positive")):maxwidth(frameWidth * 2)
			end,
			BeginCommand = function(self)
				self:queuecommand("Set")
			end,
			SetCommand = function(self)
				self:settext(THEME:GetString("TabNames", tabNames[index]))
				if isTabEnabled(index) then
					if index == 6 and FILTERMAN:AnyActiveFilter() then
						self:diffuse(color("#cc2929"))
					else
						self:diffuse(getMainColor("positive"))
					end
				else
					self:diffuse(color("#666666"))
				end
			end
		}
	return t
end

--Make tabs
for i = 1, #tabNames do
	t[#t + 1] = tabs(i)
end

return t
