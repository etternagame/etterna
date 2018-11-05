t = Def.ActorFrame {}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:zoomto(22, 22):diffuse(color("#ffffff")):diffusealpha(0.7)
	end
}

t[#t + 1] =
	Def.Quad {
	InitCommand = function(self)
		self:x(22):zoomto(66, 22):diffuse(color("#ffffff")):diffusealpha(0.5)
	end,
	MouseLeftClickMessageCommand = function(self)
		if isOver(self) then
			local s = GAMESTATE:GetCurrentSong()
			if s then
				local index = math.floor((getTrueY(self) - 70) / 24) + 1
				local allSteps = SongUtil.GetPlayableSteps(s)
				local steps = allSteps[index]
				if steps and steps ~= GAMESTATE:GetCurrentSteps(PLAYER_1) then
					local newidx = Difficulty:Compare(steps:GetDifficulty(), GAMESTATE:GetCurrentSteps(PLAYER_1):GetDifficulty())
					SCREENMAN:GetTopScreen():ChangeSteps(newidx)
				end
			end
		end
	end
}

return t
