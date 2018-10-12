local Color1 = color(Var "Color1")

local t =
	Def.ActorFrame {
	LoadActor(Var "File1") ..
		{
			OnCommand = function(self)
				self:xy(SCREEN_CENTER_X, SCREEN_CENTER_Y)
				self:scale_or_crop_background()
				self:diffuse(Color1)
				self:effectclock("music")
				-- Explanation in StretchNoLoop.lua.
				if self.GetTexture then
					self:GetTexture():rate(self:GetParent():GetUpdateRate())
				end
			end,
			GainFocusCommand = function(self)
				self:play()
			end,
			LoseFocusCommand = function(self)
				self:pause()
			end
		}
}

return t
