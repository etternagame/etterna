t = Def.ActorFrame {}

local frameX = THEME:GetMetric("ScreenTitleMenu", "ScrollerX") - 10
local frameY = THEME:GetMetric("ScreenTitleMenu", "ScrollerY")

t[#t + 1] =
	LoadActor(THEME:GetPathG("", "_OptionsScreen")) ..
	{
		OnCommand = function(self)
			self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):Center():zoom(1):diffusealpha(1)
		end
	}

local x = 75
local y = 500
if GetScreenAspectRatio() > 1.7 then
	x = 310
	y = 500
end
t[#t + 2] =
	LoadActor(THEME:GetPathG("", "_OptionsActor")) ..
	{
		OnCommand = function(self)
			self:zoomto(x, y):y(SCREEN_CENTER_Y):diffusealpha(1)
		end
	}

return t
