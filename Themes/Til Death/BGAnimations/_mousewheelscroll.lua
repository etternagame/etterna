
local moving = false
local whee
local pressingtab = false
local top

local function scrollInput(event)
	if event.DeviceInput.button == "DeviceButton_tab" then
		if event.type == "InputEventType_FirstPress" then
			pressingtab = true
		elseif event.type == "InputEventType_Release" then
			pressingtab = false
		end
	elseif event.DeviceInput.button == "DeviceButton_mousewheel up" and event.type == "InputEventType_FirstPress" and top:GetSelectionState() ~= 2 then
		moving = true
		if pressingtab == true then
			whee:Move(-3)	
		elseif whee:IsSettled() then
			whee:Move(-1)
		else
			whee:Move(-2)
		end
	elseif event.DeviceInput.button == "DeviceButton_mousewheel down" and event.type == "InputEventType_FirstPress" and top:GetSelectionState() ~= 2 then
		moving = true
		if pressingtab == true then
			whee:Move(3)	
		elseif whee:IsSettled() then
			whee:Move(1)
		else
			whee:Move(2)
		end
	elseif moving == true then
		whee:Move(0)
		moving = false
	end
end

local t = Def.ActorFrame{
	BeginCommand=function(self)
		whee = SCREENMAN:GetTopScreen():GetRoomWheel()
		top = SCREENMAN:GetTopScreen()
		top:AddInputCallback(scrollInput)
		self:visible(false)
	end,
}
return t
