local function highlight(self)
	if self:IsVisible() then
		self:queuecommand("Highlight")
	end
end

local function highlightIfOver(self)
	if isOver(self) then
		self:diffusealpha(0.75)
	else
		self:diffusealpha(0.5)
	end
end

t = Def.ActorFrame {
	InitCommand=function(self)
		self:SetUpdateFunction(highlight)
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:zoomto(20, 20):diffuse(color("#ffffff")):diffusealpha(0.7)
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:zoomto(54, 20):diffuse(color("#ffffff")):diffusealpha(0.5):halign(0)
	end,
	HighlightCommand=function(self)
		highlightIfOver(self)
	end,
	MouseLeftClickMessageCommand = function(self)
		if isOver(self) and self:GetParent():GetParent():GetDiffuseAlpha() > 0 then
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
