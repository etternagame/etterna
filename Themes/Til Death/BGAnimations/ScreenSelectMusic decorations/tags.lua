
local hoverAlpha = 0.6

local onTab = false
local song
local steps
local curInput = ""
local frameX = 10
local frameY = 45
local frameWidth = capWideScale(360, 400)
local frameHeight = 350
local fontScale = 0.4
local tagsperpage = 14
local offsetX = 10
local offsetY = 20
local tagFunction = 1
local buttondiffuse = 0
local buttonheight = 10
local currenttagpage = 1
local numtagpages = 1
local tagYSpacing = 33
local whee
local filterChanged = false
local ptags = tags:get_data().playerTags
local playertags = {}
local displayindex = {}

local translated_info = {
	AddTag = THEME:GetString("TabTags", "AddTag"),
	ExcludeMode = THEME:GetString("TabTags", "ExcludeMode"),
	Mode = THEME:GetString("TabTags", "Mode"),
	AND = THEME:GetString("TabTags", "AND"),
	OR = THEME:GetString("TabTags", "OR"),
	Next = THEME:GetString("TabTags", "Next"),
	Previous = THEME:GetString("TabTags", "Previous"),
	Showing = THEME:GetString("TabTags", "Showing"),
	Title = THEME:GetString("TabTags", "Title"),
}

local function newTagInput(event)
	changed = false
	if event.type ~= "InputEventType_Release" and onTab and hasFocus then
		if event.button == "Start" then
			hasFocus = false
			if curInput ~= "" and ptags[curInput] == nil then
				tags:get_data().playerTags[curInput] = {}
				tags:set_dirty()
				tags:save()
			end
			curInput = ""
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			MESSAGEMAN:Broadcast("RefreshTags")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			return true
		elseif event.button == "Back" then
			curInput = ""
			hasFocus = false
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			MESSAGEMAN:Broadcast("RefreshTags")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			changed = true
			curInput = curInput:sub(1, -2)
		elseif event.DeviceInput.button == "DeviceButton_delete" then
			changed = true
			curInput = ""
		elseif
			event.char and curInput:len() < 20 and
				event.char:match('[% %%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%\'%"%>%<%?%/%~%|%w]') and
				event.char ~= ""
		 then
			changed = true
			curInput = curInput .. event.char
		end
		if changed then
			MESSAGEMAN:Broadcast("RefreshTags")
		end
	end
end

local t = Def.ActorFrame {
	Name = "Tongo",
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(newTagInput)
		self:queuecommand("BORPBORPNORFNORFc"):visible(false)
	end,
	OffCommand = function(self)
		--for some reason, tweening this tab causes a recursing tween error?????? help  -ulti
		--self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
		self:diffusealpha(0)
		self:sleep(0.04):queuecommand("Invis")
	end,
	InvisCommand= function(self)
		self:visible(false)
	end,
	OnCommand = function(self)
		--here too
		--self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
		self:diffusealpha(1)
	end,
	MouseRightClickMessageCommand = function(self)
		if onTab then
			hasFocus = false
			curInput = ""
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			MESSAGEMAN:Broadcast("NumericInputEnded")
			MESSAGEMAN:Broadcast("RefreshTags")
		end
	end,
	BORPBORPNORFNORFcCommand = function(self)
		if getTabIndex() == 9 then
			self:visible(true)
			self:queuecommand("On")
			song = GAMESTATE:GetCurrentSong()
			steps = GAMESTATE:GetCurrentSteps()
			onTab = true
			MESSAGEMAN:Broadcast("RefreshTags")
		else
			self:queuecommand("Off")
			onTab = false
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("BORPBORPNORFNORFc")
	end,
	CurrentStepsChangedMessageCommand = function(self)
		if getTabIndex() == 9 then
			self:playcommand("BORPBORPNORFNORFc"):finishtweening()
		end
	end,
}

