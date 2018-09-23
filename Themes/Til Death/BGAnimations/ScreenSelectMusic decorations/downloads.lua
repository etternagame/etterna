local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, 0):diffusealpha(0)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(0, 0):diffusealpha(1)
	end,
	TabChangedMessageCommand = function(self)
		if getTabIndex() == 8 and SONGMAN:GetNumSongs() == 0 then
			SCREENMAN:SetNewScreen("ScreenBundleSelect")
		elseif getTabIndex() == 8 then
			SCREENMAN:SetNewScreen("ScreenPackDownloader")
		end
	end
}
return t
