local minidoots = {
	"Novice",
	"Novice-Expanded",
	"Beginner",
	"Beginner-Expanded",
	"Intermediate",
	"Intermediate-Expanded",
	"Advanced",
	"Advanced-Expanded",
	"Expert",
	"Expert-Expanded"
}
local diffcolors = {"#66ccff", "#099948", "#ddaa00", "#ff6666", "#c97bff"}
local moving

local function input(event)
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
	return false
end

local function highlight(self)
	self:queuecommand("Highlight")
end

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.6)
	else
		self:diffusealpha(1)
	end
end

local translated_info = {
	Selected = THEME:GetString("ScreenBundleSelect", "Selected Bundle"),
	AverageDiff = THEME:GetString("ScreenBundleSelect", "AverageDiff"),
	TotalSize = THEME:GetString("ScreenBundleSelect", "TotalSize"),
	MB = THEME:GetString("ScreenBundleSelect", "MB"),
	DownloadAll = THEME:GetString("ScreenBundleSelect", "DownloadAll"),
	GoBack = THEME:GetString("ScreenBundleSelect", "GoBack"),
	Expanded = THEME:GetString("ScreenBundleSelect", "Expanded"),
	Explanation = THEME:GetString("ScreenBundleSelect", "Explanation")
}

local width = SCREEN_WIDTH / 3
local tzoom = 0.5
local packh = 36
local packgap = 4
local packspacing = packh + packgap
local offx = 10
local offy = 40
local packtable
local ind = 0

local o =
	Def.ActorFrame {
	InitCommand = function(self)
		self:xy(offx + width / 2, 0):halign(0.5):valign(0)
		self:GetChild("PacklistDisplay"):xy(SCREEN_WIDTH / 2.5 - offx - (offx + width / 2), offy * 2 + 14):visible(false) --- uuuu messy... basically cancel out the x coord of the parent
		packlist = PackList:new()
		self:SetUpdateFunction(highlight)
	end,
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end,
	WheelUpSlowMessageCommand = function(self)
		self:queuecommand("PrevPage")
	end,
	WheelDownSlowMessageCommand = function(self)
		self:queuecommand("NextPage")
	end,
	MouseRightClickMessageCommand = function(self)
		SCREENMAN:GetTopScreen():Cancel()
	end,
	Def.Quad {
		InitCommand = function(self)
			self:y(0):zoomto(width, 500):diffuse(getMainColor("frames")):diffusealpha(1)
		end
	},
	LoadFont("Common normal") ..
		{
			InitCommand = function(self)
				self:xy(width / 2 + offx, 24):zoom(tzoom):halign(0)
			end,
			OnCommand = function(self)
				self:settext(translated_info["Explanation"])
			end
		},
	LoadFont("Common Large") ..
		{
			--selected bundle
			InitCommand = function(self)
				self:xy(width / 2 + offx, offy * 2 - 20):zoom(0.4):halign(0)
			end,
			PackTableRefreshCommand = function(self)
				self:settextf("%s: %s", translated_info["Selected"], minidoots[ind]:gsub("-Expanded", " ("..translated_info["Expanded"]..")")):diffuse(
					color(diffcolors[math.ceil(ind / 2)])
				)
			end
		},
	LoadFont("Common normal") ..
		{
			--avg diff
			InitCommand = function(self)
				self:xy(width / 2 + offx, offy * 2):zoom(tzoom + 0.1):halign(0):maxwidth(width / 2 / tzoom)
			end,
			PackTableRefreshCommand = function(self)
				self:settextf("%s: %0.2f", translated_info["AverageDiff"], packtable.AveragePackDifficulty):diffuse(byMSD(packtable.AveragePackDifficulty))
			end
		},
	LoadFont("Common normal") ..
		{
			--total size
			InitCommand = function(self)
				self:xy(width * 2 + width / 2 - 150, offy * 2):zoom(tzoom + 0.1):halign(1):maxwidth(width / 2 / tzoom)
			end,
			PackTableRefreshCommand = function(self)
				self:settextf("%s: %i(%s)", translated_info["TotalSize"], packtable.TotalSize, translated_info["MB"]):diffuse(byFileSize(packtable.TotalSize))
			end
		},
	LoadFont("Common normal") ..
		{
			--download all
			InitCommand = function(self)
				self:xy(width * 2 + width / 2 - 40, offy * 2):zoom(tzoom + 0.1):halign(1):maxwidth(width / 2 / tzoom)
			end,
			PackTableRefreshCommand = function(self)
				self:settext(translated_info["DownloadAll"])
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) then
					DLMAN:DownloadCoreBundle(minidoots[ind]:lower())
				end
			end,
			HighlightCommand = function(self)
				highlightIfOver(self)
			end
		},
	--return to normal search
	Def.Quad {
		InitCommand = function(self)
			self:y(offy):zoomto(width, packh - 2):valign(0):diffuse(color("#ffffff")):diffusealpha(0.4)
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) then
				SCREENMAN:SetNewScreen("ScreenPackDownloader")
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
	LoadFont("Common normal") ..
		{
			InitCommand = function(self)
				self:y(offy + 16):zoom(tzoom + 0.1):halign(0.5):maxwidth(width / 2 / tzoom):settext(translated_info["GoBack"])
			end
		}
}

local function UpdateHighlight(self)
	self:GetChild("Doot"):playcommand("Doot")
end

local function makedoots(i)
	local packinfo
	local t =
		Def.ActorFrame {
		InitCommand = function(self)
			self:y(packspacing * i + offy + 10)
			self:SetUpdateFunction(UpdateHighlight)
		end,
		Def.Quad {
			Name = "Doot",
			InitCommand = function(self)
				self:y(-12):zoomto(width, packh):valign(0):diffuse(color(diffcolors[math.ceil(i / 2)]))
			end,
			DootCommand = function(self)
				if isOver(self) and ind ~= i then
					self:diffusealpha(0.75)
				elseif ind == i then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.5)
				end
			end,
			MouseLeftClickMessageCommand = function(self)
				if isOver(self) then
					packlist:SetFromCoreBundle(minidoots[i]:lower())
					packtable = packlist:GetPackTable()
					self:GetParent():GetParent():queuecommand("PackTableRefresh") -- perhaps it would be best if the packlist broadcast instead - mina
					self:GetParent():GetParent():visible(true):GetChild("PacklistDisplay"):visible(true)
					ind = i
				end
			end
		},
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:zoom(tzoom)
				end,
				OnCommand = function(self)
					self:settext(minidoots[i]:gsub("-Expanded", " ("..translated_info["Expanded"]..")"))
				end
			},
		LoadFont("Common normal") ..
			{
				InitCommand = function(self)
					self:y(14):zoom(tzoom)
				end,
				OnCommand = function(self)
					local bundle = DLMAN:GetCoreBundle(minidoots[i]:lower())
					self:settextf("(%d%s)", bundle["TotalSize"], translated_info["MB"])
				end
			}
	}
	return t
end

for i = 1, #minidoots do
	o[#o + 1] = makedoots(i)
end

o[#o + 1] = LoadActor("packlistDisplay")

return o
