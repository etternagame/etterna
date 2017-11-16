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
for i=1,#ms.SkillSets+1 do 
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
					if (ActiveSS < #ms.SkillSets+1 and #SSQuery[activebound][ActiveSS] > 2) or #SSQuery[activebound][ActiveSS] > 3 then 
						SSQuery[activebound][ActiveSS] = numbershers[i]
					end
				end
			end
		end
		if SSQuery[activebound][ActiveSS] == "" then 
			SSQuery[activebound][ActiveSS] = "0"
		end
		FILTERMAN:SetSSFilter(tonumber(SSQuery[activebound][ActiveSS]), ActiveSS, activebound)
		whee:SongSearch("")		-- stupid workaround?
		MESSAGEMAN:Broadcast("UpdateFilter")
	end
end

local f = Def.ActorFrame{
	InitCommand=function(self)
		self:xy(frameX,frameY):halign(0)
	end,
	Def.Quad{
		InitCommand=function(self)
			self:zoomto(frameWidth,frameHeight):halign(0):valign(0):diffuse(color("#333333CC"))
		end,
	},
	Def.Quad{
		InitCommand=function(self)
			self:zoomto(frameWidth,offsetY):halign(0):valign(0):diffuse(getMainColor('frames')):diffusealpha(0.5)
		end,
	},
	LoadFont("Common Normal")..{
		InitCommand=function(self)
			self:xy(5,offsetY-9):zoom(0.6):halign(0):diffuse(getMainColor('positive')):settext("Filters (WIP)")
		end,
	},
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
			active = false
		end
	end,
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	MouseRightClickMessageCommand=function(self)
		ActiveSS = 0
		MESSAGEMAN:Broadcast("NumericInputEnded")
		MESSAGEMAN:Broadcast("UpdateFilter")
		SCREENMAN:set_input_redirected(PLAYER_1, false)
	end,
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX,frameY):zoom(0.3):halign(0)
		end,
		SetCommand=function(self) 
			self:settext("Left click on the filter value to set it.")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX,frameY+20):zoom(0.3):halign(0)
		end,
		SetCommand=function(self) 
			self:settext("Right click/Start/Back to cancel input.")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX,frameY+40):zoom(0.3):halign(0)
		end,
		SetCommand=function(self) 
			self:settext("Greyed out values are inactive.")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX,frameY+60):zoom(0.3):halign(0)
		end,
		SetCommand=function(self) 
			self:settext("Using both bounds creates a range.")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX,frameY+80):zoom(0.3):halign(0)
		end,
		SetCommand=function(self) 
			self:settext("'Highest Only' applies only to Mode: Or")
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175):zoom(textzoom):halign(0)
		end,
		SetCommand=function(self) 
			self:settextf("Max Rate:%5.1fx",FILTERMAN:GetMaxFilterRate())
		end,
		MaxFilterRateChangedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	Def.Quad{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175):zoomto(130,18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and active then
				FILTERMAN:SetMaxFilterRate(FILTERMAN:GetMaxFilterRate()+0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
		MouseRightClickMessageCommand=function(self)
			if isOver(self) and active then
				FILTERMAN:SetMaxFilterRate(FILTERMAN:GetMaxFilterRate()-0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175 + spacingY):zoom(textzoom):halign(0)
		end,
		SetCommand=function(self) 
			self:settextf("Min Rate:%5.1fx",FILTERMAN:GetMinFilterRate())
		end,
		MaxFilterRateChangedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	Def.Quad{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175 + spacingY):zoomto(130,18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and active then
				FILTERMAN:SetMinFilterRate(FILTERMAN:GetMinFilterRate()+0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
		MouseRightClickMessageCommand=function(self)
			if isOver(self) and active then
				FILTERMAN:SetMinFilterRate(FILTERMAN:GetMinFilterRate()-0.1)
				MESSAGEMAN:Broadcast("MaxFilterRateChanged")
				whee:SongSearch("")
			end
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175 + spacingY * 2):zoom(textzoom):halign(0)
		end,
		SetCommand=function(self)
			if FILTERMAN:GetFilterMode() then 
				self:settext("Mode: ".."AND")
			else
				self:settext("Mode: ".."OR")
			end
		end,
		FilterModeChangedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	Def.Quad{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175 + spacingY * 2):zoomto(120,18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and active then
				FILTERMAN:ToggleFilterMode()
				MESSAGEMAN:Broadcast("FilterModeChanged")
				whee:SongSearch("")
			end
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175 + spacingY * 3):zoom(textzoom):halign(0)
		end,
		SetCommand=function(self)
			if FILTERMAN:GetHighestSkillsetsOnly() then 
				self:settext("Highest Only: ".."ON")
			else
				self:settext("Highest Only: ".."OFF")
			end
			if FILTERMAN:GetFilterMode() then 
				self:diffuse(color("#666666"))
			else
				self:diffuse(color("#FFFFFF"))
			end
		end,
		FilterModeChangedMessageCommand=function(self)
			self:queuecommand("Set")
		end,
	},
	Def.Quad{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175 + spacingY * 3):zoomto(160,18):halign(0):diffusealpha(0)
		end,
		MouseLeftClickMessageCommand=function(self)
			if isOver(self) and active then
				FILTERMAN:ToggleHighestSkillsetsOnly()
				MESSAGEMAN:Broadcast("FilterModeChanged")
				whee:SongSearch("")
			end
		end,
	},
	LoadFont("Common Large")..{
		InitCommand=function(self)
			self:xy(frameX+frameWidth/2,175 + spacingY * 4):zoom(textzoom):halign(0):settext("")
		end,
		FilterResultsMessageCommand=function(self, msg)
			self:settext("Matches: "..msg.Matches.."/"..msg.Total)
		end
	},
}

