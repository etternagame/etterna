local numbershers = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}
local frameX = 10
local frameY = 45
local active = false
local whee
local spacingY = 20
local textzoom = 0.35
local ActiveSS = 0
local SSQuery = {}
local frameWidth = capWideScale(360,400)
local frameHeight = 350
local offsetX = 10
local offsetY = 20
for i=1,#ms.SkillSets do 
	SSQuery[i] = "0"
end

local function FilterInput(event)
	if event.type ~= "InputEventType_Release" and ActiveSS > 0 and active then
		if event.button == "Start" or event.button == "Back" then
			ActiveSS = 0
			MESSAGEMAN:Broadcast("NumericInputEnded")
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			SSQuery[ActiveSS] = SSQuery[ActiveSS]:sub(1, -2)
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			SSQuery[ActiveSS] = ""
		else
			for i=1,#numbershers do
				if event.DeviceInput.button == "DeviceButton_"..numbershers[i] then
					if SSQuery[ActiveSS] == "0" then 
						SSQuery[ActiveSS] = ""
					end
					SSQuery[ActiveSS] = SSQuery[ActiveSS]..numbershers[i]
				end
			end
		end
		if SSQuery[ActiveSS] == "" or #SSQuery[ActiveSS] > 2 then 
			SSQuery[ActiveSS] = "0"
		end
		whee:SetSkillsetFilter(tonumber(SSQuery[ActiveSS]), ActiveSS)
		MESSAGEMAN:Broadcast("UpdateFilter")
	end
end

local f = Def.ActorFrame{
	InitCommand=cmd(xy,frameX,frameY;halign,0),
	Def.Quad{InitCommand=cmd(zoomto,frameWidth,frameHeight;halign,0;valign,0;diffuse,color("#333333CC"))},
	Def.Quad{InitCommand=cmd(zoomto,frameWidth,offsetY;halign,0;valign,0;diffuse,getMainColor('frames');diffusealpha,0.5)},
	LoadFont("Common Normal")..{InitCommand=cmd(xy,5,offsetY-9;zoom,0.6;halign,0;diffuse,getMainColor('positive');settext,"Filters (WIP)")},
	OnCommand=function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		SCREENMAN:GetTopScreen():AddInputCallback(FilterInput)
		self:visible(false)
	end,
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 5 then
			self:visible(true)
			active = true
		else
			self:visible(false)
			self:queuecommand("Off")
		end
	end,
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	MouseRightClickMessageCommand=function(self)
		ActiveSS = 0
		MESSAGEMAN:Broadcast("UpdateFilter")
		SCREENMAN:set_input_redirected(PLAYER_1, false)
	end,
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX,frameY;zoom,0.3;halign,0),
		SetCommand=function(self) 
			self:settext("Left click on the filter value to set it.")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX,frameY+20;zoom,0.3;halign,0),
		SetCommand=function(self) 
			self:settext("Right click/Start/Back to cancel input.")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX,frameY+40;zoom,0.3;halign,0),
		SetCommand=function(self) 
			self:settext("Greyed out values are inactive.")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+frameWidth/2,175;zoom,textzoom;halign,0),
		SetCommand=function(self) 
			self:settext("Max Rate: "..1)
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+frameWidth/2,175 + spacingY;zoom,textzoom;halign,0),
		SetCommand=function(self) 
			self:settext("Mode: ".."Inclusive")
		end,
	},
		LoadFont("Common Large")..{
	InitCommand=cmd(xy,frameX+frameWidth/2,175 + spacingY*2;zoom,textzoom;halign,0),
		SetCommand=function(self) 
			self:settext("Highest SS: ".."False")
		end,
	},
}

local function CreateFilterInputBox(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large")..{
			InitCommand=cmd(addx,10;addy,175 + (i-1)*spacingY;halign,0;zoom,textzoom),
			SetCommand=cmd(settext, ms.SkillSets[i])
		},
		Def.Quad{
			InitCommand=cmd(addx,150;addy,175 + (i-1)*spacingY;zoomto,18,18;halign,1),
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) then
					ActiveSS = i
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if ActiveSS ~= i then
					self:diffuse(color("#000000"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
			NumericInputEndedMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large")..{
			InitCommand=cmd(addx,150;addy,175 + (i-1)*spacingY;halign,1;maxwidth,40;zoom,textzoom),
			SetCommand=function(self)
				local fval = whee:GetSkillsetFilter(i)
				self:settext(fval)
				if fval <= 0 and ActiveSS ~= i then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#FFFFFF"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
		},
		Def.Quad{
			InitCommand=cmd(addx,175;addy,175 + (i-1)*spacingY;zoomto,18,18;halign,1),
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and false then
					ActiveSS = i
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if ActiveSS ~= i or true then
					self:diffuse(color("#000000"))
				else
					self:diffuse(color("#666666"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
			NumericInputEndedMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large")..{
			InitCommand=cmd(addx,175;addy,175 + (i-1)*spacingY;halign,1;maxwidth,40;zoom,textzoom),
			SetCommand=function(self)
				local fval = 0 --whee:GetSkillsetFilter(i)
				self:settext(fval)
				if fval <= 0 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#FFFFFF"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
		},
	}
	return t
end

for i=1,#ms.SkillSets do 
	f[#f+1] = CreateFilterInputBox(i)
end

return f