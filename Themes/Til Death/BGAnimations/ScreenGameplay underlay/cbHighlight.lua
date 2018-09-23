--Highlights the lane in which a combo break has occured.

--Should be moved to colors.lua
local highlightColor = {
	-- Colors of Judgment highlights
	TapNoteScore_W1 = color("0.2,0.773,0.953"),
	TapNoteScore_W2 = color("1,0.8,0"),
	TapNoteScore_W3 = color("0.4,0.8,0.4"),
	TapNoteScore_W4 = color("0.35,0.46,0.73"),
	TapNoteScore_W5 = color("0.78,0.48,1"),
	TapNoteScore_Miss = color("0.85,0.33,0.33"),
	TapNoteScore_HitMine = color("0.85,0.33,0.33")
}

local enabled = {
	PlayerNumber_P1 = GAMESTATE:IsPlayerEnabled(PLAYER_1) and
		playerConfig:get_data(pn_to_profile_slot(PLAYER_1)).CBHighlight,
	PlayerNumber_P2 = GAMESTATE:IsPlayerEnabled(PLAYER_2) and
		playerConfig:get_data(pn_to_profile_slot(PLAYER_2)).CBHighlight
}

local arrowWidth = 64 -- until noteskin metrics are implemented...
local alpha = 0.75

local style = GAMESTATE:GetCurrentStyle()
local cols = style:ColumnsPerPlayer()
local styleType = ToEnumShortString(style:GetStyleType())

local numPlayers = GAMESTATE:GetNumPlayersEnabled()
local center1P = ((cols >= 6) or PREFSMAN:GetPreference("Center1Player"))

local cbThreshold = Enum.Reverse(TapNoteScore)[ComboContinue()]

local function laneHighlight(pn)
	local t = Def.ActorFrame {}
	local xpos = getNoteFieldPos(pn)

	for i = 1, cols do
		t[#t + 1] =
			Def.Quad {
			InitCommand = function(self)
				self:xy(
					xpos - (arrowWidth * (cols / 2) * getNoteFieldScale(pn)) + ((i - 1) * arrowWidth * getNoteFieldScale(pn)) +
						(getNoteFieldScale(pn) * arrowWidth / 2),
					0
				):zoomto(getNoteFieldScale(pn) * (arrowWidth - 4), SCREEN_HEIGHT / 4.5):valign(0):diffusealpha(0)
			end,
			BeginCommand = function(self)
				self:fadeleft(0.2)
				self:faderight(0.2)
			end,
			JudgmentMessageCommand = function(self, params)
				local notes = params.Notes
				if params.Player == pn and params.TapNoteScore and notes ~= nil and notes[i] ~= nil then
					if
						Enum.Reverse(TapNoteScore)[params.TapNoteScore] < cbThreshold and params.TapNoteScore ~= "TapNoteScore_None" and
							params.TapNoteScore ~= "TapNoteScore_AvoidMine" and
							params.TapNoteScore ~= "TapNoteScore_CheckpointMiss"
					 then
						self:stoptweening()
						self:visible(true)
						self:diffusealpha(0)
						self:linear(0.1)
						self:diffuse(highlightColor[params.TapNoteScore])
						self:diffusealpha(alpha)
						self:linear(0.25)
						self:diffusealpha(0)
					end
				end
			end
		}
	end

	return t
end

local t = Def.ActorFrame {}

for _, pn in pairs({PLAYER_1, PLAYER_2}) do
	if enabled[pn] then
		t[#t + 1] = laneHighlight(pn)
	end
end

return t
