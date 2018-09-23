local t =
	Def.ActorFrame {
	LoadActor("_AnyRightFoot Hold Explosion") ..
		{
			HoldingOnCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "HoldingOnCommand"),
			HoldingOffCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "HoldingOffCommand"),
			InitCommand = function(self)
				self:playcommand("HoldingOff"):finishtweening()
			end
		},
	LoadActor("_AnyRightFoot Roll Explosion") ..
		{
			RollOnCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "RollOnCommand"),
			RollOffCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "RollOffCommand"),
			InitCommand = function(self)
				self:playcommand("RollOff"):finishtweening()
			end
		},
	LoadActor("_AnyRightFoot Tap Explosion Dim") ..
		{
			InitCommand = function(self)
				self:diffusealpha(0)
			end,
			W5Command = NOTESKIN:GetMetricA("GhostArrowDim", "W5Command"),
			W4Command = NOTESKIN:GetMetricA("GhostArrowDim", "W4Command"),
			W3Command = NOTESKIN:GetMetricA("GhostArrowDim", "W3Command"),
			W2Command = NOTESKIN:GetMetricA("GhostArrowDim", "W2Command"),
			W1Command = NOTESKIN:GetMetricA("GhostArrowDim", "W1Command"),
			HeldCommand = NOTESKIN:GetMetricA("GhostArrowDim", "HeldCommand"),
			JudgmentCommand = function(self)
				self:finishtweening()
			end,
			BrightCommand = function(self)
				self:visible(false)
			end,
			DimCommand = function(self)
				self:visible(true)
			end
		},
	LoadActor("AnyRightFoot HitMine Explosion") ..
		{
			InitCommand = function(self)
				self:blend("BlendMode_Add"):diffusealpha(0)
			end,
			HitMineCommand = NOTESKIN:GetMetricA("GhostArrowBright", "HitMineCommand")
		}
}
return t
