
local hoverAlpha = 0.6

local t = Def.ActorFrame {}

local frameWidth = 280
local frameHeight = 20
local frameX = SCREEN_WIDTH - 5
local frameY = 15

local sortTable = {
	SortOrder_Group = THEME:GetString("SortOrder", "Group"),
	SortOrder_Title = THEME:GetString("SortOrder", "Title"),
	SortOrder_BPM = THEME:GetString("SortOrder", "BPM"),
	SortOrder_TopGrades = THEME:GetString("SortOrder", "TopGrades"),
	SortOrder_Artist = THEME:GetString("SortOrder", "Artist"),
	SortOrder_Genre = THEME:GetString("SortOrder", "Genre"),
	SortOrder_ModeMenu = THEME:GetString("SortOrder", "ModeMenu"),
	SortOrder_Length = THEME:GetString("SortOrder", "Length"),
	SortOrder_Favorites = THEME:GetString("SortOrder", "Favorites"),
	SortOrder_Overall = THEME:GetString("SortOrder", "Overall"),
	SortOrder_Stream = THEME:GetString("SortOrder", "Stream"),
	SortOrder_Jumpstream = THEME:GetString("SortOrder", "Jumpstream"),
	SortOrder_Handstream = THEME:GetString("SortOrder", "Handstream"),
	SortOrder_Stamina = THEME:GetString("SortOrder", "Stamina"),
	SortOrder_JackSpeed = THEME:GetString("SortOrder", "JackSpeed"),
	SortOrder_Chordjack = THEME:GetString("SortOrder", "Chordjack"),
	SortOrder_Technical = THEME:GetString("SortOrder", "Technical"),
	SortOrder_Ungrouped = THEME:GetString("SortOrder", "Ungrouped")
}

local translated_info = {
	Sort = THEME:GetString("SortOrder", "SortWord")
}

local group_rand = ""
t[#t + 1] = UIElements.TextToolTip(1, 1, "Common Large") .. {
	Name="rando",
	InitCommand = function(self)
		self:xy(frameX, frameY + 5):halign(1):zoom(0.55):maxwidth((frameWidth - 40) / 0.35)
	end,
	BeginCommand = function(self)
		self:queuecommand("Set")
	end,
	SetCommand = function(self)
		local sort = GAMESTATE:GetSortOrder()
		local song = GAMESTATE:GetCurrentSong()
		if sort == nil then
			self:settextf("%s: ", translated_info["Sort"])
		elseif sort == "SortOrder_Group" and song ~= nil then
			group_rand = song:GetGroupName()
			self:settext(group_rand)
		else
			self:settextf("%s: %s", translated_info["Sort"], sortTable[sort])
			group_rand = ""
		end
	end,
	SortOrderChangedMessageCommand = function(self)
		self:queuecommand("Set"):diffuse(getMainColor("positive"))
	end,
	CurrentSongChangedMessageCommand = function(self)
		self:playcommand("Set")
	end,
	MouseDownCommand = function(self, params)
		if group_rand ~= "" and params.event == "DeviceButton_left mouse button" then
			local w = SCREENMAN:GetTopScreen():GetMusicWheel()

			if INPUTFILTER:IsShiftPressed() and self.lastlastrandom ~= nil then

				-- if the last random song wasnt filtered out, we can select it
				-- so end early after jumping to it
				if w:SelectSong(self.lastlastrandom) then
					return
				end
				-- otherwise, just pick a new random song
			end

			local t = w:GetSongsInGroup(group_rand)
			if #t == 0 then return end
			local random_song = t[math.random(#t)]
			w:SelectSong(random_song)
			self.lastlastrandom = self.lastrandom
			self.lastrandom = random_song
		end
	end,
	MouseOverCommand = function(self)
		if group_rand ~= "" then
			self:diffusealpha(hoverAlpha)
		end
	end,
	MouseOutCommand = function(self)
		if group_rand ~= "" then
			self:diffusealpha(1)
		end
	end,
}

t[#t + 1] = StandardDecorationFromFileOptional("BPMDisplay", "BPMDisplay")
t[#t + 1] = StandardDecorationFromFileOptional("BPMLabel", "BPMLabel")

return t