t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, frameHeight):halign(0):valign(0):diffuse(getMainColor("tabs"))
	end
}
t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:xy(frameX, frameY):zoomto(frameWidth, offsetY):halign(0):valign(0)
		self:diffuse(getMainColor("frames")):diffusealpha(0.5)
	end
}
t[#t + 1] = LoadFont("Common Normal") .. {
	InitCommand = function(self)
		self:xy(frameX + 5, frameY + offsetY - 11.5):zoom(0.65):halign(0)
		self:settext(translated_info["Title"])
		self:diffuse(Saturation(getMainColor("positive"), 0.1))
	end
}

local function filterDisplay(playertags)
	local index = {}
	for i = 1, #playertags do
		index[#index + 1] = i
	end
	return index
end

local r = Def.ActorFrame {
	BeginCommand = function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		if filterTags == nil then
			filterTags = {}
		end
		if filterAgainstTags == nil then
			filterAgainstTags = {}
		end
		-- apparently i cant just do if charts and next(charts) to check nil charts
		if (charts ~= nil and #charts ~= 0) or (oCharts ~= nil and #oCharts ~= 0) then
			-- not sure why the other song doesnt work i hate this
			local ssong = GAMESTATE:GetCurrentSong()
			whee:FilterByAndAgainstStepKeys(charts, oCharts)
			whee:SelectSong(ssong)
		end

	end,
	RefreshTagsMessageCommand = function(self)
		if filterMode == nil then
			filterMode = true
		end
		if filterAgainstMode == nil then
			filterAgainstMode = false
		end
		ptags = tags:get_data().playerTags
		-- filtering
		if filterChanged then
			charts = {}
			oCharts = {}

			if next(filterTags) then
				-- MODE == AND in menu, requires all tags to be active
				if filterMode then
					local toFilterTags = {}
					for tag, v in pairs(filterTags) do
						toFilterTags[#toFilterTags + 1] = tag
					end

					local inCharts = {}
					-- Gather initial first tags chart keys
					for chartKey, v in pairs(ptags[toFilterTags[1]]) do
						inCharts[#inCharts + 1] = chartKey
					end
					-- Subtract all the charts that dont have the additional keys
					for i = 2, #toFilterTags do
						for k, chartKey in pairs(inCharts) do
							if ptags[toFilterTags[i]][chartKey] == nil then
								inCharts[k] = nil
							end
						end
					end
					for k, chartKey in pairs(inCharts) do
						charts[#charts + 1] = chartKey
					end
				-- MODE == OR in menu, requires only one tag to be active
				else
					-- Just collect everything that has the filter tag
					for tag, v in pairs(filterTags) do
						for chartKey, v in pairs(ptags[tag]) do
							charts[#charts + 1] = chartKey
						end
					end
				end
			end
			if next(filterAgainstTags) then
				-- MODE == AND in menu, requires all tags to be active
				if filterAgainstMode then
					local toFilterAgainstTags = {}
					for tag, v in pairs(filterAgainstTags) do
						toFilterAgainstTags[#toFilterAgainstTags + 1] = tag
					end

					local outCharts = {}
					-- Gather initial first tags chart keys
					for chartKey, v in pairs(ptags[toFilterAgainstTags[1]]) do
						outCharts[#outCharts + 1] = chartKey
					end
					-- Subtract all the oCharts that dont have the additional keys
					for i = 2, #toFilterAgainstTags do
						for k, chartKey in pairs(outCharts) do
							if ptags[toFilterAgainstTags[i]][chartKey] == nil then
								outCharts[k] = nil
							end
						end
					end
					for k, chartKey in pairs(outCharts) do
						oCharts[#oCharts + 1] = chartKey
					end
				-- MODE == OR in menu, requires only one tag to be active
				else
					-- Just collect everything that has the filter tag
					for tag, v in pairs(filterAgainstTags) do
						for chartKey, v in pairs(ptags[tag]) do
							oCharts[#oCharts + 1] = chartKey
						end
					end
				end
			end
			local ssong = GAMESTATE:GetCurrentSong()
			whee:FilterByAndAgainstStepKeys(charts, oCharts)
			whee:SelectSong(ssong)
			filterChanged = false
		end


		playertags = {}
		for k, v in pairs(ptags) do
			playertags[#playertags + 1] = k
		end
		table.sort(playertags)
		displayindex = filterDisplay(playertags)
		numtagpages = notShit.ceil(#displayindex / tagsperpage)
		MESSAGEMAN:Broadcast("UpdateTags")
	end
}

local function makeTag(i)
	local t = Def.ActorFrame {
		InitCommand = function(self)
			local colPos = i / 8 >= 1 and 20 + (frameWidth / 2) or offsetX + 10
			local row = i > 7 and i - 8 or i - 1
			self:xy(colPos, offsetY + 95 + row * tagYSpacing)
			self:visible(true)
		end,
		UpdateTagsMessageCommand = function(self)
			if playertags[i + ((currenttagpage - 1) * tagsperpage)] then
				self:visible(true)
			else
				self:visible(false)
			end
		end,
		Def.ActorFrame {
			InitCommand = function(self)
				self:x(5)
			end,
			UIElements.QuadButton(1, 1) .. {
				InitCommand = function(self)
					self:xy(-6, 20):zoomto(frameWidth / 2 - 20, tagYSpacing - 2):halign(0):valign(1)
				end,
				UpdateTagsMessageCommand = function(self)
					curTag = playertags[i + ((currenttagpage - 1) * tagsperpage)]
					if tagFunction == 1 then
						if song and curTag and ptags[curTag][steps:GetChartKey()] then
							self:diffuse(getMainColor("positive"))
						else
							self:diffuse(getMainColor("frames")):diffusealpha(0.35)
						end
					elseif tagFunction == 2 then
						if filterTags[curTag] then
							self:diffuse(getMainColor("positive"))
						elseif filterAgainstTags[curTag] then
							self:diffuse(getMainColor("negative"))
						else
							self:diffuse(getMainColor("frames")):diffusealpha(0.35)
						end
					else
						self:diffuse(getMainColor("frames")):diffusealpha(0.35)
					end
				end,
				MouseDownCommand = function(self, params)
					if params.event == "DeviceButton_left mouse button" then
						curTag = playertags[i + ((currenttagpage - 1) * tagsperpage)]
						if tagFunction == 1 then
							ck = steps:GetChartKey()
							if ptags[curTag][ck] then
								tags:get_data().playerTags[curTag][ck] = nil
							else
								tags:get_data().playerTags[curTag][ck] = 1
							end
							tags:set_dirty()
							tags:save()
						elseif tagFunction == 2 then
							if filterAgainstTags[curTag] then
								filterAgainstTags[curTag] = nil
							end

							if filterTags[curTag] then
								filterTags[curTag] = nil
							else
								filterTags[curTag] = 1
							end
						else
							if filterTags[curTag] then
								filterTags[curTag] = nil
							end
							tags:get_data().playerTags[curTag] = nil
							tags:set_dirty()
							tags:save()
						end
						filterChanged = true
						MESSAGEMAN:Broadcast("RefreshTags")
					elseif params.event == "DeviceButton_right mouse button" then
						curTag = playertags[i + ((currenttagpage - 1) * tagsperpage)]
						if tagFunction == 2 then
							if filterTags[curTag] then
								filterTags[curTag] = nil
							end

							if filterAgainstTags[curTag] then
								filterAgainstTags[curTag] = nil
							else
								filterAgainstTags[curTag] = 1
							end
							filterChanged = true
						end
						MESSAGEMAN:Broadcast("RefreshTags")
					end
				end,
				MouseOverCommand = function(self)
					self:GetParent():diffusealpha(hoverAlpha)
				end,
				MouseOutCommand = function(self)
					self:GetParent():diffusealpha(1)
				end,
			},
			LoadFont("Common Large") .. {
				Name = "Text",
				InitCommand = function(self)
					self:y(5):halign(0):maxwidth(frameWidth + 25)
				end,
				UpdateTagsMessageCommand = function(self)
					self:zoom(fontScale)
					if playertags[i + ((currenttagpage - 1) * tagsperpage)] then
						self:settext(playertags[i + ((currenttagpage - 1) * tagsperpage)])
					end
				end
			}
		}
	}
	return t
end

local fawa = {
	THEME:GetString("TabTags", "TagList"),
	THEME:GetString("TabTags", "TagFilter"),
	THEME:GetString("TabTags", "TagDelete")
}
local function funcButton(i)
	local t = Def.ActorFrame {
		InitCommand = function(self)
			local colPos = (i - 1) * (frameWidth / 3 - 5) + 80
			self:xy(colPos, frameY + capWideScale(80, 80) - 55)
			self:visible(true)
		end,
		UIElements.QuadButton(1, 1) .. {
			InitCommand = function(self)
				self:zoomto((frameWidth / 3 - 10), 30):halign(0.5):valign(0):diffuse(getMainColor("frames")):diffusealpha(0.35)
			end,
			BORPBORPNORFNORFcCommand = function(self)
				if tagFunction == i then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.35)
				end
			end,
			MouseDownCommand = function(self, params)
				if params.event == "DeviceButton_left mouse button" then
					tagFunction = i
					MESSAGEMAN:Broadcast("RefreshTags")
				end
			end,
			UpdateTagsMessageCommand = function(self)
				self:queuecommand("BORPBORPNORFNORFc")
			end,
			MouseOverCommand = function(self)
				self:GetParent():diffusealpha(0.6)
			end,
			MouseOutCommand = function(self)
				self:GetParent():diffusealpha(1)
			end,
		},
		LoadFont("Common Large") .. {
			InitCommand = function(self)
				self:y(12):halign(0.5):diffuse(getMainColor("positive")):maxwidth((frameWidth / 3 - 30)):maxheight(22)
			end,
			BeginCommand = function(self)
				self:settext(fawa[i])
			end,
		}
	}
	return t
end

-- new tag input
r[#r + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX + 10, frameY + capWideScale(80, 80) + 225)
	end,
	BORPBORPNORFNORFcCommand = function(self)
		self:visible(tagFunction == 1)
	end,
	UpdateTagsMessageCommand = function(self)
		self:queuecommand("BORPBORPNORFNORFc")
	end,
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:halign(0):zoom(fontScale)
		end,
		BORPBORPNORFNORFcCommand = function(self)
			self:settextf("%s:", translated_info["AddTag"])
		end
	},
	UIElements.QuadButton(1, 1) .. {
		InitCommand = function(self)
			self:addx(129):addy(3):zoomto(capWideScale(210,250), 21):halign(0):diffuse(color("#666666"))
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and onTab then
				hasFocus = true
				curInput = ""
				SCREENMAN:set_input_redirected(PLAYER_1, true)
				self:diffusealpha(0.1)
				MESSAGEMAN:Broadcast("RefreshTags")
				MESSAGEMAN:Broadcast("NumericInputActive")
			end
		end,
		BORPBORPNORFNORFcCommand = function(self)
			if hasFocus then
				self:diffuse(color("#999999"))
			else
				self:diffuse(color("#000000"))
			end
		end,
		UpdateTagsMessageCommand = function(self)
			self:queuecommand("BORPBORPNORFNORFc")
		end
	},
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:addx(133):addy(2):halign(0):maxwidth(600):zoom(fontScale - 0.05)
		end,
		BORPBORPNORFNORFcCommand = function(self)
			self:settext(curInput)
			if curInput ~= "" or hasFocus then
				self:diffuse(color("#FFFFFF"))
			else
				self:diffuse(color("#666666"))
			end
		end,
		UpdateTagsMessageCommand = function(self)
			self:queuecommand("BORPBORPNORFNORFc")
		end
	}
}

-- filter type
r[#r + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX + 10, frameY + capWideScale(80, 80) + 225)
	end,
	BORPBORPNORFNORFcCommand = function(self)
		self:visible(tagFunction == 2)
	end,
	UpdateTagsMessageCommand = function(self)
		self:queuecommand("BORPBORPNORFNORFc")
	end,
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:zoom(fontScale):halign(0)
			self:diffuse(getMainColor("positive"))
		end,
		BORPBORPNORFNORFcCommand = function(self)
			self:settextf("%s: %s", translated_info["Mode"], (filterMode and translated_info["AND"] or translated_info["OR"])):maxwidth(((frameWidth - 40) / 2) / fontScale)
		end,
		UpdateTagsMessageCommand = function(self)
			self:queuecommand("BORPBORPNORFNORFc")
		end,
	},
	UIElements.QuadButton(1, 1) .. {
		InitCommand = function(self)
			self:zoomto((frameWidth - 40) / 2, 18):halign(0):diffusealpha(0)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and onTab then
				filterMode = not filterMode
				filterChanged = true
				MESSAGEMAN:Broadcast("RefreshTags")
			end
		end,
		MouseOverCommand = function(self)
			self:GetParent():diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:GetParent():diffusealpha(1)
		end,
	}
}

-- filter against type
r[#r + 1] = Def.ActorFrame {
	InitCommand = function(self)
		-- Is inverse of frameX + 10, makes it start at exactly half way + 10px each side padding
		self:xy(frameX + ((frameWidth - 40) / 2) + 30, frameY + capWideScale(80, 80) + 225)
	end,
	BORPBORPNORFNORFcCommand = function(self)
		self:visible(tagFunction == 2)
	end,
	UpdateTagsMessageCommand = function(self)
		self:queuecommand("BORPBORPNORFNORFc")
	end,
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:zoom(fontScale):halign(0)
			self:diffuse(getMainColor("positive"))
		end,
		BORPBORPNORFNORFcCommand = function(self)
			self:settextf("%s: %s", translated_info["ExcludeMode"], (filterAgainstMode and translated_info["AND"] or translated_info["OR"])):maxwidth(((frameWidth - 40) / 2) / fontScale)
		end,
		UpdateTagsMessageCommand = function(self)
			self:queuecommand("BORPBORPNORFNORFc")
		end,
	},
	UIElements.QuadButton(1, 1) .. {
		InitCommand = function(self)
			self:zoomto(((frameWidth - 40) / 2), 18):halign(0):diffusealpha(0)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and onTab then
				filterAgainstMode = not filterAgainstMode
				filterChanged = true
				MESSAGEMAN:Broadcast("RefreshTags")
			end
		end,
		MouseOverCommand = function(self)
			self:GetParent():diffusealpha(hoverAlpha)
		end,
		MouseOutCommand = function(self)
			self:GetParent():diffusealpha(1)
		end,
	}
}

-- main quad with paginator i guess?
r[#r + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(frameX + 28, frameY + capWideScale(80, 80) + 253)
	end,
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:halign(0):zoom(0.3):diffuse(getMainColor("positive")):settext(translated_info["Previous"])
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and currenttagpage > 1 then
				currenttagpage = currenttagpage - 1
				MESSAGEMAN:Broadcast("RefreshTags")
			end
		end
	},
	UIElements.TextToolTip(1, 1, "Common Large") .. {
		InitCommand = function(self)
			self:x(capWideScale(270,300)):halign(0):zoom(0.3):diffuse(getMainColor("positive")):settext(translated_info["Next"])
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" and currenttagpage < numtagpages then
				currenttagpage = currenttagpage + 1
				MESSAGEMAN:Broadcast("RefreshTags")
			end
		end
	},
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:x(capWideScale(160,175)):halign(0.5):zoom(0.3)
		end,
		BORPBORPNORFNORFcCommand = function(self)
			self:settextf(
				"%s %i-%i (%i)",
				translated_info["Showing"],
				math.min(((currenttagpage - 1) * tagsperpage) + 1, #displayindex),
				math.min(currenttagpage * tagsperpage, #displayindex),
				#displayindex
			)
		end,
		UpdateTagsMessageCommand = function(self)
			self:queuecommand("BORPBORPNORFNORFc")
		end
	}
}

for i = 1, tagsperpage do
	r[#r + 1] = makeTag(i)
end

for i = 1, 3 do
	r[#r + 1] = funcButton(i)
end

t[#t + 1] = r

return t
