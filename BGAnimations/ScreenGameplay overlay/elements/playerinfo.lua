-- Various player and stage info
local profileP1 = GetPlayerOrMachineProfile(PLAYER_1)
local PlayerFrameX = GAMEPLAY:getItemX("playerInfoFrameX")
local PlayerFrameY = SCREEN_HEIGHT - GAMEPLAY:getItemY("playerInfoFrameYFromBottom")

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
local diffYOffset = GAMEPLAY:getItemY("playerInfoMeterY")
local msdXOffset = GAMEPLAY:getItemX("playerInfoMSDX")
local msdYOffset = GAMEPLAY:getItemY("playerInfoMSDY")
local msdwidth = diffXOffset - msdXOffset
local modsXOffset = GAMEPLAY:getItemX("playerInfoModsX")
local modsYOffset = GAMEPLAY:getItemY("playerInfoModsY")
local judgeXOffset = GAMEPLAY:getItemX("playerInfoJudgeX")
local judgeYOffset = GAMEPLAY:getItemY("playerInfoJudgeY")
local scoreTypeXOffset = GAMEPLAY:getItemX("playerInfoScoreTypeX")
local scoreTypeYOffset = GAMEPLAY:getItemY("playerInfoScoreTypeY")

return Def.ActorFrame {
    Name = "PlayerInfoContainer",
    InitCommand = function(self)
        self:xy(PlayerFrameX, PlayerFrameY)
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
            self:maxwidth(SCREEN_WIDTH / 5 / modstringTextSize)
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