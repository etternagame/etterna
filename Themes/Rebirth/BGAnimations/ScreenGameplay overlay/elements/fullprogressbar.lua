-- progress bar that goes across the screen

local width = SCREEN_WIDTH / 2 - GAMEPLAY:getItemWidth("fullProgressBarWidthBeforeHalf")
local height = SCREEN_HEIGHT / 50
local alpha = 0.7
local isReplay = GAMESTATE:GetPlayerState():GetPlayerController() == "PlayerController_Replay" and not allowedCustomization

local translations = {
    MustBePaused = THEME:GetString("ScreenGameplay", "MustBePaused"),
}

local progressbarTextSize = GAMEPLAY:getItemHeight("fullProgressBarText")

local function bounds()
    local stps = GAMESTATE:GetCurrentSteps()
    return stps:GetFirstSecond(), stps:GetLastSecond()
end

-- ternary logic
-- will become either nothing or a slider
-- used to seek in replays
local replaySlider = isReplay and
    UIElements.QuadButton(1, 1) .. {
        Name = "SliderButtonArea",
        InitCommand = function(self)
            self:diffusealpha(0.3)
            self:zoomto(width, height)
        end,
        MouseHoldCommand = function(self, params)
            if params.event ~= "DeviceButton_left mouse button" then return end

            if not GAMESTATE:IsPaused() then
                TOOLTIP:SetText(translations["MustBePaused"])
                TOOLTIP:Show()
            end

            local localX = clamp(params.MouseX - self:GetTrueX() + width/2, 0, width)
            local localY = clamp(params.MouseY, 0, height)

            local lb, ub = bounds()
            local percentX = localX / width

            local posx = clamp(lb + (percentX * (ub - lb)), lb, ub)
            SCREENMAN:GetTopScreen():SetSongPosition(posx)
        end,
        MouseReleaseCommand = function(self)
            TOOLTIP:Hide()
        end,
        MouseUpCommand = function(self)
            TOOLTIP:Hide()
        end,
    } or
    Def.Actor {Name = "Nothing"}

return Def.ActorFrame {
    Name = "FullProgressBar",
    InitCommand = function(self)
        self:playcommand("SetUpMovableValues")
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            zoomInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.FullProgressBarX, MovableValues.FullProgressBarY)
        self:zoomto(MovableValues.FullProgressBarWidth, MovableValues.FullProgressBarHeight)
    end,

    replaySlider,
    Def.Quad {
        Name = "BG",
        InitCommand = function(self)
            self:zoomto(width, height)
            self:diffuse(COLORS:getGameplayColor("FullProgressBarBG"))
            self:diffusealpha(alpha)
        end
    },
    Def.SongMeterDisplay {
        Name = "Progress",
        InitCommand = function(self)
            self:SetUpdateRate(0.5)
        end,
        StreamWidth = width,
        Stream = Def.Quad {
            InitCommand = function(self)
                self:zoomy(height)
                self:diffuse(COLORS:getGameplayColor("FullProgressBar"))
                self:diffusealpha(alpha)
            end
        }
    },
    LoadFont("Common Normal") .. {
        Name = "Title",
        InitCommand = function(self)
            self:zoom(progressbarTextSize)
            self:maxwidth((width * 0.8) / progressbarTextSize)
        end,
        BeginCommand = function(self)
            self:settext(GAMESTATE:GetCurrentSong():GetDisplayMainTitle())
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            self:playcommand("Begin")
        end,
        PracticeModeReloadMessageCommand = function(self)
            self:playcommand("Begin")
        end
    },
    LoadFont("Common Normal") .. {
        Name = "SongLength",
        InitCommand = function(self)
            self:x(width / 2)
            self:halign(1)
            self:zoom(progressbarTextSize)
            self:maxwidth((width * 0.2) / progressbarTextSize)
        end,
        BeginCommand = function(self)
            local ttime = GetPlayableTime()
            self:settext(SecondsToMMSS(ttime))
            self:diffuse(colorByMusicLength(ttime))
        end,
        DoneLoadingNextSongMessageCommand = function(self)
            self:playcommand("Begin")
        end,
        CurrentRateChangedMessageCommand = function(self)
            self:playcommand("Begin")
        end,
        PracticeModeReloadMessageCommand = function(self)
            self:playcommand("Begin")
        end
    },
}
