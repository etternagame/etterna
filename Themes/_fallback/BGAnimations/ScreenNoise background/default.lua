local t = Def.ActorFrame{
	LoadActor("noise")..{
		Name="Noise";
		InitCommand=function(self)
			self:Center():texturewrapping(true):SetTextureFiltering(false):zoom(2):diffusealpha(0)
		end;
		OnCommand=function(self)
			self:linear(1):diffusealpha(1)
		end;
	};
};

local function Update(self)
	local c = self:GetChild("Noise");
	local x = math.random();
	local y = math.random();
	c:customtexturerect(0+x,0+y,2+x,2+y);
end;
t.InitCommand = function(self)
	self:SetUpdateFunction(Update)
end;

return t;
