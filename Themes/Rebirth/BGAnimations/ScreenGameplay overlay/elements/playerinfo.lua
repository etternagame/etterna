-- Various player and stage info
local translations = {
    Judge = THEME:GetString("ScreenGameplay", "JudgeDifficulty"),
    Scoring = THEME:GetString("ScreenGameplay", "ScoringType"),
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

local bufferspace = 5

return Def.ActorFrame {
    Name = "PlayerInfo",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.PlayerInfoX, MovableValues.PlayerInfoY)
        self:zoom(MovableValues.PlayerInfoZoom)
    end,
    OrderedSetCommand = function(self)
        local diff = self:GetChild("Difficulty")
        local msd = self:GetChild("MSD")
        local mods = self:GetChild("ModString")

        msd:playcommand("Set")
        diff:playcommand("Set")
        mods:playcommand("Set")
    end,
    DoneLoadingNextSongMessageCommand = function(self)
        self:playcommand("OrderedSet")
    end,
    CurrentRateChangedMessageCommand = function(self)
        self:playcommand("OrderedSet")
    end,
    PracticeModeReloadMessageCommand = function(self)
        self:playcommand("OrderedSet")
    end,

    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoomto(modsXOffset + SCREEN_WIDTH/5, avatarSize)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryBackground")
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
            local xp = self:GetParent():GetChild("MSD"):GetZoomedWidth()
            self:xy(avatarSize + bufferspace + xp + bufferspace, avatarSize/2 - 5)
        end,
    },
    LoadFont("Common Large") .. {
        Name = "MSD",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:zoom(msdTextSize)
            self:maxwidth(msdwidth / msdTextSize)
        end,
        SetCommand = function(self)
            local meter = GAMESTATE:GetCurrentSteps():GetMSD(getCurRateValue(), 1)
            self:settextf("%05.2f", meter)
            self:diffuse(COLORS:colorByMSD(meter))
            self:xy(avatarSize + bufferspace, avatarSize - 2)
        end,
        
    },
    LoadFont("Common Normal") .. {
        Name = "ModString",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:zoom(modstringTextSize)
            self:maxwidth(SCREEN_WIDTH / 5 / modstringTextSize)
            self:diffusealpha(1)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
        end,
        SetCommand = function(self)
            local xp = self:GetParent():GetChild("MSD"):GetZoomedWidth()
            self:xy(avatarSize + bufferspace + xp + bufferspace, avatarSize - 2)
            self:settext(getModifierTranslations(GAMESTATE:GetPlayerState():GetPlayerOptionsString("ModsLevel_Current")))
        end,
    },
    LoadFont("Common Normal") .. {
        Name = "Judge",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:zoom(judgeDiffTextSize)
            self:diffusealpha(1)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
        end,
        BeginCommand = function(self)
            self:xy(avatarSize + bufferspace, avatarSize/24)
            self:settextf("%s: %d", translations["Judge"], GetTimingDifficulty())
        end
    },
    LoadFont("Common Normal") .. {
        Name = "ScoreType",
        InitCommand = function(self)
            self:halign(0):valign(1)
            self:zoom(scoringTextSize)
            self:diffusealpha(1)
            registerActorToColorConfigElement(self, "gameplay", "PrimaryText")
        end,
        BeginCommand = function(self)
            self:xy(avatarSize + bufferspace, avatarSize/2 - avatarSize/8)
            self:settextf("%s: %s", translations["Scoring"], "Wife")
        end
    },
}
