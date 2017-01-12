local active = true

local CtrlPressed = false
local function input(event)
	if event.type ~= "InputEventType_Release" and active then
		for i=1,6 do
			if event.DeviceInput.button == "DeviceButton_"..i and CtrlPressed == true then
				setTabIndex(i-1)
				MESSAGEMAN:Broadcast("TabChanged")
			end
		end
		if event.DeviceInput.button == "DeviceButton_left mouse button" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
			MESSAGEMAN:Broadcast("MouseRightClick")
		end
	end
	if event.DeviceInput.button == "DeviceButton_left ctrl" then
		if event.type == "InputEventType_Release" then
			CtrlPressed = false
		else
			CtrlPressed = true
		end
	end
	if event.DeviceInput.button == "DeviceButton_right ctrl" then
		if event.type == "InputEventType_Release" then
			CtrlPressed = false
		else
			CtrlPressed = true
		end
	end
	return false
end

local t = Def.ActorFrame{
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end,
	BeginCommand=function(self) resetTabIndex() end,
	PlayerJoinedMessageCommand=function(self) resetTabIndex() end,
	BeginningSearchMessageCommand=function(self) active = true end,
	EndingSearchMessageCommand=function(self) active = true end,
}

-- Just for debug
--[[
t[#t+1] = LoadFont("Common Normal") .. {
	InitCommand=cmd(xy,300,300;halign,0;zoom,2;diffuse,getMainColor(2));
	BeginCommand=cmd(queuecommand,"Set");
	SetCommand=function(self)
		self:settext(getTabIndex())
	end;
	CodeMessageCommand=cmd(queuecommand,"Set");
};
--]]
--======================================================================================

local tabNames = {"General","MSD","Score","Search","Profile","Other"} -- this probably should be in tabmanager.

local frameWidth = (SCREEN_WIDTH*(403/854))/(#tabNames-1)
local frameX = frameWidth/2
local frameY = SCREEN_HEIGHT-70

function tabs(index)
	local t = Def.ActorFrame{
		Name="Tab"..index;
		InitCommand=cmd(xy,frameX+((index-1)*frameWidth),frameY);
		BeginCommand=cmd(queuecommand,"Set");
		SetCommand=function(self)
			self:finishtweening()
			self:linear(0.1)
			--show tab if it's the currently selected one
			if getTabIndex() == index-1 then
				self:y(frameY)
				self:diffusealpha(1)
			else -- otherwise "Hide" them
				self:y(frameY)
				self:diffusealpha(0.65)
			end;
		end;
		TabChangedMessageCommand=cmd(queuecommand,"Set");
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set");
	};

	t[#t+1] = Def.Quad{
		Name="TabBG";
		InitCommand=cmd(y,2;valign,0;zoomto,frameWidth,20;diffusecolor,getMainColor('frames');diffusealpha,0.85);
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				setTabIndex(index-1)
				MESSAGEMAN:Broadcast("TabChanged")
			end;
		end;
	};
		
	t[#t+1] = LoadFont("Common Normal") .. {
		InitCommand=cmd(y,5;valign,0;zoom,0.45;diffuse,getMainColor('positive'));
		BeginCommand=cmd(queuecommand,"Set");
		SetCommand=function(self)
			self:settext(tabNames[index])
			if isTabEnabled(index) then
				self:diffuse(getMainColor('positive'))
			else
				self:diffuse(color("#666666"))
			end;
		end;
		PlayerJoinedMessageCommand=cmd(queuecommand,"Set");
	};
	return t
end;

--Make tabs
for i=1,#tabNames do
	t[#t+1] =tabs(i)
end;

return t
