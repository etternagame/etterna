-- Various player and stage info
local profileP1 = GetPlayerOrMachineProfile(PLAYER_1)
local PlayerFrameX = 0
local PlayerFrameY = SCREEN_HEIGHT - 50

local translated_info = {
	Judge = THEME:GetString("ScreenGameplay", "ScoringJudge"),
	Scoring = THEME:GetString("ScreenGameplay", "ScoringType")
}

local modstringTextSize = 0.4
local judgeDiffTextSize = 0.45
local difficultyTextSize = 0.45
local msdTextSize = 0.75
local scoringTextSize = 0.45

return Def.ActorFrame {
	Def.Sprite {
		InitCommand = function(self)
			self:halign(0):valign(0)
            self:xy(PlayerFrameX, PlayerFrameY)
		end,
		BeginCommand = function(self)
			self:finishtweening()
			self:Load(getAvatarPath(PLAYER_1))
			self:zoomto(50, 50)
		end
	},
	LoadFont("Common Large") .. {
        InitCommand = function(self)
            self:halign(0)
            self:xy(PlayerFrameX + 90, PlayerFrameY + 24)
            self:zoom(difficultyTextSize)
            self:maxwidth(120) -- one hundred twenty
        end,
        SetCommand = function(self)
            self:settext(getDifficulty(GAMESTATE:GetCurrentSteps():GetDifficulty()))
            self:diffuse(
                getDifficultyColor(
                    GetCustomDifficulty(
                        GAMESTATE:GetCurrentSteps():GetStepsType(),
                        GAMESTATE:GetCurrentSteps():GetDifficulty()
                    )
                )
            )
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            self:queuecommand("Set")
        end
    },
	LoadFont("Common Large") .. {
        InitCommand = function(self)
            self:halign(0)
            self:xy(PlayerFrameX + 52, PlayerFrameY + 28)
            self:zoom(msdTextSize)
            self:maxwidth(50)
        end,
        SetCommand = function(self)
            local meter = GAMESTATE:GetCurrentSteps():GetMSD(getCurRateValue(), 1)
            self:settextf("%05.2f", meter)
            self:diffuse(COLORS:colorByDifficulty(meter))
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            self:queuecommand("Set")
        end,
        CurrentRateChangedMessageCommand = function(self)
            self:queuecommand("Set")
        end,
        PracticeModeReloadMessageCommand = function(self)
            self:queuecommand("Set")
        end,
    },
	LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:halign(0)
            self:xy(PlayerFrameX + 91, PlayerFrameY + 39)
            self:zoom(modstringTextSize)
            self:maxwidth(SCREEN_WIDTH / 3 / modstringTextSize)
        end,
        BeginCommand = function(self)
            self:settext(getModifierTranslations(GAMESTATE:GetPlayerState():GetPlayerOptionsString("ModsLevel_Current")))
        end
    },
	LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:halign(0)
            self:xy(PlayerFrameX + 53, PlayerFrameY - 2)
            self:zoom(judgeDiffTextSize)
        end,
        BeginCommand = function(self)
            self:settextf("%s: %d", translated_info["Judge"], GetTimingDifficulty())
        end
    },
	LoadFont("Common Normal") .. {
        InitCommand = function(self)
            self:halign(0)
            self:xy(PlayerFrameX + 53, PlayerFrameY + 8)
            self:zoom(scoringTextSize)
        end,
        BeginCommand = function(self)
            self:settextf("%s: %s", translated_info["Scoring"], "Wife")
        end
    },
}