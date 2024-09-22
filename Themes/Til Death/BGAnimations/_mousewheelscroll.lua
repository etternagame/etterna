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
	    local DI_Button = event.DeviceInput.button

		if DI_Button == "DeviceButton_mousewheel up" then
			moving = true
			if pressingtab == true and not whee:IsSettled() then
				whee:Move(-2)
			else
				whee:Move(-1)
			end
		elseif DI_Button == "DeviceButton_mousewheel down" then
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

	return false
end

local t = Def.ActorFrame {
	BeginCommand = function(self)
		whee = SCREENMAN:GetTopScreen():GetMusicWheel()
		top = SCREENMAN:GetTopScreen()
		top:AddInputCallback(scrollInput)
		self:visible(false)
	end
}
return t