local function CreateFilterInputBox(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(10):addy(175 + (i-1)*spacingY):halign(0):zoom(textzoom)
			end,
			SetCommand=function(self)
				self:settext( i == (#ms.SkillSets+1) and "Length" or ms.SkillSets[i])
			end	
		},
		Def.Quad{
			InitCommand=function(self)
				self:addx(i == (#ms.SkillSets+1) and 159 or 150):addy(175 + (i-1)*spacingY):zoomto(i == (#ms.SkillSets+1) and 27 or 18,18):halign(1)
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and active then
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
			UpdateFilterMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			NumericInputEndedMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			NumericInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(i == (#ms.SkillSets+1) and 159 or 150):addy(175 + (i-1)*spacingY):halign(1):maxwidth(60):zoom(textzoom)
			end,
			SetCommand=function(self)
				local fval = FILTERMAN:GetSSFilter(i,0)				-- lower bounds
				self:settext(fval)
				if fval <= 0 and ActiveSS ~= i then
					self:diffuse(color("#666666"))
				elseif activebound == 0 then
					self:diffuse(color("#FFFFFF"))
				end
			end,
			UpdateFilterMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			NumericInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		Def.Quad{
			InitCommand=function(self)
				self:addx(i == (#ms.SkillSets+1) and 193 or 175):addy(175 + (i-1)*spacingY):zoomto(i == (#ms.SkillSets+1) and 27 or 18,18):halign(1)
			end,
			MouseLeftClickMessageCommand=function(self)
				if isOver(self) and active then
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
			UpdateFilterMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			NumericInputEndedMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			NumericInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
		LoadFont("Common Large")..{
			InitCommand=function(self)
				self:addx(i == (#ms.SkillSets+1) and 193 or 175):addy(175 + (i-1)*spacingY):halign(1):maxwidth(60):zoom(textzoom)
			end,
			SetCommand=function(self)
				local fval = FILTERMAN:GetSSFilter(i,1)				-- upper bounds
				self:settext(fval)
				if fval <= 0 and ActiveSS ~= i then
					self:diffuse(color("#666666"))
				elseif activebound == 1 then
					self:diffuse(color("#FFFFFF"))
				end
			end,
			UpdateFilterMessageCommand=function(self)
				self:queuecommand("Set")
			end,
			NumericInputActiveMessageCommand=function(self)
				self:queuecommand("Set")
			end,
		},
	}
	return t
end

--reset button
f[#f+1] = Def.Quad{
    InitCommand=function(self)
    	self:xy(frameX+frameWidth-150,frameY+250):zoomto(60,20):halign(0.5):diffuse(getMainColor('frames')):diffusealpha(0)
    end,
    MouseLeftClickMessageCommand=function(self)
        if isOver(self) and active then
            FILTERMAN:ResetSSFilters()
            for i=1,#ms.SkillSets do
                SSQuery[0][i] = "0"
                SSQuery[1][i] = "0"
            end
            activebound = 0
			ActiveSS = 0
			MESSAGEMAN:Broadcast("UpdateFilter")
			whee:SongSearch("")
        end
    end
    }
f[#f+1] = LoadFont("Common Large") .. {
        InitCommand=function(self)
        	self:xy(frameX+frameWidth-150,frameY+250):halign(0.5):zoom(0.35)
        end,
        BeginCommand=function(self)
            self:settext( 'Reset' )
        end
    }

for i=1,(#ms.SkillSets+1) do 
	f[#f+1] = CreateFilterInputBox(i)
end
return f