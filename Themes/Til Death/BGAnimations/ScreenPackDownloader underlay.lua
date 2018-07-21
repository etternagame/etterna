
local filters = {"", "0", "0", "0", "0", "0", "0"}--1=name 2=lowerdiff 3=upperdiff 4=lowersize 5=uppersize

local curInput = ""
local inputting = 0 --1=name 2=lowerdiff 3=upperdiff 4=lowersize 5=uppersize 0=none
local function getFilter(index)
	return filters[index]
end

local pressingtab = false
local moving = false
local update = true

local function DlInput(event)
	if event.DeviceInput.button == "DeviceButton_tab" then
		if event.type == "InputEventType_FirstPress" then
			pressingtab = true
		elseif event.type == "InputEventType_Release" then
			pressingtab = false
		end
	elseif event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" then
		moving = true
		if pressingtab == true then
			MESSAGEMAN:Broadcast("WheelUpFast")
		else
			MESSAGEMAN:Broadcast("WheelUpSlow")
		end
	elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" then	
		moving = true
		if pressingtab == true then
			MESSAGEMAN:Broadcast("WheelDownFast")
		else
			MESSAGEMAN:Broadcast("WheelDownSlow")
		end
	elseif event.DeviceInput.button == 'DeviceButton_left mouse button' then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		end
	elseif event.DeviceInput.button == 'DeviceButton_right mouse button' then
		if event.type == "InputEventType_Release" then
			MESSAGEMAN:Broadcast("MouseLeftClick")
		end
	elseif moving == true then
		moving = false
	end
	if event.type ~= "InputEventType_Release" and update and inputting ~= 0 then
		local changed = false
		if event.button == "Start" then
			curInput = ""
			inputting = 0
			MESSAGEMAN:Broadcast("DlInputEnded")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			return true
		elseif event.button == "Back" then
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			curInput = curInput:sub(1, -2)
			changed = true
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			curInput = ""
			changed = true
		else
			if inputting == 2 or inputting == 3 or inputting == 4 or inputting == 5 then
				if tonumber(event.char) ~= nil then
					curInput = curInput..event.char
					changed = true
				end
			else
				if event.char and event.char:match("[%%%+%-%!%@%#%$%^%&%*%(%)%=%_%.%,%:%;%'%\"%>%<%?%/%~%|%w]") and event.char ~= "" then
					curInput = curInput..event.char
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
			MESSAGEMAN:Broadcast("UpdateFilterDisplays")
			return true
		end
	end
end

local function highlight(self)
	self:queuecommand("Highlight")
end

local function diffuseIfActive(self, cond)
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

local width = SCREEN_WIDTH/3
local fontScale = 0.5
local packh = 36
local packgap = 4
local packspacing = packh + packgap
local offx = 10
local offy = 40
local packlist
local ind = 0




local o = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(0, 0):halign(0.5):valign(0)
		self:GetChild("PacklistDisplay"):xy(SCREEN_WIDTH/2.5 - offx, offy * 2 + 14)
		packlist = DLMAN:GetThePackList()
		self:SetUpdateFunction(highlight)
	end,
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(DlInput) end,
	
	WheelUpSlowMessageCommand=function(self)
		self:queuecommand("PrevPage")
	end,
	WheelDownSlowMessageCommand=function(self)
		self:queuecommand("NextPage")
	end,
	UpdateFilterDisplaysMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	DlInputEndedMessageCommand=function(self) self:queuecommand("Set") end,
	DlInputActiveMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	
	
}

local function numFilter(i,x,y)
	return Def.ActorFrame{
		Def.Quad{
			InitCommand=function(self)
				self:addx(x):addy(y):zoomto(18,18):halign(1)
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and update then
					inputting = i
					curInput = ""
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if inputting == i then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			HighlightCommand=function(self)
				highlightIfOver(self)
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(x):addy(y):halign(1):maxwidth(40):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(i)
				self:settext(fval)
				if tonumber(fval) > 0 or inputting == i then
					self:diffuse(color("#FFFFFF"))
				else
					self:diffuse(color("#666666"))
				end
			end,
		}
	}
end
for i=2,3 do 
	o[#o+1] = numFilter(i, 40*i, 20 *i)
end



local nwidth = SCREEN_WIDTH/3
local namex = nwidth
local namey = 40
local nhite = 22

-- name string search
o[#o+1] = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(namex,namey):halign(0):valign(0)
	end,
	
	Def.Quad{
			InitCommand=function(self)
				self:zoomto(nwidth,nhite):halign(0):valign(0):diffuse(color("#666666"))
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) then
					inputting=1
					curInput = ""
					self:diffusealpha(activealpha)
					MESSAGEMAN:Broadcast("DlInputActive")
					MESSAGEMAN:Broadcast("NumericInputActive")
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				diffuseIfActive(self, inputting == 1)
			end,
			HighlightCommand=function(self)
				highlightIfOver(self)
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:zoom(fontScale):halign(1):valign(0):settext( "Search by name:")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:x(20):halign(0):valign(0):maxwidth(nwidth/fontScale - 40):zoom(fontScale)
			end,
			SetCommand=function(self)
				local fval= getFilter(1)
				self:settext(fval)
				diffuseIfActive(self, fval ~= "" or inputting == 1)
			end,
		},
		
}
o[#o+1] = LoadActor("packlistDisplay")
return o