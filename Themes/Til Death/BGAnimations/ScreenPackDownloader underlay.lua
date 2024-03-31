
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
	SearchName = THEME:GetString("ScreenPackDownloader", "SearchingName"),
	SizeExplanation = THEME:GetString("ScreenPackDownloader", "ExplainSizeLimit")
}

local width = SCREEN_WIDTH / 3
local fontScale = 0.5
local packh = 36
local packgap = 4
local packspacing = packh + packgap
local offx = 10
local offy = 40

local fx = SCREEN_WIDTH / 4.5 -- this isnt very smart alignment
local f0y = 160
local f1y = f0y + 40
local f2y = f1y + 40
local fdot = 24

local o = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(0, 0):halign(0.5):valign(0)
		self:GetChild("PacklistDisplay"):xy(SCREEN_WIDTH / 2.5 - offx, offy * 2 + 14)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(function(event)
		end)
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
	Def.Quad {
		InitCommand = function(self)
			self:xy(10, f0y - 30):halign(0):valign(0):zoomto(SCREEN_WIDTH / 3, f2y + 20):diffuse(color("#666666")):diffusealpha(
				0.4
			)
		end
	},

	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH / 6 + 10, 56):zoom(0.4):halign(0.5):maxwidth(SCREEN_WIDTH / 2)
			self:settext("nothing")
		end
	},
	UIElements.QuadButton(1, 1) .. {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH / 4 + 15, 40 + packh):zoomto(SCREEN_WIDTH / 6 - 10, packh - 2):valign(0):diffuse(
				color("#ffffff")
			):diffusealpha(0.4)
		end,
		MouseDownCommand = function(self, params)
			if params.event == "DeviceButton_left mouse button" then
				local dl = DLMAN:GetDownloads()[1]
				if dl then
					dl:Stop()
				end
			end
		end,
		MouseOverCommand = function(self)
			self:diffusealpha(0.8)
		end,
		MouseOutCommand = function(self)
			self:diffusealpha(0.4)
		end,
	},
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH / 4 + 15, 56 + packh):zoom(0.4):halign(0.5):maxwidth(SCREEN_WIDTH / 3)
			self:settext(translated_info["CancelCurrent"])
		end
	}
}

local nwidth = SCREEN_WIDTH / 2
local namex = nwidth
local namey = 40
local nhite = 22
local nameoffx = 20
-- name string search
o[#o + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(namex, namey):halign(0):valign(0)
	end,
	UIElements.QuadButton(1, 1) .. {
		InitCommand = function(self)
			self:zoomto(nwidth, nhite):halign(0):valign(0)
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
		InitCommand = function(self)
			self:x(nameoffx):halign(0):valign(0):maxwidth(nwidth / fontScale - nameoffx * 2):zoom(fontScale)
		end,
		SetCommand = function(self)
			local fval = nameInput
			self:settext(fval)
			diffuseIfActiveText(self, fval ~= "" or inputting == 1)
		end
	},
	LoadFont("Common Large") .. {
		InitCommand = function(self)
			self:zoom(fontScale):halign(1):valign(0)
			self:settextf("%s:", translated_info["SearchName"]) -- this being so far down is kinda awkward
		end
	},
	LoadFont("Common Normal") .. {
		InitCommand = function(self)
			self:xy(-90, 40)
			self:zoom(fontScale):halign(0):valign(0)
			self:settext(translated_info["SizeExplanation"])
		end
	}
}
o[#o + 1] = LoadActor("packlistDisplay")
return o
