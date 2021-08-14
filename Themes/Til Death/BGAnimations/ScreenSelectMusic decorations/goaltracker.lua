local t =
	Def.ActorFrame {
	BeginCommand = function(self)
		self:visible(false):queuecommand("Set")
		self:GetChild("GoalDisplay"):xy(10, 60)
	end,
	OffCommand = function(self)
		self:bouncebegin(0.2):xy(-500, SCREEN_TOP - 10):diffusealpha(0)
		self:sleep(0.04):queuecommand("Invis")
	end,
	InvisCommand= function(self)
		self:visible(false)
	end,
	OnCommand = function(self)
		self:bouncebegin(0.2):xy(SCREEN_LEFT, SCREEN_TOP - 10):diffusealpha(1)
	end,
	SetCommand = function(self)
		self:finishtweening(1)
		if getTabIndex() == 6 then
			self:queuecommand("On")
			self:visible(true)
		else
			self:queuecommand("Off")
		end
	end,
	TabChangedMessageCommand = function(self)
		self:queuecommand("Set")
	end
}
t[#t + 1] = LoadActor("../GoalDisplay")

return t
