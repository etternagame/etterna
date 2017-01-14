local numbershers = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}
local frameX = 10
local frameY = 45
local active = false
local whee
local spacingY = 20
local textzoom = 0.35
local ActiveSS = 0
local SSQuery = {}
SSQuery[0] = {}
SSQuery[1] = {}
local frameWidth = capWideScale(360,400)
local frameHeight = 350
local offsetX = 10
local offsetY = 20
local activebound = 0
for i=1,#ms.SkillSets do 
	SSQuery[0][i] = "0"
	SSQuery[1][i] = "0"
end

local function FilterInput(event)
	if event.type ~= "InputEventType_Release" and ActiveSS > 0 and active then
		if event.button == "Start" or event.button == "Back" then
			ActiveSS = 0
			MESSAGEMAN:Broadcast("NumericInputEnded")
			SCREENMAN:set_input_redirected(PLAYER_1, false)
			return true
		elseif event.DeviceInput.button == "DeviceButton_backspace" then
			SSQuery[activebound][ActiveSS] = SSQuery[activebound][ActiveSS]:sub(1, -2)
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			SSQuery[activebound][ActiveSS] = ""
		else
			for i=1,#numbershers do
				if event.DeviceInput.button == "DeviceButton_"..numbershers[i] then
					if SSQuery[activebound][ActiveSS] == "0" then 
						SSQuery[activebound][ActiveSS] = ""
					end
					SSQuery[activebound][ActiveSS] = SSQuery[activebound][ActiveSS]..numbershers[i]
					if #SSQuery[activebound][ActiveSS] > 2 then 
						SSQuery[activebound][ActiveSS] = numbershers[i]
					end
				end
			end
		end
		if SSQuery[activebound][ActiveSS] == "" then 
			SSQuery[activebound][ActiveSS] = "0"
		end
		GAMESTATE:SetSSFilter(tonumber(SSQuery[activebound][ActiveSS]), ActiveSS, activebound)
		whee:SongSearch("")		-- stupid workaround?
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
			MESSAGEMAN:Broadcast("NumericInputEnded")
			self:visible(false)
			self:queuecommand("Off")
		end
	end,
	TabChangedMessageCommand=cmd(queuecommand,"Set"),
	MouseRightClickMessageCommand=function(self)
		ActiveSS = 0
		MESSAGEMAN:Broadcast("NumericInputEnded")
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
	Def.Quad{
		InitCommand=cmd(xy,frameX+frameWidth/2+90,175;zoomto,40,20;halign,0;diffusealpha,0.5),
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				GAMESTATE:SetMaxFilterRate(GAMESTATE:GetMaxFilterRate()+0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
		MouseRightClickMessageCommand=function(self)
			if isOver(self) then
				GAMESTATE:SetMaxFilterRate(GAMESTATE:GetMaxFilterRate()-0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+frameWidth/2,175;zoom,textzoom;halign,0),
		SetCommand=function(self) 
			self:settextf("Max Rate:%5.1fx",GAMESTATE:GetMaxFilterRate())
		end,
		MaxFilterRateChangedMessageCommand=cmd(queuecommand,"Set"),
	},
	LoadFont("Common Large")..{
		InitCommand=cmd(xy,frameX+frameWidth/2,175 + spacingY;zoom,textzoom;halign,0),
		SetCommand=function(self)
			local mode = GAMESTATE:GetFilterMode()
			if mode then 
				self:settext("Mode: ".."AND")
			else
				self:settext("Mode: ".."OR")
			end
		end,
		FilterModeChangedMessageCommand=cmd(queuecommand,"Set"),
	},
	Def.Quad{
		InitCommand=cmd(xy,frameX+frameWidth/2+50,175 + spacingY;zoomto,40,20;halign,0;diffusealpha,0.5),
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) then
				GAMESTATE:ToggleFilterMode()
				MESSAGEMAN:Broadcast("FilterModeChanged")
				whee:SongSearch("")
			end
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
					activebound = 0
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if ActiveSS == i and activebound == 0 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
			NumericInputEndedMessageCommand=cmd(queuecommand,"Set"),
			NumericInputActiveMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large")..{
			InitCommand=cmd(addx,150;addy,175 + (i-1)*spacingY;halign,1;maxwidth,40;zoom,textzoom),
			SetCommand=function(self)
				local fval = GAMESTATE:GetSSFilter(i,0)				-- lower bounds
				self:settext(fval)
				if fval <= 0 and ActiveSS ~= i then
					self:diffuse(color("#666666"))
				elseif activebound == 0 then
					self:diffuse(color("#FFFFFF"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
			NumericInputActiveMessageCommand=cmd(queuecommand,"Set"),
		},
		Def.Quad{
			InitCommand=cmd(addx,175;addy,175 + (i-1)*spacingY;zoomto,18,18;halign,1),
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) then
					ActiveSS = i
					activebound = 1
					MESSAGEMAN:Broadcast("NumericInputActive")
					self:diffusealpha(0.1)
					SCREENMAN:set_input_redirected(PLAYER_1, true)
				end
			end,
			SetCommand=function(self)
				if ActiveSS == i and activebound == 1 then
					self:diffuse(color("#666666"))
				else
					self:diffuse(color("#000000"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
			NumericInputEndedMessageCommand=cmd(queuecommand,"Set"),
			NumericInputActiveMessageCommand=cmd(queuecommand,"Set"),
		},
		LoadFont("Common Large")..{
			InitCommand=cmd(addx,175;addy,175 + (i-1)*spacingY;halign,1;maxwidth,40;zoom,textzoom),
			SetCommand=function(self)
				local fval = GAMESTATE:GetSSFilter(i,1)				-- upper bounds
				self:settext(fval)
				if fval <= 0 and ActiveSS ~= i then
					self:diffuse(color("#666666"))
				elseif activebound == 1 then
					self:diffuse(color("#FFFFFF"))
				end
			end,
			UpdateFilterMessageCommand=cmd(queuecommand,"Set"),
			NumericInputActiveMessageCommand=cmd(queuecommand,"Set"),
		},
	}
	return t
end

for i=1,#ms.SkillSets do 
	f[#f+1] = CreateFilterInputBox(i)
end

return f