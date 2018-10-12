local Color1 = color(Var "Color1")
local Color2 = color(Var "Color2")

local t =
	Def.ActorFrame {
	LoadActor(Var "File1") ..
		{
			OnCommand = function(self)
				self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y):scale_or_crop_background():diffuse(Color1):effectclock("music")
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
		},
	LoadActor(Var "File2") ..
		{
			OnCommand = function(self)
				self:blend("BlendMode_Add"):x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y):scaletoclipped(SCREEN_WIDTH, SCREEN_HEIGHT):diffuse(
					Color2
				):effectclock("music")
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
