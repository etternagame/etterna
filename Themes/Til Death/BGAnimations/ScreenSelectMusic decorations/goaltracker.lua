local t = Def.ActorFrame{
	BeginCommand=function(self)
		self:queuecommand("Set"):visible(false)
		self:GetChild("GoalDisplay"):xy(10,60):visible(false)
	end,
	OffCommand=function(self)
		self:bouncebegin(0.2):xy(-500,0):diffusealpha(0)
	end,
	OnCommand=function(self)
		self:bouncebegin(0.2):xy(SCREEN_LEFT,SCREEN_TOP - 10):diffusealpha(1)
	end,
	SetCommand=function(self)
		self:finishtweening(1)
		if getTabIndex() == 6 then
			self:queuecommand("On")
			self:visible(true)								-- input filter has a get:visible check so it doesn't eat inputs if the element isn't displayed
			self:GetChild("GoalDisplay"):visible(true)		-- however it isn't recursive, so we set the child explicitly, leaving this here to remind myself 
		else												-- to look into changing the getvisible logic or adding a new recursive function maybe -mina
			self:queuecommand("Off")
			self:GetChild("GoalDisplay"):xy(10,60):visible(false)
		end
	end,
	TabChangedMessageCommand=function(self)
		self:queuecommand("Set")
	end,
}
t[#t+1] = LoadActor("../GoalDisplay")

return t