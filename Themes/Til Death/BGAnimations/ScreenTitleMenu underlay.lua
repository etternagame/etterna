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

t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:draworder(-300):xy(frameX,frameY):zoomto(SCREEN_WIDTH,160):halign(0):diffuse(getMainColor('highlight')):diffusealpha(0.15):diffusetopedge(color("0,0,0,0"))
	end	
}

t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:draworder(-300):xy(frameX,frameY-100):zoomto(SCREEN_WIDTH,160):halign(0):diffuse(getMainColor('highlight')):diffusealpha(0.15):diffusebottomedge(color("0,0,0,0"))
	end	
}

t[#t+1] = LoadFont("Common Large") .. {
	InitCommand=function(self)
		self:xy(10,frameY-180):zoom(0.65):valign(1):halign(0):diffuse(getDifficultyColor("Difficulty_Couple"))
	end,
	OnCommand=function(self)
		self:settext(getThemeName())
	end,
}

t[#t+1] = LoadActor(THEME:GetPathG("","_ring")) .. {
	InitCommand=function(self)
		self:xy(capWideScale(get43size(SCREEN_WIDTH-450),SCREEN_WIDTH-530),frameY-130):diffuse(getDifficultyColor("Difficulty_Couple")):diffusealpha(1):baserotationx(0)
	end	
}

-- lazy game update button -mina
local gameneedsupdating = false
t[#t+1] = Def.Quad{
	InitCommand=function(self)
		self:xy(22,134):zoomto(126,36):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(0)
		if tonumber((DLMAN:GetLastVersion():gsub("[.]","",1))) > tonumber((GAMESTATE:GetEtternaVersion():gsub("[.]","",1))) then
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