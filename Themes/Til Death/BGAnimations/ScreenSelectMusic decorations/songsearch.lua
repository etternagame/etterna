local searchstring = ""
local frameX = 10
local frameY = 300
local active = false
local whee
local lastsearchstring = ""
local instantSearch = themeConfig:get_data().global.InstantSearch
local IgnoreTabInput = themeConfig:get_data().global.IgnoreTabInput

local function searchInput(event)
	if event.type ~= "InputEventType_Release" and active == true then
		if event.button == "Back" then
			local tind = getTabIndex()
			searchstring = ""
			whee:SongSearch(searchstring)
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = 0})
			MESSAGEMAN:Broadcast("EndingSearch")
		elseif event.button == "Start" then
			local tind = getTabIndex()
			resetTabIndex(0)
			if not instantSearch then
				whee:SongSearch(searchstring)
			end
			MESSAGEMAN:Broadcast("EndingSearch")
			MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = 0})
		elseif event.DeviceInput.button == "DeviceButton_space" then -- add space to the string
			searchstring = searchstring .. " "
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			searchstring = searchstring:sub(1, -2) -- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete" then
			searchstring = ""
		else
			local CtrlPressed = INPUTFILTER:IsControlPressed()
			if event.DeviceInput.button == "DeviceButton_v" and CtrlPressed then
				searchstring = searchstring .. Arch.getClipboard()
			elseif
			--if not nil and (not a number or (ctrl pressed and not online))
				event.char and event.char:match('[%%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%\'%"%>%<%?%/%~%|%w%[%]%{%}%`%\\]') and
					(not tonumber(event.char) or CtrlPressed or IgnoreTabInput > 1)
			 then
				searchstring = searchstring .. event.char
			end
		end
		if lastsearchstring ~= searchstring then
			MESSAGEMAN:Broadcast("UpdateString")
			if instantSearch then
				whee:SongSearch(searchstring)
			end
			lastsearchstring = searchstring
		end
	end
end

local translated_info = {
	Active = THEME:GetString("TabSearch", "Active"),
	Complete = THEME:GetString("TabSearch", "Complete"),
	ExplainStart = THEME:GetString("TabSearch", "ExplainStart"),
	ExplainBack = THEME:GetString("TabSearch", "ExplainBack"),
	ExplainDel = THEME:GetString("TabSearch", "ExplainDelete"),
	ExplainLimit = THEME:GetString("TabSearch", "ExplainLimitation"),
	ExplainNumInput = THEME:GetString("TabSearch", "ExplainNumInput")
}

local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:visible(false)
		self:queuecommand("Set")
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		SCREENMAN:GetTopScreen():AddInputCallback(searchInput)
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
		self:sleep(0.04):queuecommand("Invis")
	end,
	InvisCommand= function(self)
		self:visible(false)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == 3 then
			MESSAGEMAN:Broadcast("BeginningSearch")
			self:visible(true)
			self:queuecommand("On")
			active = true
			whee:Move(0)
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
		else
			self:queuecommand("Off")
			active = false
			SCREENMAN:set_input_redirected(PLAYER_1, false)
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 250 - capWideScale(get43size(95), 10), frameY - 93):zoom(0.7):halign(0.5):maxwidth(470)
			end,
			SetCommand = function(self)
				if active then
					self:settextf("%s:", translated_info["Active"])
					self:diffuse(getGradeColor("Grade_Tier10"))
				elseif not active and searchstring ~= "" then
					self:settext(translated_info["Complete"])
					self:diffuse(getGradeColor("Grade_Tier04"))
				else
					self:settext("")
				end
			end,
			UpdateStringMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX - capWideScale(3.5,-3.5), frameY - 46):zoomto(capWideScale(362.5,472), 44):align(0,0.5):diffuse(getMainColor("tabs"))
		end,
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 250 - capWideScale(get43size(95), 10), frameY - 50):zoom(0.7)
				self:halign(0.5):maxwidth(capWideScale(500,650))
			end,
			SetCommand = function(self)
				self:settext(searchstring)
			end,
			UpdateStringMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 20, frameY - 200):zoom(0.4):halign(0)
				self:settext(translated_info["ExplainStart"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 20, frameY - 175):zoom(0.4):halign(0)
				self:settext(translated_info["ExplainBack"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 20, frameY - 150):zoom(0.4):halign(0)
				self:settext(translated_info["ExplainDel"])
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 20, frameY + 50):zoom(0.5):halign(0)
				self:settext(translated_info["ExplainLimit"])
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 20, frameY + 70):zoom(0.5):align(0,0)
				self:settext(translated_info["ExplainNumInput"])
			end
		}
}

return t
