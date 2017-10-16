--- Stretch a file with red diffusion.
local Color1 = color("1,0,0,1");

local t = Def.ActorFrame {
	LoadActor(Var "File1") .. {
		OnCommand=function(self)
			self:x(SCREEN_CENTER_X):y(SCREEN_CENTER_Y):scale_or_crop_background():diffuse(Color1):effectclock("music")
			-- Explanation in StretchNoLoop.lua.
			if self.GetTexture then
				self:GetTexture():rate(self:GetParent():GetUpdateRate())
			end
		end,
		GainFocusCommand=function(self)
			self:play()
		end;
		LoseFocusCommand=function(self)
			self:pause()
		end;
	};
};

return t;
