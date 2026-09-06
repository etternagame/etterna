--If a Command has "NOTESKIN:GetMetricA" in it, that means it gets the command from the metrics.ini, else use cmd(); to define command.
--If you dont know how "NOTESKIN:GetMetricA" works here is an explanation.
--NOTESKIN:GetMetricA("The [Group] in the metrics.ini", "The actual Command to fallback on in the metrics.ini");

--The NOTESKIN:LoadActor() just tells us the name of the image the Actor redirects on.
--Oh and if you wonder about the "Button" in the "NOTESKIN:LoadActor( )" it means that it will check for that direction.
--So you dont have to do "Down" or "Up" or "Left" etc for every direction which will save space ;)
local t =
	Def.ActorFrame {
	--Hold Explosion Commands
	NOTESKIN:LoadActor(Var "Button", "Hold Explosion") ..
		{
			HoldingOnCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "HoldingOnCommand"),
			HoldingOffCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "HoldingOffCommand"),
			InitCommand = function(self)
				self:playcommand("HoldingOff"):finishtweening()
			end
		},
	--Roll Explosion Commands
	NOTESKIN:LoadActor(Var "Button", "Roll Explosion") ..
		{
			RollOnCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "RollOnCommand"),
			RollOffCommand = NOTESKIN:GetMetricA("HoldGhostArrow", "RollOffCommand"),
			InitCommand = function(self)
				self:playcommand("RollOff"):finishtweening()
			end
		},
	--Dim Explosion Commands
	NOTESKIN:LoadActor(Var "Button", "Tap Explosion Dim") ..
		{
			InitCommand = function(self)
				self:diffusealpha(0)
			end,
			W5Command = NOTESKIN:GetMetricA("GhostArrowDim", "W5Command"),
			W4Command = NOTESKIN:GetMetricA("GhostArrowDim", "W4Command"),
			W3Command = NOTESKIN:GetMetricA("GhostArrowDim", "W3Command"),
			W2Command = NOTESKIN:GetMetricA("GhostArrowDim", "W2Command"),
			W1Command = NOTESKIN:GetMetricA("GhostArrowDim", "W1Command"),
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
	--yes yes I know I could do it in another way but I'm lazy and it works doesnt it ;>
	--This code give the Hold OK explosion Dim the same images as Bright
	NOTESKIN:LoadActor(Var "Button", "Tap Explosion Bright") ..
		{
			InitCommand = function(self)
				self:diffusealpha(0)
			end,
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
	--Bright Explosion Commands
	NOTESKIN:LoadActor(Var "Button", "Tap Explosion Bright") ..
		{
			InitCommand = function(self)
				self:diffusealpha(0)
			end,
			W5Command = NOTESKIN:GetMetricA("GhostArrowBright", "W5Command"),
			W4Command = NOTESKIN:GetMetricA("GhostArrowBright", "W4Command"),
			W3Command = NOTESKIN:GetMetricA("GhostArrowBright", "W3Command"),
			W2Command = NOTESKIN:GetMetricA("GhostArrowBright", "W2Command"),
			W1Command = NOTESKIN:GetMetricA("GhostArrowBright", "W1Command"),
			HeldCommand = NOTESKIN:GetMetricA("GhostArrowBright", "HeldCommand"),
			JudgmentCommand = function(self)
				self:finishtweening()
			end,
			BrightCommand = function(self)
				self:visible(true)
			end,
			DimCommand = function(self)
				self:visible(false)
			end
		},
	--Mine Explosion Commands
	NOTESKIN:LoadActor(Var "Button", "HitMine Explosion") ..
		{
			InitCommand = function(self)
				self:blend("BlendMode_Add"):diffusealpha(0)
			end,
			HitMineCommand = NOTESKIN:GetMetricA("GhostArrowBright", "HitMineCommand")
		}
}
return t
