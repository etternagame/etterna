local hoverAlpha = 0.6

local t = Def.ActorFrame {}

t[#t + 1] = Def.Quad {
	InitCommand = function(self)
		self:zoomto(20, 20):diffuse(color("#ffffff")):diffusealpha(0.7)
	end
}

t[#t + 1] = UIElements.QuadButton(1, 1) .. {
	InitCommand = function(self)
		self:zoomto(54, 20):diffuse(color("#ffffff")):diffusealpha(0.5):halign(0)
	end,
	MouseOverCommand = function(self)
		self:diffusealpha(hoverAlpha)
	end,
	MouseOutCommand = function(self)
		self:diffusealpha(1)
	end,
	MouseDownCommand = function(self, params)
		if params.event == "DeviceButton_left mouse button" and self:GetParent():GetParent():GetDiffuseAlpha() > 0 then
			local s = GAMESTATE:GetCurrentSong()
			if s then
				local idx = self:GetParent():GetParent():GetIndex() - self:GetParent():GetParent():GetParent():GetCurrentIndex()
				if idx ~= 0 then
					SCREENMAN:GetTopScreen():ChangeSteps(idx)
				end
			end
		end
	end
}

return t
