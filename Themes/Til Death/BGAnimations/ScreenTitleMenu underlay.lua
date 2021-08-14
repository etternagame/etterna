if IsSMOnlineLoggedIn() then
	CloseConnection()
end

local function input(event) -- for update button
	if event.type ~= "InputEventType_Release" then
		if event.DeviceInput.button == "DeviceButton_left mouse button" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
			MESSAGEMAN:Broadcast("MouseRightClick")
		end
	end
	return false
end

local t =
	Def.ActorFrame {
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end
}

local frameX = THEME:GetMetric("ScreenTitleMenu", "ScrollerX") - 10
local frameY = THEME:GetMetric("ScreenTitleMenu", "ScrollerY")

--Left gray rectangle
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(0, 0):halign(0):valign(0):zoomto(250, 900):diffuse(getTitleColor('BG_Left')):diffusealpha(1)
	end
}

--Right gray rectangle
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(250, 0):halign(0):valign(0):zoomto(1000, 900):diffuse(getTitleColor('BG_Right')):diffusealpha(1)
	end
}

--Light purple line
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(250, 0):halign(0):valign(0):zoomto(10, 900):diffuse(getTitleColor('Line_Left')):diffusealpha(1)
	end
}

--Dark purple line
t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:xy(260, 0):halign(0):valign(0):zoomto(10, 900):diffuse(getTitleColor('Line_Right')):diffusealpha(1)
	end
}

--Title text
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand=function(self)
			self:xy(125,frameY-82):zoom(0.7):align(0.5,1)
			self:diffusetopedge(Saturation(getMainColor("highlight"), 0.5))
			self:diffusebottomedge(Saturation(getMainColor("positive"), 0.8))
		end,
		OnCommand=function(self)
			self:settext("Etterna")
		end
}

--Theme text
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		InitCommand=function(self)
			self:xy(125,frameY-52):zoom(0.325):align(0.5,1)
			self:diffusetopedge(Saturation(getMainColor("highlight"), 0.5))
			self:diffusebottomedge(Saturation(getMainColor("positive"), 0.8))
		end,
		OnCommand=function(self)
			self:settext(getThemeName())
		end
}

--Version number
t[#t + 1] =
	LoadFont("Common Large") ..
	{
		Name = "Version",
		InitCommand=function(self)
			self:xy(125,frameY-35):zoom(0.25):align(0.5,1)
			self:diffusetopedge(Saturation(getMainColor("highlight"), 0.5))
			self:diffusebottomedge(Saturation(getMainColor("positive"), 0.8))
		end,
		BeginCommand = function(self)
			self:settext(GAMESTATE:GetEtternaVersion())
		end,
		MouseLeftClickMessageCommand=function(self)
			local tag = "urlnoexit,https://github.com/etternagame/etterna/releases/tag/v" .. GAMESTATE:GetEtternaVersion()
			if isOver(self) then
				GAMESTATE:ApplyGameCommand(tag)
			end
		end
	}

--game update button
local gameneedsupdating = false
local buttons = {x = 20, y = 20, width = 142, height = 42, fontScale = 0.35, color = getMainColor("frames")}
t[#t + 1] = Def.ActorFrame {
	InitCommand = function(self)
		self:xy(buttons.x,buttons.y)
	end,
	Def.Quad {
		InitCommand = function(self)
			self:zoomto(buttons.width, buttons.height):halign(0):valign(0):diffuse(buttons.color):diffusealpha(0)
			local latest = tonumber((DLMAN:GetLastVersion():gsub("[.]", "", 1)))
			local current = tonumber((GAMESTATE:GetEtternaVersion():gsub("[.]", "", 1)))
			if latest and latest > current then
				gameneedsupdating = true
			end
		end,
		OnCommand = function(self)
			if gameneedsupdating then
				self:diffusealpha(0.3)
			end
		end,
		MouseLeftClickMessageCommand = function(self)
			if isOver(self) and gameneedsupdating then
				GAMESTATE:ApplyGameCommand("urlnoexit,https://github.com/etternagame/etterna/releases;text,GitHub")
			end
		end
	},
	LoadFont("Common Large") ..
	{
		OnCommand = function(self)
			self:xy(1.7, 1):align(0,0):zoom(buttons.fontScale):diffuse(getMainColor("positive"))
			if gameneedsupdating then
				self:settext(THEME:GetString("ScreenTitleMenu", "UpdateAvailable"))
			else
				self:settext("")
			end
		end
	}
}

function mysplit(inputstr, sep)
	if sep == nil then
		sep = "%s"
	end
	local t = {}
	i = 1
	for str in string.gmatch(inputstr, "([^" .. sep .. "]+)") do
		t[i] = str
		i = i + 1
	end
	return t
end

local transformF = THEME:GetMetric("ScreenTitleMenu", "ScrollerTransform")
local scrollerX = THEME:GetMetric("ScreenTitleMenu", "ScrollerX")
local scrollerY = THEME:GetMetric("ScreenTitleMenu", "ScrollerY")
local scrollerChoices = THEME:GetMetric("ScreenTitleMenu", "ChoiceNames")
local _, count = string.gsub(scrollerChoices, "%,", "")
local choices = mysplit(scrollerChoices, ",")
local choiceCount = count + 1
local i
for i = 1, choiceCount do
	t[#t + 1] =
		Def.Quad {
		OnCommand = function(self)
			self:xy(scrollerX, scrollerY):zoomto(260, 16)
			transformF(self, 0, i, choiceCount)
			self:addx(SCREEN_CENTER_X - 20)
			self:addy(SCREEN_CENTER_Y - 20)
			self:diffusealpha(0)
		end,
		MouseLeftClickMessageCommand = function(self)
			if (isOver(self)) then
				SCREENMAN:GetTopScreen():playcommand("MadeChoicePlayer_1")
				SCREENMAN:GetTopScreen():playcommand("Choose")
				if choices[i] == "Multi" or choices[i] == "GameStart" then
					GAMESTATE:JoinPlayer()
				end
				GAMESTATE:ApplyGameCommand(THEME:GetMetric("ScreenTitleMenu", "Choice" .. choices[i]))
			end
		end
	}
end

return t
