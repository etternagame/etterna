local moving = false
local whee
local pressingtab = false
local top

local function scrollInput(event)
	if top:GetName() == "ScreenSelectmusic" then
		if top:GetSelectionState() == 2 then
			return
		end	
	elseif event.DeviceInput.button == "DeviceButton_tab" then
		if event.type == "InputEventType_FirstPress" then
			pressingtab = true
		elseif event.type == "InputEventType_Release" then
			pressingtab = false
		end
	elseif event.type == "InputEventType_FirstPress" then
		if event.DeviceInput.button == "DeviceButton_mousewheel up" then
			moving = true
			if pressingtab == true and not whee:IsSettled() then
				whee:Move(-2)
			else
				whee:Move(-1)
			end
		elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
			moving = true
			if pressingtab == true and not whee:IsSettled() then
				whee:Move(2)
			else
				whee:Move(1)
			end
		end
	elseif moving == true then
		whee:Move(0)
		moving = false
	end

	local mouseX = INPUTFILTER:GetMouseX()
	local mouseY = INPUTFILTER:GetMouseY()
	
	if mouseX > capWideScale(370, 500) and mouseX < SCREEN_WIDTH - 32 then
		if event.DeviceInput.button == "DeviceButton_left mouse button" and event.type == "InputEventType_FirstPress" then
			local n = 0
			local m = 1
			if mouseY > 220 and mouseY < 256 then
				m = 0
			elseif mouseY > 256 and mouseY < 292 then
				m = 1
				n = 1
			elseif mouseY > 292 and mouseY < 328 then
				m = 1
				n = 2
			elseif mouseY > 328 and mouseY < 364 then
				m = 1
				n = 3
			elseif mouseY > 364 and mouseY < 400 then
				m = 1
				n = 4
			elseif mouseY > 400 and mouseY < 436 then
				m = 1
				n = 5
			elseif mouseY > 184 and mouseY < 220 then
				m = -1
				n = 1
			elseif mouseY > 148 and mouseY < 184 then
				m = -1
				n = 2
			elseif mouseY > 112 and mouseY < 148 then
				m = -1
				n = 3
			elseif mouseY > 76 and mouseY < 112 then
				m = -1
				n = 4
			elseif mouseY > 40 and mouseY < 76 then
				m = -1
				n = 5
			end

			local doot = whee:MoveAndCheckType(m * n)
			whee:Move(0)
			if m == 0 or doot == "WheelItemDataType_Section" then
				top:SelectCurrent(0)
			end
		elseif event.DeviceInput.button == "DeviceButton_right mouse button" and event.type == "InputEventType_FirstPress" then
			local tind = getTabIndex()
			setTabIndex(7)
			MESSAGEMAN:Broadcast("TabChanged", {from = tind, to = 7})
		end
	end
	return false
end

local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		top = SCREENMAN:GetTopScreen()
		top:AddInputCallback(scrollInput)
		self:visible(false)
	end
}
return t
