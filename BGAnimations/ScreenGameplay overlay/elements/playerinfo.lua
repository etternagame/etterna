-- Various player and stage info
local profileP1 = GetPlayerOrMachineProfile(PLAYER_1)

local translated_info = {
	Judge = "Judge",
	Scoring = "Scoring",
}

local modstringTextSize = GAMEPLAY:getItemHeight("playerInfoModsText")
local judgeDiffTextSize = GAMEPLAY:getItemHeight("playerInfoJudgeText")
local difficultyTextSize = GAMEPLAY:getItemHeight("playerInfoMeterText")
local msdTextSize = GAMEPLAY:getItemHeight("playerInfoMSDText")
local scoringTextSize = GAMEPLAY:getItemHeight("playerInfoScoreTypeText")

local avatarSize = GAMEPLAY:getItemHeight("playerInfoAvatar")
local diffwidth = GAMEPLAY:getItemWidth("playerInfoMeter")
local diffXOffset = GAMEPLAY:getItemX("playerInfoMeterX")
local msdXOffset = GAMEPLAY:getItemX("playerInfoMSDX")
local msdwidth = diffXOffset - msdXOffset
local modsXOffset = GAMEPLAY:getItemX("playerInfoModsX")
local judgeXOffset = GAMEPLAY:getItemX("playerInfoJudgeX")
local scoreTypeXOffset = GAMEPLAY:getItemX("playerInfoScoreTypeX")

return Def.ActorFrame {
    Name = "PlayerInfo",
    InitCommand = function(self)
        self:xy(MovableValues.PlayerInfoX, MovableValues.PlayerInfoY)
        self:zoom(MovableValues.PlayerInfoZoom)
        registerActorToCustomizeGameplayUI(self)
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(modsXOffset + SCREEN_WIDTH/5, avatarSize)
            registerActorToColorConfigElement(self, "main", "PrimaryBackground")
            self:diffusealpha(0.1)
        end,
    },
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
            self:halign(0):valign(0)
            self:xy(diffXOffset, avatarSize/2 - 5)
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
            self:halign(0):valign(1)
            self:xy(msdXOffset, avatarSize - 2)
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
            self:halign(0):valign(1)
            self:xy(modsXOffset, avatarSize - 2)
            self:zoom(modstringTextSize)
            self:maxwidth(SCREEN_WIDTH / 5 / modstringTextSize)
        end,
        BeginCommand = function(self)
            self:settext(getModifierTranslations(GAMESTATE:GetPlayerState():GetPlayerOptionsString("ModsLevel_Current")))
        end
    },
	LoadFont("Common Normal") .. {
        Name = "Judge",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:xy(judgeXOffset, avatarSize/24)
            self:zoom(judgeDiffTextSize)
        end,
        BeginCommand = function(self)
            self:settextf("%s: %d", translated_info["Judge"], GetTimingDifficulty())
        end
    },
	LoadFont("Common Normal") .. {
        Name = "ScoreType",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:xy(scoreTypeXOffset, avatarSize/2 - avatarSize/8)
            self:zoom(scoringTextSize)
        end,
        BeginCommand = function(self)
            self:settextf("%s: %s", translated_info["Scoring"], "Wife")
        end
    },
}