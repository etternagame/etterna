local t = Def.ActorFrame {}
local screname
local whee
local mdown = false -- mouse down mouse up
local moving = false -- allow dragging while not over the required space
local prev = 0 -- previous wheel index moved to to prevent holding the mouse down causing a lot of clicky noises
t[#t + 1] = Def.ActorFrame {
	BeginCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(function(event)
			if event.DeviceInput.button == "DeviceButton_left mouse button" then
				if event.type ~= "InputEventType_Release" then
					mdown = true

					self:SetUpdateFunction(function(self)
						local mx = SCREEN_WIDTH - INPUTFILTER:GetMouseX()
						local my = SCREEN_HEIGHT - INPUTFILTER:GetMouseY()
						if moving or mx < 32 and mx > 0 and my < 440 and my > 48 then
							moving = true
							self:playcommand("ClickingMusicWheelScroller")
						end
					end)
				else
					mdown = false
					moving = false
					self:SetUpdateFunction(nil)
				end
			end
		end)
	end,

	Def.Quad {
		Name = "DootyMcBooty",
		BeginCommand = function(self)
			self:zoomto(32, 32):valign(0.634522134234)
			screname = SCREENMAN:GetTopScreen():GetName()
			if screname == "ScreenSelectMusic" or screname == "ScreenNetSelectMusic" then
				whee = SCREENMAN:GetTopScreen():GetMusicWheel()
			end
		end,
		ClickingMusicWheelScrollerCommand = function(self)
			if whee then
				local idx = whee:GetCurrentIndex()
				local num = whee:GetNumItems()
				local dum = math.min(math.max(0, INPUTFILTER:GetMouseY() - 45) / (SCREEN_HEIGHT - 103), 1)
				local newmove = notShit.round(num * dum) - idx
				if newmove ~= prev then
					prev = notShit.round(num * dum) - idx
					-- prevent looping around at the bottom
					if prev - num ~= idx and num - prev ~= idx then
						whee:Move(prev)
						whee:Move(0)
					end
				end
			end
		end
	}
}
return t