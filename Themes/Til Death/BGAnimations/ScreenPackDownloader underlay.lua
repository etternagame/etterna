local filters = {"", "0", "0", "0", "0", "0", "0"}
--1=name 2=lowerdiff 3=upperdiff 4=lowersize 5=uppersize

local curInput = ""
local inputting = 1 --1=name 2=lowerdiff 3=upperdiff 4=lowersize 5=uppersize 0=none

local function getFilter(index)
	return filters[index]
end

local function sendFilterAndSearchQuery()
	packlist:FilterAndSearch(
		tostring(filters[1]),
		tonumber(filters[2]),
		tonumber(filters[3]),
		tonumber(filters[4] * 1024 * 1024),
		tonumber(filters[5] * 1024 * 1024)
	)
end

local moving = false

local function DlInput(event)

	if (event.DeviceInput.button == "DeviceButton_mousewheel up" or event.button == "MenuUp" or event.button == "MenuLeft") and event.type == "InputEventType_FirstPress" then
		moving = true
		MESSAGEMAN:Broadcast("WheelUpSlow")
	elseif (event.DeviceInput.button == "DeviceButton_mousewheel down" or event.button == "MenuDown" or event.button == "MenuRight") and event.type == "InputEventType_FirstPress" then
		moving = true
		MESSAGEMAN:Broadcast("WheelDownSlow")
	elseif event.DeviceInput.button == "DeviceButton_left mouse button" then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		end
	elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("MouseRightClick")
		end
	elseif moving == true then
		moving = false
	end
	if event.type ~= "InputEventType_Release" and inputting ~= 0 then
		local changed = false
		if event.button == "Start" then
			curInput = ""
			inputting = 0
			MESSAGEMAN:Broadcast("UpdateFilterDisplays")
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			return true
		elseif event.button == "Back" then
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			SCREENMAN:GetTopScreen():Cancel()
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			curInput = curInput:sub(1, -2)
			changed = true
		elseif event.DeviceInput.button == "DeviceButton_delete" then
			curInput = ""
			changed = true
		elseif event.DeviceInput.button == "DeviceButton_space" then
			curInput = curInput .. " "
			changed = true
		else
			if inputting == 2 or inputting == 3 or inputting == 4 or inputting == 5 then
				if tonumber(event.char) ~= nil then
					curInput = curInput .. event.char
					changed = true
				end
			else
				if event.char and event.char:match('[%%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%\'%"%>%<%?%/%~%|%w]') and event.char ~= "" then
					curInput = curInput .. event.char
					changed = true
				end
			end
		end
		if changed then
			if inputting == 2 or inputting == 3 or inputting == 4 or inputting == 5 then
				if curInput == "" or not tonumber(curInput) then
					curInput = "0"
				end
			end
			filters[inputting] = curInput
			sendFilterAndSearchQuery()
			MESSAGEMAN:Broadcast("UpdateFilterDisplays")
			MESSAGEMAN:Broadcast("FilterChanged")
			return true
		end
	end
	if
		event.type ~= "InputEventType_Release" and inputting == 0 and curInput == "" and
			event.type == "InputEventType_FirstPress"
	 then -- quickstart the string search with enter if there is no query atm
		if event.button == "Start" then
			curInput = ""
			inputting = 1
			MESSAGEMAN:Broadcast("UpdateFilterDisplays")
			SCREENMAN:set_input_redirected(PLAYER_1, true)
			return true
		end
	end
end

local function highlight(self)
	self:queuecommand("Highlight")
end

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

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(highlightalpha)
	else
		self:diffusealpha(inactivealpha)
	end
end

