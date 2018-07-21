local minidoots = {"Novice", "Novice-Expanded", "Beginner", "Beginner-Expanded", "Intermediate", "Intermediate-Expanded", "Advanced", "Advanced-Expanded", "Expert", "Expert-Expanded"}
local diffcolors = {"#66ccff","#099948","#ddaa00","#ff6666","#c97bff"}
local pressingtab

local function input(event)
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
	return false
end

local width = SCREEN_WIDTH/3
local tzoom = 0.5
local packh = 42
local packgap = 4
local packspacing = packh + packgap
local offx = 10
local offy = 40

local o = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(offx + width/2, 0):halign(0.5):valign(0)
		self:GetChild("PacklistDisplay"):xy(offx + width/2, offy)
	end,
	OnCommand=function(self) SCREENMAN:GetTopScreen():AddInputCallback(input) end,
	Def.Quad{
		InitCommand=function(self)
			self:y(0):zoomto(width,500):diffuse(getMainColor('frames')):diffusealpha(1)
		end,
	},
	LoadFont("Common normal") .. {
		InitCommand=function(self)
			self:xy(-width/2,20):zoom(tzoom):halign(0)
		end,
		OnCommand=function(self)
			self:settext("Core bundles are diverse selections of packs that span a skill range.\nExpanded sets contain more files and are heftier downloads.")
		end
	}
}

local function UpdateHighlight(self)
	self:GetChild("Doot"):playcommand("Doot")
end

local function makedoots(i)
	local packinfo
	local t = Def.ActorFrame{
		InitCommand=function(self)
			self:y(packspacing*i + offy)
			self:SetUpdateFunction(UpdateHighlight)
		end,
		
		Def.Quad{
			Name="Doot",
			InitCommand=function(self)
				self:y(-12):zoomto(width,packh):valign(0):diffuse(color(diffcolors[math.ceil(i/2)]))
			end,
			OnCommand=function(self)
				self:queuecommand("SelectionChanged")
			end,
			DootCommand=function(self)
				if isOver(self) and ind ~= i then
					self:diffusealpha(0.75)
				elseif ind == i then
					self:diffusealpha(1)
				else
					self:diffusealpha(0.5)
				end
			end,
		},
		LoadFont("Common normal") .. {
			InitCommand=function(self)
				self:zoom(tzoom)
			end,
			OnCommand=function(self)
				self:settext(minidoots[i]:gsub("-Expanded", " (expanded)"))
			end
		},
		LoadFont("Common normal") .. {
			InitCommand=function(self)
				self:y(14):zoom(tzoom)
			end,
			OnCommand=function(self)
				local bundle = DLMAN:GetCoreBundle(minidoots[i]:lower())
				self:settextf("(%dmb)", bundle["TotalSize"])
			end
		}
	}
	return t
end

for i=1,#minidoots do
	o[#o+1] = makedoots(i)
end

o[#o+1] = LoadActor("packlistDisplay")


return o