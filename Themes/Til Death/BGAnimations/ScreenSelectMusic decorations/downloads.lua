local t = Def.ActorFrame{
	BeginCommand=function(self)
		self:queuecommand("Set"):visible(false)
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(-500,0):diffusealpha(0)
	end,
	OnCommand=function(self)
		self:bouncebegin(0.2):xy(0,0):diffusealpha(1)
	end,
	MouseRightClickMessageCommand=function(self)
		if update == true then
			inputting = 0
			curInput = 0
			MESSAGEMAN:Broadcast("DlInputEnded")
			MESSAGEMAN:Broadcast("NumericInputEnded")
			MESSAGEMAN:Broadcast("UpdatePacks")
		end
	end,
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 8 then
			SCREENMAN:SetNewScreen("ScreenPackDownloader")
			self:queuecommand("On")
			self:visible(true)
			update = true
		else 
			self:queuecommand("Off")
			update = false
		end
		MESSAGEMAN:Broadcast("DisplayAll")
	end,
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
}
return t