
--[[

pick a tag or enter a name
hit enter or hit apply, search begins
results put you on page 1
packlist is irrelevant?

]]

local function diffuseIfActiveButton(self, cond)
	if cond then
		self:diffuse(color("#666666"))
	else
		self:diffuse(color("#ffffff"))
	end
end

local function diffuseIfActiveText(self, cond)
	if cond then
		self:diffuse(color("#FFFFFF"))
	else
		self:diffuse(color("#666666"))
	end
end

local activealpha = 0.1
local inactivealpha = 0.3
local highlightalpha = 0.5

local translated_info = {
	CancelCurrent = THEME:GetString("ScreenPackDownloader", "CancelCurrentDownload"),
	CancelAll = THEME:GetString("ScreenPackDownloader", "CancelAllDownloads"),
	SearchName = THEME:GetString("ScreenPackDownloader", "SearchingName"),
	SizeExplanation = THEME:GetString("ScreenPackDownloader", "ExplainSizeLimit")
}

local width = SCREEN_WIDTH / 3
local fontScale = 0.5
local packh = 30
local packgap = 4
local packspacing = packh + packgap
local offx = 10
local offy = 40

local fx = SCREEN_WIDTH / 4.5 -- this isnt very smart alignment
local f0y = 160
local f1y = f0y + 40
local f2y = f1y + 40
local fdot = 24

local tagFrameWidth = SCREEN_WIDTH / 3
local cancelButtonSpace = 4
local leftSpace = 10
local cancelFrameY = 56

local selectedTags = {}
local nameInput = ""

local o = Def.ActorFrame {
	Name = "MainFrame",
	InitCommand = function(self)
		self:xy(0, 0):halign(0.5):valign(0)
		self:GetChild("PacklistDisplay"):xy(SCREEN_WIDTH / 2.5 - offx, offy * 2 + 14)
	end,
	WheelUpSlowMessageCommand = function(self)
		self:queuecommand("PrevPage")
	end,
	WheelDownSlowMessageCommand = function(self)
		self:queuecommand("NextPage")
	end,
	UpdateFilterDisplaysMessageCommand = function(self)
		self:queuecommand("Set")
	end,
	FilterChangedMessageCommand = function(self)
		self:queuecommand("PackTableRefresh")
	end,
	MouseRightClickMessageCommand = function(self)
		SCREENMAN:GetTopScreen():Cancel()
	end,

}



o[#o+1] = Def.ActorFrame {
	Name = "LeftButtonFrame",
	InitCommand = function(self)
		self:xy(leftSpace + tagFrameWidth/2, cancelFrameY)
	end,

	LoadFont("Common Large") .. {
		Name = "TitleText",
		InitCommand = function(self)
			self:y(-cancelFrameY/2)
			self:zoom(0.4)
			self:maxwidth(SCREEN_WIDTH / 2)
			self:settext("Pack Downloads")
		end
	},
	UIElements.TextButton(1, 1, "Common Large") .. {
		Name = "StopAllDownloadsButton",
		InitCommand = function(self)
			self.txt = self:GetChild("Text")
			self.bg = self:GetChild("BG")

			self:xy(-tagFrameWidth/2, packh)

			self.txt:xy(tagFrameWidth/4 - leftSpace/4, packh/2)
			self.txt:valign(0.5)
			self.txt:settext(translated_info["CancelAll"])
			self.txt:zoom(0.4)
			self.txt:maxwidth((tagFrameWidth/2 - leftSpace) / 0.4)

			self.bg:zoomto(tagFrameWidth/2 - leftSpace/2, packh)
			self.bg:halign(0):valign(0)
			self.bg:diffuse(color("#ffffff"))

			self.alphaDeterminingFunction = function(self)
				if isOver(self.bg) then
					self.bg:diffusealpha(0.8)
				else
					self.bg:diffusealpha(0.4)
				end
			end
			self:alphaDeterminingFunction()
		end,
		RolloverUpdateCommand = function(self, params)
			self:alphaDeterminingFunction()
		end,
		ClickCommand = function(self, params)
			if params.update == "OnMouseDown" then
				for k,v in pairs(DLMAN:GetDownloads()) do
					v:Stop()
				end
			end
		end,
	},
	UIElements.TextButton(1, 1, "Common Large") .. {
		Name = "StopCurrentDownloadButton",
		InitCommand = function(self)
			self.txt = self:GetChild("Text")
			self.bg = self:GetChild("BG")

			self:xy(leftSpace/2, packh)

			self.txt:xy(tagFrameWidth/4 - leftSpace/4, packh/2)
			self.txt:valign(0.5)
			self.txt:settext(translated_info["CancelCurrent"])
			self.txt:zoom(0.4)
			self.txt:maxwidth((tagFrameWidth/2 - leftSpace) / 0.4)

			self.bg:zoomto(tagFrameWidth/2 - leftSpace/2, packh)
			self.bg:halign(0):valign(0)
			self.bg:diffuse(color("#ffffff"))

			self.alphaDeterminingFunction = function(self)
				if isOver(self.bg) then
					self.bg:diffusealpha(0.8)
				else
					self.bg:diffusealpha(0.4)
				end
			end
			self:alphaDeterminingFunction()
		end,
		RolloverUpdateCommand = function(self, params)
			self:alphaDeterminingFunction()
		end,
		ClickCommand = function(self, params)
			if params.update == "OnMouseDown" then
				local dl = DLMAN:GetDownloads()[1]
				if dl then
					dl:Stop()
				end
			end
		end,
	},
}

