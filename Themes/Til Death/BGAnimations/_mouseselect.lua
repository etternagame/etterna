
local whee
local top
local function mouseInput(event)
	if top:GetSelectionState() == 2 then
		return
	elseif event.DeviceInput.button == "DeviceButton_left mouse button" and event.type == "InputEventType_FirstPress"then
		if INPUTFILTER:GetMouseX() > capWideScale(370,500) then
			local n=0
			local m=1
			if INPUTFILTER:GetMouseY() > 220 and INPUTFILTER:GetMouseY() < 256 then
				m=0
			elseif INPUTFILTER:GetMouseY() > 256 and INPUTFILTER:GetMouseY() < 292 then
				m=1
				n=1
			elseif INPUTFILTER:GetMouseY() > 292 and INPUTFILTER:GetMouseY() < 328 then
				m=1
				n=2
			elseif INPUTFILTER:GetMouseY() > 328 and INPUTFILTER:GetMouseY() < 364 then
				m=1
				n=3
			elseif INPUTFILTER:GetMouseY() > 364 and INPUTFILTER:GetMouseY() < 400 then
				m=1
				n=4
			elseif INPUTFILTER:GetMouseY() > 400 and INPUTFILTER:GetMouseY() < 436 then
				m=1
				n=5
			elseif INPUTFILTER:GetMouseY() > 184 and INPUTFILTER:GetMouseY() < 220 then
				m=-1
				n=1
			elseif INPUTFILTER:GetMouseY() > 148 and INPUTFILTER:GetMouseY() < 184 then
				m=-1
				n=2
			elseif INPUTFILTER:GetMouseY() > 112 and INPUTFILTER:GetMouseY() < 148 then
				m=-1
				n=3
			elseif INPUTFILTER:GetMouseY() > 76 and INPUTFILTER:GetMouseY() < 112 then
				m=-1
				n=4
			elseif INPUTFILTER:GetMouseY() > 40 and INPUTFILTER:GetMouseY() <76 then
				m=-1
				n=5
			end
			
			local doot = whee:MoveAndCheckType(m*n)
			whee:Move(0)
			if m == 0 or doot == "WheelItemDataType_Section" then
				top:SelectCurrent(0)
			end
		end
	elseif event.DeviceInput.button == "DeviceButton_right mouse button" and event.type == "InputEventType_FirstPress"then
		if INPUTFILTER:GetMouseX() > capWideScale(370,500) then
			setTabIndex(7)
			MESSAGEMAN:Broadcast("TabChanged")
		end
	end
end


local t = Def.ActorFrame{
	BeginCommand=function(self)
		top = SCREENMAN:GetTopScreen()
		whee = top:GetMusicWheel()
		top:AddInputCallback(mouseInput)
	end,
}

return t