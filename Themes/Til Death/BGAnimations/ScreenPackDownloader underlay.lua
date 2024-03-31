
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
				local dl = DLMAN:GetDownloads()[1]
				if dl then
					dl:Stop()
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

o[#o+1] = Def.ActorFrame {
	Name = "TagFrame",
	InitCommand = function(self)
		self:xy(leftSpace, f0y - 30)
	end,

	Def.Quad {
		Name = "BG",
		InitCommand = function(self)
			self:halign(0):valign(0)
			self:zoomto(SCREEN_WIDTH / 3, SCREEN_HEIGHT - (f0y-30) - leftSpace)
			self:diffuse(color("#666666"))
			self:diffusealpha(0.4)
		end
	},
}

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
			
		end)
	end,
	UIElements.QuadButton(1, 1) .. {
		Name = "SearchBox",
		InitCommand = function(self)
			self:zoomto(nwidth - leftSpace, nhite):halign(0):valign(0)
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
			self:x(nameoffx):halign(0):valign(0)
			self:zoom(fontScale)
			self:maxwidth(nwidth / fontScale - nameoffx * 2)
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
			self:halign(1):valign(0)
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