local function tagframe()
	local maxtags = 12
	local curpage = 1

	local frameBGHeight = SCREEN_HEIGHT - (f0y-30) - leftSpace
	local frameBGWidth = SCREEN_WIDTH / 3
	local tagSpacing = 2
	local tagHeight = ((frameBGHeight * 0.7) / maxtags)
	local tagWidth = frameBGWidth - tagSpacing*2
	local tagTextSize = 0.5
	local tagListStartY = frameBGHeight * 0.22

	local alltags = DLMAN:GetPackTags()
	local skillsetTags = table.sorted(alltags["global_skillset"])
	local keycountTags = table.sorted(alltags["global_keyCount"], function(a,b)
		local ax = a:sub(1, #a-1)
		local bx = b:sub(1, #b-1)
		return tonumber(ax) < tonumber(bx)
	end)
	local otherTags = table.sorted(alltags["pack_tag"])
	local orderedTags = table.combine(keycountTags, skillsetTags, otherTags)

	local function movePage(n)
		local newpage = curpage + n
		local maxpage = math.ceil(#orderedTags / maxtags)
		if newpage < 1 then
			newpage = maxpage
		elseif newpage > maxpage then
			newpage = 1
		end
		curpage = newpage
		MESSAGEMAN:Broadcast("SetTagPage")
	end

	local t = Def.ActorFrame {
		Name = "TagFrame",
		InitCommand = function(self)
			self:xy(leftSpace, f0y - 30)
		end,
		BeginCommand = function(self)
			SCREENMAN:GetTopScreen():AddInputCallback(function(event)
				if isOver(self:GetChild("BG")) then
					if event.type == "InputEventType_FirstPress" then
						if event.DeviceInput.button == "DeviceButton_mousewheel up" then
							movePage(-1)
						elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
							movePage(1)
						end
					end
				end
			end)
			self:playcommand("UpdateTags")
		end,
		SetTagPageMessageCommand = function(self)
			self:playcommand("UpdateTags")
		end,
	
		Def.Quad {
			Name = "BG",
			InitCommand = function(self)
				self:halign(0):valign(0)
				self:zoomto(frameBGWidth, frameBGHeight)
				self:diffuse(color("#666666"))
				self:diffusealpha(0.4)
			end
		},
		LoadFont("Common Large") .. {
			Name = "TagExplain",
			InitCommand = function(self)
				self:settext("Select tags to filter packs")
				self:zoom(0.4)
				self:valign(0)
				self:xy(frameBGWidth/2, leftSpace)
				self:maxwidth(frameBGWidth/0.4)
			end,
		},
		LoadFont("Common Large") .. {
			Name = "PageNum",
			InitCommand = function(self)
				self:zoom(0.2)
				self:valign(1):halign(1)
				self:xy(frameBGWidth - leftSpace/2, tagListStartY - tagSpacing)
			end,
			UpdateTagsCommand = function(self)
				self:settextf("%d-%d of %d", (curpage-1) * maxtags + 1, math.min(#orderedTags, curpage * maxtags), #orderedTags)
			end,
		},
		UIElements.TextButton(1, 1, "Common Large") .. {
			Name = "ApplyButton",
			InitCommand = function(self)
				self.bg = self:GetChild("BG")
				self.txt = self:GetChild("Text")
				self:xy(tagSpacing, tagListStartY - tagSpacing - packh*0.8)

				self.txt:xy(tagFrameWidth/4 - leftSpace/4, (packh*0.8)/2)
				self.txt:settext("Apply")
				self.txt:zoom(0.4)
				self.txt:maxwidth((tagFrameWidth/2 - leftSpace) / 0.4)
				self.bg:zoomto(tagFrameWidth/2 - leftSpace/2, packh*0.8)
				self.bg:halign(0):valign(0)
				self.bg:diffuse(color("#ffffff"))

				self.alphaDeterminingFunction = function(self)
					if isOver(self.bg) then
						self.bg:diffusealpha(0.8)
					else
						self.bg:diffusealpha(0.4)
					end
				end
				self:alphaDeterminingFunction()
			end,
			RolloverUpdateCommand = function(self, params)
				self:alphaDeterminingFunction()
			end,
			ClickCommand = function(self, params)
				local tags = {}
				for k,v in pairs(selectedTags) do
					if v == true then
						tags[#tags+1] = k
					end
				end
				MESSAGEMAN:Broadcast("InvokePackSearch", {name=nameInput, tags=tags})
			end,
		}
	}

	local function tagentry(i)
		local tagtxt = nil
		return UIElements.TextButton(1, 1, "Common Normal") .. {
			Name = "Tag"..i,
			InitCommand = function(self)
				self.bg = self:GetChild("BG")
				self.txt = self:GetChild("Text")

				self:xy(
					tagFrameWidth/2,
					tagListStartY + tagHeight/2 + tagSpacing + (i-1) * (tagHeight + tagSpacing)
				)

				self.bg:zoomto(tagWidth, tagHeight)
				self.txt:zoom(tagTextSize)
				self.bg:diffuse(color("#000000FF"))

				self.alphaDeterminingFunction = function(self)
					local mult = selectedTags[tagtxt] and 1.5 or 1
					if isOver(self.bg) then
						self.bg:diffusealpha(0.8 * mult)
					else
						self.bg:diffusealpha(0.3 * mult*mult)
					end
				end
				self:alphaDeterminingFunction()
			end,
			UpdateTagsCommand = function(self)
				tagtxt = orderedTags[i + ((curpage-1) * maxtags)]

				if tagtxt then
					self:visible(true)
					self.txt:settextf("%s", tagtxt)
				else
					self:visible(false)
				end
				self:alphaDeterminingFunction()
			end,
			RolloverUpdateCommand = function(self, params)
				self:alphaDeterminingFunction()
			end,
			ClickCommand = function(self, params)
				if selectedTags[tagtxt] == true then
					selectedTags[tagtxt] = nil
				else
					selectedTags[tagtxt] = true
				end
				self:alphaDeterminingFunction()
			end,
		}
	end

	for i=1,maxtags do
		t[#t+1] = tagentry(i)
	end

	return t
end
o[#o+1] = tagframe()

local nwidth = SCREEN_WIDTH / 2
local namex = nwidth
local namey = 40
local nhite = 22
local nameoffx = 20

-- name string search
o[#o + 1] = Def.ActorFrame {
	Name = "TextEntryFrame",
	InitCommand = function(self)
		self:xy(namex, namey):halign(0):valign(0)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(function(event)
			if event.type ~= "InputEventType_Release" then
				local btn = event.DeviceInput.button
				local shift = INPUTFILTER:IsShiftPressed()
				local ctrl = INPUTFILTER:IsControlPressed()

				if btn == "DeviceButton_enter" or event.button == "Start" then
					-- invoke search
					local tags = {}
					for k,v in pairs(selectedTags) do
						if v == true then
							tags[#tags+1] = k
						end
					end
					MESSAGEMAN:Broadcast("InvokePackSearch", {name=nameInput, tags=tags})
				else
					local del = btn == "DeviceButton_delete"
					local bs = btn == "DeviceButton_backspace"
					local copypasta = btn == "DeviceButton_v" and ctrl
					local char = inputToCharacter(event)

					-- paste
					if copypasta then
						char = Arch.getClipboard()
					end

					if bs then
						nameInput = nameInput:sub(1, -2)
					elseif del then
						nameInput = ""
					elseif char then
						nameInput = nameInput .. char
					else
						return
					end

					self:playcommand("Set")

				end
			end
		end)
	end,
	UIElements.QuadButton(1, 1) .. {
		Name = "SearchBox",
		InitCommand = function(self)
			self:zoomto(nwidth - leftSpace, nhite):halign(0)
			self:diffusealpha(inactivealpha)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				inputting = 1
				curInput = ""
				self:GetParent():GetParent():queuecommand("Set")
				self:diffusealpha(activealpha)
				SCREENMAN:set_input_redirected(PLAYER_1, true)
			end
		end,
		SetCommand = function(self)
			diffuseIfActiveButton(self, inputting == 1)
			if isOver(self) then
				self:diffusealpha(highlightalpha)
			else
				self:diffusealpha(inactivealpha)
			end
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(highlightalpha)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(inactivealpha)
		end,
	},
	LoadFont("Common Large") .. {
		Name = "UserInputText",
		InitCommand = function(self)
			self:x(nameoffx):halign(0)
			self:zoom(fontScale)
			self:maxwidth((nwidth - leftSpace - nameoffx) / fontScale)
		end,
		SetCommand = function(self)
			local fval = nameInput
			self:settext(fval)
			diffuseIfActiveText(self, fval ~= "" or inputting == 1)
		end,
	},
	LoadFont("Common Large") .. {
		Name = "SearchLabel",
		InitCommand = function(self)
			self:xy(-2,-1) -- font problems
			self:halign(1)
			self:zoom(fontScale)
			self:settextf("%s:", translated_info["SearchName"])
		end,
	},
	LoadFont("Common Normal") .. {
		Name = "PackSizeRestrictionLabel",
		InitCommand = function(self)
			self:xy(-90, 40):halign(0):valign(0)
			self:zoom(fontScale)
			self:settextf("%s", translated_info["SizeExplanation"])
		end,
	}
}

o[#o + 1] = LoadActor("packlistDisplay")

return o
