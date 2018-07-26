local collapsed = false
local t = Def.ActorFrame{
	BeginCommand=function(self)
		self:queuecommand("Set"):visible(false)
		self:GetChild("GoalDisplay"):xy(10,60)
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(-500,0):diffusealpha(0)
	end,
	OnCommand=function(self)
		self:bouncebegin(0.2):xy(SCREEN_LEFT,SCREEN_TOP - 10):diffusealpha(1)
	end,
	SetCommand=function(self)
		self:hurrytweening(1)
		if getTabIndex() == 6 then
			self:queuecommand("On")
			self:visible(true)
		elseif collapsed and getTabIndex() == 0 then
			self:queuecommand("On")
			self:visible(true)
		elseif collapsed and getTabIndex() ~= 0 then
			self:queuecommand("Off")
		elseif not collapsed then
			self:queuecommand("Off")
		end
	end,
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
	CollapseCommand=function(self)
		collapsed = true
		resetTabIndex()
		MESSAGEMAN:Broadcast("TabChanged")
	end,
	ExpandCommand=function(self)
		collapsed = true
		setTabIndex(6)
		MESSAGEMAN:Broadcast("TabChanged")
	end
}
t[#t+1] = LoadActor("../superscoreboard")

return t