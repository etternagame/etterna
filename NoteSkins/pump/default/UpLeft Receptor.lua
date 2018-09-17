local player = Var "Player" or GAMESTATE:GetMasterPlayerNumber()

local function Beat(self)
	-- too many locals
	local this = self:GetChildren()
	local playerstate = GAMESTATE:GetPlayerState(player)
	local songposition = playerstate:GetSongPosition() -- GAMESTATE:GetSongPosition()

	local beat = songposition:GetSongBeat() -- GAMESTATE:GetSongBeat()

	local part = beat % 1
	part = clamp(part, 0, 0.5)
	local eff = scale(part, 0, 0.5, 1, 0)
	if (songposition:GetDelay() or false) and part == 0 then
		eff = 0
	end
	if beat < 0 then
		eff = 0
	end
	this.Glow:diffusealpha(eff)
end

return Def.ActorFrame {
	-- COMMANDS --
	InitCommand = function(self)
		self:SetUpdateFunction(Beat)
	end,
	-- LAYERS --
	NOTESKIN:LoadActor("Center", "Outline Receptor") ..
		{
			Name = "Outline Full",
			Condition = Var "Button" == "Center" and GAMESTATE:GetCurrentStyle():GetStepsType() ~= "StepsType_Pump_Halfdouble"
			--InitCommand=cmd(x,96);
		},
	NOTESKIN:LoadActor("DownLeft", "Outline Receptor") ..
		{
			Name = "Outline Half",
			Condition = Var "Button" == "DownLeft" and GAMESTATE:GetCurrentStyle():GetStepsType() == "StepsType_Pump_Halfdouble"
			--InitCommand=cmd(x,96);
		},
	NOTESKIN:LoadActor(Var "Button", "Ready Receptor") ..
		{
			Name = "Base",
			Frames = {
				{Frame = 0}
			},
			PressCommand = function(self)
				self:finishtweening():linear(0.05):zoom(0.9):linear(0.1):zoom(1)
			end
		},
	NOTESKIN:LoadActor(Var "Button", "Ready Receptor") ..
		{
			Name = "Glow",
			Frames = {
				{Frame = 1}
			},
			InitCommand = function(self)
				self:blend("BlendMode_Add")
			end,
			PressCommand = function(self)
				self:finishtweening():linear(0.05):zoom(0.9):linear(0.1):zoom(1)
			end
		}
	--[[
	NOTESKIN:LoadActor(Var "Button", "Ready Receptor")..{
		Name="Tap";
		Frames = { Frame = 2 };
		InitCommand=function(self)
			self:zoom(1):diffusealpha(0):glow(1,1,1,0)
		end;
		--NOTESKIN:GetMetricA(Var "Button", "TapInitCommand");
		--
		PressCommand=function(self)
			self:finishtweening():glow(1,1,1,1):zoom(1):linear(0.2):glow(1,1,1,0):zoom(1.2)
		end;
		--NOTESKIN:GetMetricA(Var "Button", "TapHeldCommand");
		--
	};
	--]]
}
