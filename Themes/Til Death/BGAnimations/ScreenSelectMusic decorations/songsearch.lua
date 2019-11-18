local searchstring = ""
local frameX = 10
local frameY = 180 + capWideScale(get43size(120), 120)
local active = false
local whee
local lastsearchstring = ""
local instantSearch = themeConfig:get_data().global.InstantSearch

local function searchInput(event)
	if event.type ~= "InputEventType_Release" and active == true then
		if event.button == "Back" then
			searchstring = ""
			whee:SongSearch(searchstring)
			resetTabIndex(0)
			MESSAGEMAN:Broadcast("TabChanged")
			MESSAGEMAN:Broadcast("EndingSearch")
		elseif event.button == "Start" then
			resetTabIndex(0)
			if not instantSearch then
				whee:SongSearch(searchstring)
			end
			MESSAGEMAN:Broadcast("EndingSearch")
			MESSAGEMAN:Broadcast("TabChanged")
		elseif event.DeviceInput.button == "DeviceButton_space" then -- add space to the string
			searchstring = searchstring .. " "
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			searchstring = searchstring:sub(1, -2) -- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete" then
			searchstring = ""
		elseif event.DeviceInput.button == "DeviceButton_=" then
			searchstring = searchstring .. "="
		else
			local CtrlPressed = INPUTFILTER:IsControlPressed()
			if event.DeviceInput.button == "DeviceButton_v" and CtrlPressed then
				searchstring = searchstring .. HOOKS:GetClipboard()
			elseif
			--if not nil and (not a number or (ctrl pressed and not online))
				event.char and event.char:match('[%%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%\'%"%>%<%?%/%~%|%w]') and
					(not tonumber(event.char) or CtrlPressed)
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
	Header = THEME:GetString("TabSearch", "HeaderMessage"),
	Active = THEME:GetString("TabSearch", "Active"),
	Complete = THEME:GetString("TabSearch", "Complete"),
	ExplainStart = THEME:GetString("TabSearch", "ExplainStart"),
	ExplainBack = THEME:GetString("TabSearch", "ExplainBack"),
	ExplainDel = THEME:GetString("TabSearch", "ExplainDelete"),
	ExplainLimit = THEME:GetString("TabSearch", "ExplainLimitation")
}

local t =
	Def.ActorFrame {
	OnCommand = function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		SCREENMAN:GetTopScreen():AddInputCallback(searchInput)
		self:visible(false)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == 3 then
			ms.ok(translated_info["Header"])
			MESSAGEMAN:Broadcast("BeginningSearch")
			self:visible(true)
			active = true
			whee:Move(0)
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			MESSAGEMAN:Broadcast("RefreshSearchResults")
		else
			self:visible(false)
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
				self:xy(frameX + 250 - capWideScale(get43size(120), 30), frameY - 90):zoom(0.7):halign(0.5):maxwidth(470)
			end,
			SetCommand = function(self)
				if active then
					self:settextf("%s:", translated_info["Active"])
					self:diffuse(getGradeColor("Grade_Tier03"))
				else
					self:settextf("%s:", translated_info["Complete"])
					self:diffuse(byJudgment("TapNoteScore_Miss"))
				end
			end,
			UpdateStringMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + 250 - capWideScale(get43size(120), 30), frameY - 50):zoom(0.7):halign(0.5):maxwidth(470)
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
				self:xy(frameX + 20, frameY + 70):zoom(0.5):halign(0)
				self:settext(translated_info["ExplainLimit"])
			end
		}
}

return t
