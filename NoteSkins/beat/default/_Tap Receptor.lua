local Reverse = string.find(GAMESTATE:GetPlayerState(pn):GetPlayerOptionsString("ModsLevel_Preferred"), "Reverse");
local sButton = Var "Button"

if sButton == "scratch"
	then column = "Red"

elseif sButton == "Key2"
	or sButton == "Key4"
	or sButton == "Key6"
	then column = "Blue"
else
	column = "White"
end

local t =
	Def.ActorFrame {
	--Judgeline
	Def.Sprite {
		Texture = column .. " Go Receptor",
		Frames=Sprite.LinearFrames(1,1),
		InitCommand = NOTESKIN:GetMetricA("ReceptorArrow", "InitCommand"),
		NoneCommand = NOTESKIN:GetMetricA("ReceptorArrow", "NoneCommand"),
		PressCommand = NOTESKIN:GetMetricA("ReceptorArrow", "PressCommand"),
		LiftCommand = NOTESKIN:GetMetricA("ReceptorArrow", "LiftCommand"),
		W5Command = NOTESKIN:GetMetricA("ReceptorArrow", "W5Command"),
		W4Command = NOTESKIN:GetMetricA("ReceptorArrow", "W4Command"),
		W3Command = NOTESKIN:GetMetricA("ReceptorArrow", "W3Command"),
		W2Command = NOTESKIN:GetMetricA("ReceptorArrow", "W2Command"),
		W1Command = NOTESKIN:GetMetricA("ReceptorArrow", "W1Command")
	},
	--Laser
	Def.Sprite {
		Texture = column .. " Laser",
		Frames=Sprite.LinearFrames(1,1),
		InitCommand = function(self)
			if not Reverse then
				self:addrotationz(180)
				self:addy(4)
			else
				self:addy(-4)
			end

			self:valign(1)
			self:diffusealpha(0)
			self:zoom(1)
			self:blend("BlendMode_Add")
		end,
		NoneCommand = NOTESKIN:GetMetricA("ReceptorArrow", "NoneCommand"),
		PressCommand = function(self)
			self:finishtweening()
			self:diffusealpha(1)
			self:zoomy(0.5)
			self:zoomx(1)
		end,
		LiftCommand = function(self)
			self:zoomy(0.75)
			self:decelerate(0.192)
			self:zoomx(0.33)
			self:diffusealpha(0)
		end
	}
	
}
return t