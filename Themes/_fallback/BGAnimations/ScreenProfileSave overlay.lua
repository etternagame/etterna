-- this used to be responsible for showing "Saving Profiles" after Evaluation, but we moved saving to before Evaluation
-- now it is responsible for clearing the chart leaderboard cache
return Def.ActorFrame {
	Def.Quad {
		InitCommand = function(self)
			self:Center():zoomto(SCREEN_WIDTH, 80):diffuse(color("0,0,0,0.5"))
		end,
	},
	Def.Actor {
		BeginCommand = function(self)
			if SCREENMAN:GetTopScreen():HaveProfileToSave() then
				self:sleep(1)
			end
			self:queuecommand("Load")
		end,
		LoadCommand = function()
			SCREENMAN:GetTopScreen():Continue()
		end,
	},
}