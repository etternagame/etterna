local function input(event)	-- for update button
	if event.type ~= "InputEventType_Release" then
		if event.DeviceInput.button == "DeviceButton_left mouse button" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
			MESSAGEMAN:Broadcast("MouseRightClick")
		end
	end
	return false
end

local t = Def.ActorFrame{
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end,
}

local frameX = THEME:GetMetric("ScreenTitleMenu","ScrollerX")-10
local frameY = THEME:GetMetric("ScreenTitleMenu","ScrollerY")

--Left gray rectangle
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(0,0):halign(0):valign(0):zoomto(250,900):diffuse(color("#161515")):diffusealpha(1)
	end;
};

--Right gray rectangle
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(250,0):halign(0):valign(0):zoomto(1000,900):diffuse(color("#222222")):diffusealpha(1)
	end;
};

--Light purple line
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(250,0):halign(0):valign(0):zoomto(10,900):diffuse(color("#b87cf0")):diffusealpha(1)
	end;
};

--Dark purple line
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(260,0):halign(0):valign(0):zoomto(10,900):diffuse(color("#59307f")):diffusealpha(1)
	end;
};

t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(42,frameY-62):zoom(0.65):valign(1):halign(0):diffuse(color("#b87cf0"))
	end,
	OnCommand=function(self)
		self:settext(getThemeName())
	end,
}

-- lazy game update button -mina
local gameneedsupdating = false
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(22,134):zoomto(126,36):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(0)
		local latest = tonumber((DLMAN:GetLastVersion():gsub("[.]","",1)))
		local current = tonumber((GAMESTATE:GetEtternaVersion():gsub("[.]","",1)))
		if latest and latest > current then
			gameneedsupdating = true
		end
	end,
	OnCommand=function(self)
		if gameneedsupdating then
			self:diffusealpha(1)
		end
	end,
	MouseLeftClickMessageCommand=function(self)
		if isOver(self) and gameneedsupdating then
			GAMESTATE:ApplyGameCommand("urlnoexit,https://github.com/etternagame/etterna/releases;text,GitHub")
		end
	end
}

t[#t+1] = LoadFont("Common Large") .. {
	OnCommand=function(self)
		self:xy(25,148):halign(0):zoom(0.3):diffuse(getMainColor('positive'))
		if gameneedsupdating then
			self:settext("Update Available\nClick to Update")
		else
			self:settext("")
		end
	end
}

return t