local numbershers = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}
local frameX = 10
local frameY = 45
local active = false
local whee
local spacingY = 20
local textzoom = 0.35
local ActiveSS = 0
local SSQuery = {}
SSQuery[0] = {}
SSQuery[1] = {}
local frameWidth = capWideScale(360, 400)
local frameHeight = 350
local offsetX = 10
local offsetY = 20
local activebound = 0
for i = 1, #ms.SkillSets + 1 do
	SSQuery[0][i] = "0"
	SSQuery[1][i] = "0"
end

local function FilterInput(event)
	if event.type ~= "InputEventType_Release" and ActiveSS > 0 and active then
		local shouldUpdate = false
		if event.button == "Start" or event.button == "Back" then
			ActiveSS = 0
			MESSAGEMAN:Broadcast("NumericInputEnded")
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			SSQuery[activebound][ActiveSS] = SSQuery[activebound][ActiveSS]:sub(1, -2)
			shouldUpdate = true
		elseif event.DeviceInput.button == "DeviceButton_delete" then
			SSQuery[activebound][ActiveSS] = ""
			shouldUpdate = true
		else
			for i = 1, #numbershers do
				if event.DeviceInput.button == "DeviceButton_" .. numbershers[i] then
					shouldUpdate = true
					if SSQuery[activebound][ActiveSS] == "0" then
						SSQuery[activebound][ActiveSS] = ""
					end
					SSQuery[activebound][ActiveSS] = SSQuery[activebound][ActiveSS] .. numbershers[i]
					if (ActiveSS < #ms.SkillSets + 1 and #SSQuery[activebound][ActiveSS] > 2) or #SSQuery[activebound][ActiveSS] > 3 then
						SSQuery[activebound][ActiveSS] = numbershers[i]
					end
				end
			end
		end
		if SSQuery[activebound][ActiveSS] == "" then
			shouldUpdate = true
			SSQuery[activebound][ActiveSS] = "0"
		end
		if shouldUpdate then
			FILTERMAN:SetSSFilter(tonumber(SSQuery[activebound][ActiveSS]), ActiveSS, activebound)
			whee:SongSearch("") -- stupid workaround?
			MESSAGEMAN:Broadcast("UpdateFilter")
		end
	end
end

local translated_info = {
	Mode = THEME:GetString("TabFilter", "Mode"),
	HighestOnly = THEME:GetString("TabFilter", "HighestOnly"),
	HighestDifficultyOnly = THEME:GetString("TabFilter", "HighestDifficultyOnly"),
	On = THEME:GetString("OptionNames", "On"),
	Off = THEME:GetString("OptionNames", "Off"),
	Matches = THEME:GetString("TabFilter", "Matches"),
	CommonPackFilter = THEME:GetString("TabFilter", "CommonPackFilter"),
	Length = THEME:GetString("TabFilter", "Length"),
	AND = THEME:GetString("TabFilter", "AND"),
	OR = THEME:GetString("TabFilter", "OR"),
	ExplainStartInput = THEME:GetString("TabFilter", "ExplainStartInput"),
	ExplainCancelInput = THEME:GetString("TabFilter", "ExplainCancelInput"),
	ExplainGrey = THEME:GetString("TabFilter", "ExplainGrey"),
	ExplainBounds = THEME:GetString("TabFilter", "ExplainBounds"),
	ExplainHighest = THEME:GetString("TabFilter", "ExplainHighest"),
	ExplainHighestDifficulty = THEME:GetString("TabFilter", "ExplainHighestDifficulty"),
	MaxRate = THEME:GetString("TabFilter", "MaxRate"),
	Title = THEME:GetString("TabFilter", "Title"),
	MinRate = THEME:GetString("TabFilter", "MinRate")
}

local f =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX, frameY):halign(0)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(frameWidth, frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
		end
	},
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(frameWidth, offsetY):halign(0):valign(0):diffuse(getMainColor("frames")):diffusealpha(0.5)
		end
	},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(5, offsetY - 9):zoom(0.6):halign(0):diffuse(getMainColor("positive")):settext(translated_info["Title"])
			end
		},
	OnCommand = function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		SCREENMAN:GetTopScreen():AddInputCallback(FilterInput)
		self:visible(false)
	end,
	SetCommand = function(self)
		self:finishtweening()
		if getTabIndex() == 5 then
			self:visible(true)
			active = true
		else
			MESSAGEMAN:Broadcast("NumericInputEnded")
			self:visible(false)
			self:queuecommand("Off")
			active = false
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	MouseRightClickMessageCommand = function(self)
		ActiveSS = 0
		MESSAGEMAN:Broadcast("NumericInputEnded")
		MESSAGEMAN:Broadcast("UpdateFilter")
		SCREENMAN:set_input_redirected(PLAYER_1, false)
	end,
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY):zoom(0.3):halign(0)
				self:settext(translated_info["ExplainStartInput"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY + 20):zoom(0.3):halign(0)
				self:settext(translated_info["ExplainCancelInput"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY + 40):zoom(0.3):halign(0)
				self:settext(translated_info["ExplainGrey"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY + 60):zoom(0.3):halign(0)
				self:settext(translated_info["ExplainBounds"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY + 80):zoom(0.3):halign(0)
				self:settext(translated_info["ExplainHighest"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX, frameY + 100):zoom(0.3):halign(0)
				self:settext(translated_info["ExplainHighestDifficulty"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + frameWidth / 2, 175):zoom(textzoom):halign(0)
			end,
			SetCommand = function(self)
				self:settextf("%s:%5.1fx", translated_info["MaxRate"], FILTERMAN:GetMaxFilterRate())
			end,
			MaxFilterRateChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ResetFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX + frameWidth / 2, 175):zoomto(130, 18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) and active then
				FILTERMAN:SetMaxFilterRate(FILTERMAN:GetMaxFilterRate() + 0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
		MouseRightClickMessageCommand = function(self)
			if isOver(self) and active then
				FILTERMAN:SetMaxFilterRate(FILTERMAN:GetMaxFilterRate() - 0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + frameWidth / 2, 175 + spacingY):zoom(textzoom):halign(0)
			end,
			SetCommand = function(self)
				self:settextf("%s:%5.1fx", translated_info["MinRate"], FILTERMAN:GetMinFilterRate())
			end,
			MaxFilterRateChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ResetFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX + frameWidth / 2, 175 + spacingY):zoomto(130, 18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) and active then
				FILTERMAN:SetMinFilterRate(FILTERMAN:GetMinFilterRate() + 0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
		MouseRightClickMessageCommand = function(self)
			if isOver(self) and active then
				FILTERMAN:SetMinFilterRate(FILTERMAN:GetMinFilterRate() - 0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + frameWidth / 2, 175 + spacingY * 2):zoom(textzoom):halign(0)
			end,
			SetCommand = function(self)
				if FILTERMAN:GetFilterMode() then
					self:settextf("%s: %s", translated_info["Mode"], translated_info["AND"])
				else
					self:settextf("%s: %s", translated_info["Mode"], translated_info["OR"])
				end
			end,
			FilterModeChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ResetFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX + frameWidth / 2, 175 + spacingY * 2):zoomto(120, 18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) and active then
				FILTERMAN:ToggleFilterMode()
				MESSAGEMAN:Broadcast("FilterModeChanged")
				whee:SongSearch("")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + frameWidth / 2, 175 + spacingY * 3):zoom(textzoom):halign(0)
			end,
			SetCommand = function(self)
				local translated = translated_info["HighestOnly"]
				if FILTERMAN:GetHighestSkillsetsOnly() then
					self:settextf("%s: %s", translated, translated_info["On"]):maxwidth(frameWidth / 2 / textzoom - 50)
				else
					self:settextf("%s: %s", translated, translated_info["Off"]):maxwidth(frameWidth / 2 / textzoom - 50)
				end
				if FILTERMAN:GetFilterMode() then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#FFFFFF"))
				end
			end,
			FilterModeChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ResetFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX + frameWidth / 2, 175 + spacingY * 3):zoomto(180, 18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) and active then
				FILTERMAN:ToggleHighestSkillsetsOnly()
				MESSAGEMAN:Broadcast("FilterModeChanged")
				whee:SongSearch("")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + frameWidth / 2, 175 + spacingY * 4):zoom(textzoom):halign(0)
			end,
			SetCommand = function(self)
				local translated = translated_info["HighestDifficultyOnly"]
				if FILTERMAN:GetHighestDifficultyOnly() then
					self:settextf("%s: %s", translated, translated_info["On"]):maxwidth(frameWidth / 2 / textzoom - 50)
				else
					self:settextf("%s: %s", translated, translated_info["Off"]):maxwidth(frameWidth / 2 / textzoom - 50)
				end
				if FILTERMAN:GetFilterMode() then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#FFFFFF"))
				end
			end,
			FilterModeChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ResetFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	Def.Quad {
		InitCommand = function(self)
			self:xy(frameX + frameWidth / 2, 175 + spacingY * 4):zoomto(180, 18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) and active then
				FILTERMAN:ToggleHighestDifficultyOnly()
				MESSAGEMAN:Broadcast("FilterModeChanged")
				whee:SongSearch("")
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(frameX + frameWidth / 2, 175 + spacingY * 6):zoom(textzoom):halign(0):settext("")
			end,
			FilterResultsMessageCommand = function(self, msg)
				self:settextf("%s: %i/%i", translated_info["Matches"], msg.Matches, msg.Total)
			end
		},
	LoadFont("Common Large") ..
		{
			BeginCommand = function(self)
				self:xy(frameX + frameWidth / 2, 175 + spacingY * 7):zoom(textzoom):halign(0):maxwidth(300)
				self.packlistFiltering = FILTERMAN:GetFilteringCommonPacks()
				self.enabled = SCREENMAN:GetTopScreen():GetName() == "ScreenNetSelectMusic"
				if not self.enabled then
					self:visible(false)
				end
				self:queuecommand("Set")
			end,
			MouseLeftClickMessageCommand = function(self)
				if self.enabled and isOver(self) and active then
					self.packlistFiltering = whee:SetPackListFiltering(not self.packlistFiltering)
					self:queuecommand("Set")
				end
			end,
			SetCommand = function(self)
				self:settextf("%s: %s", translated_info["CommonPackFilter"], (self.packlistFiltering and translated_info["On"] or translated_info["Off"]))
			end,
			FilterModeChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			ResetFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end
		}
}

local function CreateFilterInputBox(i)
	local t =
		Def.ActorFrame {
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:addx(10):addy(175 + (i - 1) * spacingY):halign(0):zoom(textzoom)
				end,
				SetCommand = function(self)
					self:settext(i == (#ms.SkillSets + 1) and translated_info["Length"] or ms.SkillSetsTranslated[i])
				end
			},
		Def.Quad {
			InitCommand = function(self)
				self:addx(i == (#ms.SkillSets + 1) and 159 or 150):addy(175 + (i - 1) * spacingY):zoomto(
					i == (#ms.SkillSets + 1) and 27 or 18,
					18
				):halign(1)
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) and active then
					ActiveSS = i
					activebound = 0
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand = function(self)
				if ActiveSS == i and activebound == 0 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdateFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			NumericInputEndedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			NumericInputActiveMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:addx(i == (#ms.SkillSets + 1) and 159 or 150):addy(175 + (i - 1) * spacingY):halign(1):maxwidth(60):zoom(
						textzoom
					)
				end,
				SetCommand = function(self)
					local fval = FILTERMAN:GetSSFilter(i, 0) -- lower bounds
					self:settext(fval)
					if fval <= 0 and ActiveSS ~= i then
						self:diffuse(color("#666666"))
					elseif activebound == 0 then
						self:diffuse(color("#FFFFFF"))
					end
				end,
				UpdateFilterMessageCommand = function(self)
					self:queuecommand("Set")
				end,
				NumericInputActiveMessageCommand = function(self)
					self:queuecommand("Set")
				end
			},
		Def.Quad {
			InitCommand = function(self)
				self:addx(i == (#ms.SkillSets + 1) and 193 or 175):addy(175 + (i - 1) * spacingY):zoomto(
					i == (#ms.SkillSets + 1) and 27 or 18,
					18
				):halign(1)
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) and active then
					ActiveSS = i
					activebound = 1
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand = function(self)
				if ActiveSS == i and activebound == 1 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdateFilterMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			NumericInputEndedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			NumericInputActiveMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:addx(i == (#ms.SkillSets + 1) and 193 or 175):addy(175 + (i - 1) * spacingY):halign(1):maxwidth(60):zoom(
						textzoom
					)
				end,
				SetCommand = function(self)
					local fval = FILTERMAN:GetSSFilter(i, 1) -- upper bounds
					self:settext(fval)
					if fval <= 0 and ActiveSS ~= i then
						self:diffuse(color("#666666"))
					elseif activebound == 1 then
						self:diffuse(color("#FFFFFF"))
					end
				end,
				UpdateFilterMessageCommand = function(self)
					self:queuecommand("Set")
				end,
				NumericInputActiveMessageCommand = function(self)
					self:queuecommand("Set")
				end
			}
	}
	return t
end

--reset button
f[#f + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(frameX + frameWidth - 150, frameY + 250 + spacingY):zoomto(60, 20):halign(0.5):diffuse(getMainColor("frames")):diffusealpha(
			0
		)
	end,
	MouseLeftClickMessageCommand = function(self)
		if isOver(self) and active then
			FILTERMAN:ResetAllFilters()
			for i = 1, #ms.SkillSets do
				SSQuery[0][i] = "0"
				SSQuery[1][i] = "0"
			end
			activebound = 0
			ActiveSS = 0
			MESSAGEMAN:Broadcast("UpdateFilter")
			MESSAGEMAN:Broadcast("ResetFilter")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			whee:SongSearch("")
		end
	end
}
f[#f + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand = function(self)
			self:xy(frameX + frameWidth - 150, frameY + 250 + spacingY):halign(0.5):zoom(0.35)
			self:settext(THEME:GetString("TabFilter", "Reset"))
		end
	}

for i = 1, (#ms.SkillSets + 1) do
	f[#f + 1] = CreateFilterInputBox(i)
end
return f