local translated_info = {
	Filters = THEME:GetString("ScreenPackDownloader", "Filters"),
	AverageDiff = THEME:GetString("ScreenPackDownloader", "AverageDiff"),
	Size = THEME:GetString("ScreenPackDownloader", "Size"),
	EnterBundles = THEME:GetString("ScreenPackDownloader", "BundleSelectEntry"),
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

local o =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(0, 0):halign(0.5):valign(0)
		self:GetChild("PacklistDisplay"):xy(SCREEN_WIDTH / 2.5 - offx, offy * 2 + 14)
		self:SetUpdateFunction(highlight)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(DlInput)
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
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(fx * 0.9, f0y):zoom(fontScale):halign(0.5):valign(0)
				self:settextf("%s:", translated_info["Filters"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(fx, f1y):zoom(fontScale):halign(1):valign(0)
				self:settextf("%s:", translated_info["AverageDiff"])
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(fx, f2y):zoom(fontScale):halign(1):valign(0)
				self:settextf("%s:", translated_info["Size"])
			end
		},
	-- maybe we'll have more one day

	-- goes to bundles (funkied the xys to match bundle screen)
	Def.Quad {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH / 6 + 10, 40):zoomto(SCREEN_WIDTH / 3, packh - 2):valign(0):diffuse(color("#ffffff")):diffusealpha(
				0.4
			)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				SCREENMAN:SetNewScreen("ScreenBundleSelect")
			end
		end,
		HighlightCommand = function(self)
			if isOver(self) then
				self:diffusealpha(0.8)
			else
				self:diffusealpha(0.4)
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(SCREEN_WIDTH / 6 + 10, 56):zoom(0.4):halign(0.5):maxwidth(SCREEN_WIDTH / 2)
				self:settext(translated_info["EnterBundles"])
			end
		},
	--[[
	Def.Quad {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH / 12 + 5, 40 + packh):zoomto(SCREEN_WIDTH / 6 - 10, packh - 2):valign(0):diffuse(
				color("#ffffff")
			):diffusealpha(0.4)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				local dls = DLMAN:GetDownloads()
				for i, dl in ipairs(dls) do
					dl:Stop()
				end
			end
		end,
		HighlightCommand = function(self)
			if isOver(self) then
				self:diffusealpha(0.8)
			else
				self:diffusealpha(0.4)
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(SCREEN_WIDTH / 12 + 10, 56 + packh):zoom(0.4):halign(0.5):maxwidth(SCREEN_WIDTH / 3):settext(
					"Cancel all dls"
				)
			end
		},
	--]]
	Def.Quad {
		InitCommand = function(self)
			self:xy(SCREEN_WIDTH / 4 + 15, 40 + packh):zoomto(SCREEN_WIDTH / 6 - 10, packh - 2):valign(0):diffuse(
				color("#ffffff")
			):diffusealpha(0.4)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				local dl = DLMAN:GetDownloads()[1]
				if dl then
					dl:Stop()
				end
			end
		end,
		HighlightCommand = function(self)
			if isOver(self) then
				self:diffusealpha(0.8)
			else
				self:diffusealpha(0.4)
			end
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(SCREEN_WIDTH / 4 + 15, 56 + packh):zoom(0.4):halign(0.5):maxwidth(SCREEN_WIDTH / 3)
				self:settext(translated_info["CancelCurrent"])
			end
		}
}

local function numFilter(i, x, y)
	return Def.ActorFrame {
		InitCommand = function(self)
			self:xy(fx + 10, f0y):addx(x):addy(y)
		end,
		Def.Quad {
			InitCommand = function(self)
				self:zoomto(fdot, fdot):halign(0):valign(0)
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) then
					inputting = i
					curInput = ""
					self:GetParent():GetParent():queuecommand("Set")
					self:diffusealpha(activealpha)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand = function(self)
				diffuseIfActiveButton(self, inputting == i)
			end,
			HighlightCommand = function(self)
				highlightIfOver(self)
			end
		},
		LoadFont("Common Large") ..
			{
				InitCommand = function(self)
					self:addx(fdot):halign(1):valign(0):maxwidth(fdot / fontScale):zoom(fontScale)
				end,
				SetCommand = function(self)
					local fval = getFilter(i)
					self:settext(fval)
					diffuseIfActiveText(self, tonumber(fval) > 0 or inputting == i)
				end
			}
	}
end
for i = 2, 3 do
	o[#o + 1] = numFilter(i, 40 * (i - 2), f1y - f0y)
end
for i = 4, 5 do
	o[#o + 1] = numFilter(i, 40 * (i - 4), f2y - f0y)
end

local nwidth = SCREEN_WIDTH / 2
local namex = nwidth
local namey = 40
local nhite = 22
local nameoffx = 20
-- name string search
o[#o + 1] =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(namex, namey):halign(0):valign(0)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(nwidth, nhite):halign(0):valign(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				inputting = 1
				curInput = ""
				self:GetParent():GetParent():queuecommand("Set")
				self:diffusealpha(activealpha)
				SCREENMAN:set_input_redirected(PLAYER_1, true)
			end
		end,
		SetCommand = function(self)
			diffuseIfActiveButton(self, inputting == 1)
		end,
		HighlightCommand = function(self)
			highlightIfOver(self)
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:x(nameoffx):halign(0):valign(0):maxwidth(nwidth / fontScale - nameoffx * 2):zoom(fontScale)
			end,
			SetCommand = function(self)
				local fval = getFilter(1)
				self:settext(fval)
				diffuseIfActiveText(self, fval ~= "" or inputting == 1)
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:zoom(fontScale):halign(1):valign(0)
				self:settextf("%s:", translated_info["SearchName"]) -- this being so far down is kinda awkward
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(-90, 40)
				self:zoom(fontScale):halign(0):valign(0)
				self:settext(translated_info["SizeExplanation"])
			end
		}
}
o[#o + 1] = LoadActor("packlistDisplay")
return o
