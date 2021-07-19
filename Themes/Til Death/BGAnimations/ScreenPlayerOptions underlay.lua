t = Def.ActorFrame {
	InitCommand=function(self)
		setenv("DifferentOptionsScreen",false)
		setenv("NewOptions","Main")
	end
}

t[#t + 1] =
	LoadActor(THEME:GetPathG("", "_OptionsScreen")) ..
	{
		OnCommand = function(self)
			self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT):Center():zoom(1):diffusealpha(1)
		end
	}

if SameColor(getMainColor("positive"),color("#9654FD")) then
	t[#t + 1] =
		LoadActor(THEME:GetPathG("", "_OptionsSidebar")) ..
		{
			OnCommand = function(self)
				self:zoomto(152, SCREEN_HEIGHT):align(0,0)
			end
		}
else
	t[#t + 1] =
		LoadActor(THEME:GetPathG("", "_OptionsGreySidebar")) ..
		{
			OnCommand = function(self)
				self:zoomto(152, SCREEN_HEIGHT):align(0,0)
				self:diffuse(ColorMultiplier(getMainColor("positive"),1.5))
			end
		}
	t[#t + 1] =
		LoadActor(THEME:GetPathG("", "_OptionsGreySidebar")) ..
		{
			OnCommand = function(self)
				self:zoomto(152, SCREEN_HEIGHT):align(0,0)
				self:diffuse(ColorMultiplier(getMainColor("positive"),0.75))
				self:diffusealpha(0.2):blend(Blend.Add)
			end
		}
end


return t
