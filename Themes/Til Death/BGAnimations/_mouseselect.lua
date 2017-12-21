
local whee
local top
local function mouseInput(event)
	if top:GetSelectionState() == 2 then
		return
	end
	
	local mouseX = INPUTFILTER:GetMouseX()
	local mouseY = INPUTFILTER:GetMouseY()
	
	if mouseX > capWideScale(370,500) and mouseX < SCREEN_WIDTH then
		if event.DeviceInput.button == "DeviceButton_left mouse button" and event.type == "InputEventType_FirstPress"then
			local n=0
			local m=1
			if mouseY > 220 and mouseY < 256 then
				m=0
			elseif mouseY > 256 and mouseY < 292 then
				m=1
				n=1
			elseif mouseY > 292 and mouseY < 328 then
				m=1
				n=2
			elseif mouseY > 328 and mouseY < 364 then
				m=1
				n=3
			elseif mouseY > 364 and mouseY < 400 then
				m=1
				n=4
			elseif mouseY > 400 and mouseY < 436 then
				m=1
				n=5
			elseif mouseY > 184 and mouseY < 220 then
				m=-1
				n=1
			elseif mouseY > 148 and mouseY < 184 then
				m=-1
				n=2
			elseif mouseY > 112 and mouseY < 148 then
				m=-1
				n=3
			elseif mouseY > 76 and mouseY < 112 then
				m=-1
				n=4
			elseif mouseY > 40 and mouseY <76 then
				m=-1
				n=5
			end
			
			local doot = whee:MoveAndCheckType(m*n)
			whee:Move(0)
			if m == 0 or doot == "WheelItemDataType_Section" then
				top:SelectCurrent(0)
			end
		elseif event.DeviceInput.button == "DeviceButton_right mouse button" and event.type == "InputEventType_FirstPress"then
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