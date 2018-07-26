local t = Def.ActorFrame{
	BeginCommand=function(self)
		self:queuecommand("Set"):visible(false)
		self:GetChild("GoalDisplay"):xy(10,60)
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(-500,0):diffusealpha(0)
	end,
	OnCommand=function(self)
		self:bouncebegin(0.2):xy(0,0):diffusealpha(1)
	end,
	SetCommand=function(self)
		self:finishtweening()
		if getTabIndex() == 6 then
			self:queuecommand("On")
			self:visible(true)
		else 
			self:queuecommand("Off")
		end
	end,
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
}


t[#t+1] = LoadActor("../goaldisplay")

return t