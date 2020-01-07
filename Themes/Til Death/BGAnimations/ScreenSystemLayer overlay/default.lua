local t = Def.ActorFrame {}
-- Text
t[#t + 1] =
	Def.ActorFrame {
	Def.Quad {
		InitCommand = function(self)
			self:zoomtowidth(SCREEN_WIDTH):zoomtoheight(30):horizalign(left):vertalign(top):y(SCREEN_TOP):diffuse(
				color("0,0,0,0")
			)
		end,
		OnCommand = function(self)
			self:finishtweening():diffusealpha(0.85)
		end,
		OffCommand = function(self)
			self:sleep(3):linear(0.5):diffusealpha(0)
		end
	},
	Def.BitmapText {
		Font = "Common Normal",
		Name = "Text",
		InitCommand = function(self)
			self:maxwidth(SCREEN_WIDTH * 0.8):horizalign(left):vertalign(top):y(SCREEN_TOP + 10):x(SCREEN_LEFT + 10):diffusealpha(
				0
			)
		end,
		OnCommand = function(self)
			self:finishtweening():diffusealpha(1):zoom(0.5)
		end,
		OffCommand = function(self)
			self:sleep(3):linear(0.5):diffusealpha(0)
		end
	},
	SystemMessageMessageCommand = function(self, params)
		self:GetChild("Text"):settext(params.Message)
		self:playcommand("On")
		if params.NoAnimate then
			self:finishtweening()
		end
		self:playcommand("Off")
	end,
	HideSystemMessageMessageCommand = function(self)
		self:finishtweening()
	end
}

-- song reload
local www = 1366 * 0.8
local hhh = SCREEN_HEIGHT * 0.8
local rtzoom = 0.6

local function dooting(self)
	if self:IsVisible() then
		self:GetChild("BGQframe"):queuecommand("dooting")
	end
end

local translated_info = {
	ItemsDownloading = THEME:GetString("ScreenSystemLayerOverlay", "ItemsDownloading"),
	ItemsLeftInQueue = THEME:GetString("ScreenSystemLayerOverlay", "ItemsLeftInQueue")
}

local dltzoom = 0.5
-- download queue/progress
t[#t + 1] =
	Def.ActorFrame {
	PausingDownloadsMessageCommand=function(self)
		self:visible(false)
	end,
	ResumingDownloadsMessageCommand=function(self)
		self:visible(false)
	end,
	AllDownloadsCompletedMessageCommand = function(self)
		self:visible(false)
	end,
	DLProgressAndQueueUpdateMessageCommand = function(self)
		self:visible(true)
	end,
	BeginCommand = function(self)
		self:SetUpdateFunction(dooting)
		self:visible(false)
		self:x(www / 8 + 10):y(SCREEN_TOP + hhh / 8 + 10)
	end,
	Def.Quad {
		Name = "BGQframe",
		InitCommand = function(self)
			self:zoomto(www / 4, hhh / 4):diffuse(color("0.1,0.1,0.1,0.8"))
		end,
		dootingCommand = function(self)
			if isOver(self) then
				self:GetParent():x(SCREEN_WIDTH - self:GetParent():GetX())
			end
		end
	},
	Def.BitmapText {
		Font = "Common Normal",
		InitCommand = function(self)
			self:xy(-www / 8 + 10, -hhh / 8):diffusealpha(0.9):settext("5 items in queue:\ndoot\nmcscoot"):maxwidth(
				(www / 4 - 20) / dltzoom
			):halign(0):valign(0):zoom(dltzoom)
		end,
		DLProgressAndQueueUpdateMessageCommand = function(self, params)
			self:settextf("%s %s\n%s\n\n%s %s:\n%s",
				params.dlsize,
				translated_info["ItemsDownloading"],
				params.dlprogress,
				params.queuesize,
				translated_info["ItemsLeftInQueue"],
				params.queuedpacks
			)
			self:GetParent():GetChild("BGQframe"):zoomy(self:GetHeight() - hhh / 4 + 10)
		end
	}
}

t[#t + 1] =
	Def.ActorFrame {
	DFRStartedMessageCommand = function(self)
		self:visible(true)
	end,
	DFRFinishedMessageCommand = function(self, params)
		self:visible(false)
	end,
	BeginCommand = function(self)
		self:visible(false)
		self:x(www / 8 + 10):y(SCREEN_BOTTOM - hhh / 8 - 70)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(www / 4, hhh / 4):diffuse(color("0.1,0.1,0.1,0.8"))
		end
	},
	Def.BitmapText {
		Font = "Common Normal",
		InitCommand = function(self)
			self:diffusealpha(0.9):settext(""):maxwidth((www / 4 - 40) / rtzoom):zoom(rtzoom)
		end,
		DFRUpdateMessageCommand = function(self, params)
			self:settext(params.txt)
		end
	}
}

return t
