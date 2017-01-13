local numbershers = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}
local frameX = 10
local frameY = 180+capWideScale(get43size(120),120)
local active = false
local whee
local filterspacing = 20
local filtertextzoom = 0.35
local ActiveSS = 0
local SSQuery = {}
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
			SSQuery[ActiveSS] = SSQuery[ActiveSS]:sub(1, -2)								-- remove the last element of the string
		elseif event.DeviceInput.button == "DeviceButton_delete"  then
			SSQuery[ActiveSS] = ""
		else
			for i=1,#numbershers do														-- add standard characters to string
				if event.DeviceInput.button == "DeviceButton_"..numbershers[i] then
					if SSQuery[ActiveSS] == "0" then 
						SSQuery[ActiveSS] = ""
					end
					SSQuery[ActiveSS] = SSQuery[ActiveSS]..numbershers[i]
				end
			end
		end
		if SSQuery[ActiveSS] == "" then 
			SSQuery[ActiveSS] = "0"
		end
		MESSAGEMAN:Broadcast("UpdateFilter")
		if SSQuery[ActiveSS] ~= "" then
			whee:SetSkillsetFilter(tonumber(SSQuery[ActiveSS]), ActiveSS)
		end
	end
end

local f = Def.ActorFrame{
	InitCommand=cmd(xy,frameX+30,frameY-50;halign,0),
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
	end,
}

local function CreateFilterInputBox(i)
	local t = Def.ActorFrame{
		LoadFont("Common Large")..{
			InitCommand=cmd(addy,(i-1)*filterspacing;halign,0;zoom,filtertextzoom),
			SetCommand=cmd(settext, ms.SkillSets[i])
		},
		Def.Quad{
			InitCommand=cmd(addx,135;addy,(i-1)*filterspacing;zoomto,18,18;halign,1),
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
			InitCommand=cmd(addx,135;addy,(i-1)*filterspacing;halign,1;maxwidth,40;zoom,filtertextzoom),
			SetCommand=function(self)
				self:settext(SSQuery[i])
				if tonumber(SSQuery[i]) <= 0 then
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