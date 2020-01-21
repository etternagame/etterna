-- Various player and stage info, more text = fps drop so we should be sparing
local profileP1 = GetPlayerOrMachineProfile(PLAYER_1)
local PlayerFrameX = 0
local PlayerFrameY = SCREEN_HEIGHT - 50

local translated_info = {
	Judge = THEME:GetString("ScreenGameplay", "ScoringJudge"),
	Scoring = THEME:GetString("ScreenGameplay", "ScoringType")
}

local t =
	Def.ActorFrame {
	Def.Sprite {
		InitCommand = function(self)
			self:halign(0):valign(0):xy(PlayerFrameX, PlayerFrameY)
		end,
		BeginCommand = function(self)
			self:finishtweening()
			self:Load(getAvatarPath(PLAYER_1))
			self:zoomto(50, 50)
		end
	},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(PlayerFrameX + 90, PlayerFrameY + 24):halign(0):zoom(0.45):maxwidth(120):diffuse(getMainColor("positive"))
			end,
			SetCommand = function(self)
				self:settext(getDifficulty(GAMESTATE:GetCurrentSteps(PLAYER_1):GetDifficulty()))
				self:diffuse(
					getDifficultyColor(
						GetCustomDifficulty(
							GAMESTATE:GetCurrentSteps(PLAYER_1):GetStepsType(),
							GAMESTATE:GetCurrentSteps(PLAYER_1):GetDifficulty()
						)
					)
				)
			end,
			DoneLoadingNextSongMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	LoadFont("Common Large") ..
		{
			InitCommand = function(self)
				self:xy(PlayerFrameX + 52, PlayerFrameY + 28):halign(0):zoom(0.75):maxwidth(50)
			end,
			SetCommand = function(self)
				local meter = GAMESTATE:GetCurrentSteps(PLAYER_1):GetMSD(getCurRateValue(), 1)
				self:settextf("%05.2f", meter)
				self:diffuse(byMSD(meter))
			end,
			DoneLoadingNextSongMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			CurrentRateChangedMessageCommand = function(self)
				self:queuecommand("Set")
			end,
			PracticeModeReloadMessageCommand = function(self)
				self:queuecommand("Set")
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(PlayerFrameX + 91, PlayerFrameY + 39):halign(0):zoom(0.4):maxwidth(SCREEN_WIDTH * 0.8)
			end,
			BeginCommand = function(self)
				self:settext(getModifierTranslations(GAMESTATE:GetPlayerState(PLAYER_1):GetPlayerOptionsString("ModsLevel_Current")))
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(PlayerFrameX + 53, PlayerFrameY - 2):halign(0):zoom(0.45)
			end,
			BeginCommand = function(self)
				self:settextf("%s: %d", translated_info["Judge"], GetTimingDifficulty())
			end
		},
	LoadFont("Common Normal") ..
		{
			InitCommand = function(self)
				self:xy(PlayerFrameX + 53, PlayerFrameY + 8):halign(0):zoom(0.45)
			end,
			BeginCommand = function(self)
				self:settextf("%s: %s", translated_info["Scoring"], scoringToText(themeConfig:get_data().global.DefaultScoreType))
			end
		}
}
return t
