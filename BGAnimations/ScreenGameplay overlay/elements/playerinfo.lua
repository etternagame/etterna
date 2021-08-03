-- Various player and stage info
local profileP1 = GetPlayerOrMachineProfile(PLAYER_1)
local PlayerFrameX = 0
local PlayerFrameY = SCREEN_HEIGHT - 50 / GAMEPLAY_SIZING_RATIO

local translated_info = {
	Judge = "Judge",
	Scoring = "Scoring",
}

local modstringTextSize = 0.4 / GAMEPLAY_SIZING_RATIO
local judgeDiffTextSize = 0.45 / GAMEPLAY_SIZING_RATIO
local difficultyTextSize = 0.45 / GAMEPLAY_SIZING_RATIO
local msdTextSize = 1.1 / GAMEPLAY_SIZING_RATIO -- something weird about this
local scoringTextSize = 0.45 / GAMEPLAY_SIZING_RATIO

-- a lot of really bad magic numbers
-- clean this
local avatarSize = 50 / GAMEPLAY_SIZING_RATIO
local diffwidth = 120 / GAMEPLAY_SIZING_RATIO -- width as a magic number is very bad
local diffXOffset = 90 / GAMEPLAY_SIZING_RATIO
local diffYOffset = 24 / GAMEPLAY_SIZING_RATIO
local msdXOffset = 52 / GAMEPLAY_SIZING_RATIO
local msdYOffset = 28 / GAMEPLAY_SIZING_RATIO
local msdwidth = diffXOffset - msdXOffset
local modsXOffset = 91 / GAMEPLAY_SIZING_RATIO
local modsYOffset = 39 / GAMEPLAY_SIZING_RATIO -- lack of width
local judgeXOffset = 53 / GAMEPLAY_SIZING_RATIO
local judgeYOffset = -2 / GAMEPLAY_SIZING_RATIO -- lack of width
local scoreTypeXOffset = 53 / GAMEPLAY_SIZING_RATIO
local scoreTypeYOffset = 8 / GAMEPLAY_SIZING_RATIO -- lack of width

return Def.ActorFrame {
    Name = "PlayerInfoContainer",
    InitCommand = function(self)
        self:xy(PlayerFrameX, PlayerFrameY)
    end,

	Def.Sprite {
        Name = "Avatar",
		InitCommand = function(self)
			self:halign(0):valign(0)
		end,
		BeginCommand = function(self)
			self:finishtweening()
			self:Load(getAvatarPath(PLAYER_1))
			self:zoomto(avatarSize, avatarSize)
		end
	},
	LoadFont("Common Large") .. {
        Name = "Difficulty",
        InitCommand = function(self)
            self:halign(0)
            self:xy(diffXOffset, diffYOffset)
            self:zoom(difficultyTextSize)
            self:maxwidth(diffwidth)
        end,
        SetCommand = function(self)
            self:settext(getDifficulty(GAMESTATE:GetCurrentSteps():GetDifficulty()))
            self:diffuse(
                colorByDifficulty(
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
        Name = "MSD",
        InitCommand = function(self)
            self:halign(0)
            self:xy(msdXOffset, msdYOffset)
            self:zoom(msdTextSize)
            self:maxwidth(msdwidth / msdTextSize)
        end,
        SetCommand = function(self)
            local meter = GAMESTATE:GetCurrentSteps():GetMSD(getCurRateValue(), 1)
            self:settextf("%05.2f", meter)
            self:diffuse(COLORS:colorByMSD(meter))
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
        Name = "ModString",
        InitCommand = function(self)
            self:halign(0)
            self:xy(modsXOffset, modsYOffset)
            self:zoom(modstringTextSize)
            self:maxwidth(SCREEN_WIDTH / 3 / modstringTextSize)
        end,
        BeginCommand = function(self)
            self:settext(getModifierTranslations(GAMESTATE:GetPlayerState():GetPlayerOptionsString("ModsLevel_Current")))
        end
    },
	LoadFont("Common Normal") .. {
        Name = "Judge",
        InitCommand = function(self)
            self:halign(0)
            self:xy(judgeXOffset, judgeYOffset)
            self:zoom(judgeDiffTextSize)
        end,
        BeginCommand = function(self)
            self:settextf("%s: %d", translated_info["Judge"], GetTimingDifficulty())
        end
    },
	LoadFont("Common Normal") .. {
        Name = "ScoreType",
        InitCommand = function(self)
            self:halign(0)
            self:xy(scoreTypeXOffset, scoreTypeYOffset)
            self:zoom(scoringTextSize)
        end,
        BeginCommand = function(self)
            self:settextf("%s: %s", translated_info["Scoring"], "Wife")
        end
    },
}