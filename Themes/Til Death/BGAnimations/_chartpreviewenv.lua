-- chart preview using a notefield actor given by c++

local plotWidth, plotHeight = SCREEN_WIDTH * 0.15, SCREEN_HEIGHT
local plotX, plotY = SCREEN_WIDTH - plotWidth/2 - 25, -SCREEN_HEIGHT/2
local plotMargin = 4
local dotWidth = 4
local dotHeight = 4
local baralpha = 0.2
local bgalpha = 1
local textzoom = 0.5


-- button inputs because i dont want to use the metrics to do button inputs
local function input(event)
	if event.DeviceInput.button == "DeviceButton_right mouse button" or event.DeviceInput.button == "DeviceButton_left mouse button" then
		if event.type == "InputEventType_Release" then
			SCREENMAN:GetTopScreen():Cancel()
			MESSAGEMAN:Broadcast("DeletePreviewNoteField")
		end
	end
	if event.type ~= "InputEventType_Release" then
		if event.GameButton == "Back" or event.GameButton == "Start" then
			SCREENMAN:GetTopScreen():Cancel()
			MESSAGEMAN:Broadcast("DeletePreviewNoteField")
		end
	end
	return false
end


-- the fuel that lights this dumpsterfire
local p =
	Def.ActorFrame {
	OnCommand = function(self)
		SCREENMAN:GetTopScreen():AddInputCallback(input)
	end
}

-- the big black background box
p[#p + 1] =
	Def.Quad {
	ChartPreviewUpdateMessageCommand = function(self)
		self:zoomto(plotWidth, plotHeight):diffuse(color("0.05,0.05,0.05,0.05")):diffusealpha(
			bgalpha
		)
	end
}


-- 

return p