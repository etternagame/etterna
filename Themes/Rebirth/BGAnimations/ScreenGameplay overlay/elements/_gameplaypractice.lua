-- this file is responsible for practice-mode only behavior
-- mostly it handles the additional inputs and functionality surrounding the chord density graph
-- right clicking or pressing InsertCoin to set bookmarks, etc

local musicratio = 1
local width = 280
local height = 53 / 555 * SCREEN_HEIGHT
local cd
local loopStartPos
local loopEndPos

local bookmarkWidth = 1 / 1080 * SCREEN_WIDTH
local bookmarkAlpha = 1
local regionAlpha = 0.5

local bookmarkColor = COLORS:getGameplayColor("PracticeBookmark")
local regionColor = COLORS:getGameplayColor("PracticeRegion")

local function handleRegionSetting(positionGiven)
    -- don't allow a negative region
    -- internally it is limited to -2
    -- the start delay is 2 seconds, so limit this to 0
    if positionGiven < 0 then return end

    -- first time starting a region
    if not loopStartPos and not loopEndPos then
        loopStartPos = positionGiven
        MESSAGEMAN:Broadcast("RegionSet")
        return
    end

    -- reset region to bookmark only if double right click
    if positionGiven == loopStartPos or positionGiven == loopEndPos then
        loopEndPos = nil
        loopStartPos = positionGiven
        MESSAGEMAN:Broadcast("RegionSet")
        SCREENMAN:GetTopScreen():ResetLoopRegion()
        return
    end

    -- measure the difference of the new pos from each end
    local startDiff = math.abs(positionGiven - loopStartPos)
    local endDiff = startDiff + 0.1
    if loopEndPos then
        endDiff = math.abs(positionGiven - loopEndPos)
    end

    -- use the diff to figure out which end to move

    -- if there is no end, then you place the end
    if not loopEndPos then
        if loopStartPos < positionGiven then
            loopEndPos = positionGiven
        elseif loopStartPos > positionGiven then
            loopEndPos = loopStartPos
            loopStartPos = positionGiven
        else
            -- this should never happen
            -- but if it does, reset to bookmark
            loopEndPos = nil
            loopStartPos = positionGiven
            MESSAGEMAN:Broadcast("RegionSet")
            SCREENMAN:GetTopScreen():ResetLoopRegion()
            return
        end
    else
        -- closer to the start, move the start
        if startDiff < endDiff then
            loopStartPos = positionGiven
        else
            loopEndPos = positionGiven
        end
    end
    SCREENMAN:GetTopScreen():SetLoopRegion(loopStartPos, loopEndPos)
    MESSAGEMAN:Broadcast("RegionSet", {loopLength = loopEndPos-loopStartPos})
end

local t = Def.ActorFrame {
    Name = "GameplayPracticeController",
    InitCommand = function(self)
    end,
    BeginCommand = function(self)
        musicratio = GAMESTATE:GetCurrentSteps():GetLastSecond() / width

        SCREENMAN:GetTopScreen():AddInputCallback(function(event)
            if event.type == "InputEventType_Release" then
                if event.DeviceInput.button == "DeviceButton_left mouse button" then
                    MESSAGEMAN:Broadcast("MouseLeftClick")
                elseif event.DeviceInput.button == "DeviceButton_right mouse button" then
                    MESSAGEMAN:Broadcast("MouseRightClick")
                end
            elseif event.type == "InputEventType_FirstPress" then
                if event.DeviceInput.button == "DeviceButton_backspace" then
                    if loopStartPos ~= nil then
                        SCREENMAN:GetTopScreen():SetSongPositionAndUnpause(loopStartPos, 1, true)
                    else
                        SCREENMAN:GetTopScreen():SetSongPositionAndUnpause(0, 1, true)
                    end
                elseif event.button == "EffectUp" then
                    SCREENMAN:GetTopScreen():AddToRate(0.05)
                elseif event.button == "EffectDown" then
                    SCREENMAN:GetTopScreen():AddToRate(-0.05)
                elseif event.button == "Coin" then
                    handleRegionSetting(SCREENMAN:GetTopScreen():GetSongPosition())
                elseif event.DeviceInput.button == "DeviceButton_mousewheel up" then
                    if GAMESTATE:IsPaused() then
                        local pos = SCREENMAN:GetTopScreen():GetSongPosition()
                        local dir = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse() and 1 or -1
                        local nextpos = pos + dir * 0.05
                        if loopEndPos ~= nil and nextpos >= loopEndPos then
                            handleRegionSetting(nextpos + 1)
                        end
                        SCREENMAN:GetTopScreen():SetSongPosition(nextpos, 0, false)
                    end
                elseif event.DeviceInput.button == "DeviceButton_mousewheel down" then
                    if GAMESTATE:IsPaused() then
                        local pos = SCREENMAN:GetTopScreen():GetSongPosition()
                        local dir = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse() and 1 or -1
                        local nextpos = pos - dir * 0.05
                        if loopEndPos ~= nil and nextpos >= loopEndPos then
                            handleRegionSetting(nextpos + 1)
                        end
                        SCREENMAN:GetTopScreen():SetSongPosition(nextpos, 0, false)
                    end
                end
            end
            return false
        end)
    end,
    PracticeModeReloadMessageCommand = function(self)
        musicratio = GAMESTATE:GetCurrentSteps():GetLastSecond() / width
    end,

    -- invisible button covering the entire screen
    -- right clicking anywhere that hits it (anywhere but the graph) pauses music
    UIElements.QuadButton(1, 1) .. {
        Name = "PauseArea",
        InitCommand = function(self)
            self:halign(0):valign(0)
            self:z(1)
            self:diffusealpha(0)
            self:zoomto(SCREEN_WIDTH, SCREEN_HEIGHT)
        end,
        MouseDownCommand = function(self, params)
            if params.event == "DeviceButton_right mouse button" then
                local top = SCREENMAN:GetTopScreen()
                if top then
                    top:TogglePause()
                end
            end
        end,
    }
}

-- Load the CDGraph with a forced width parameter.
t[#t + 1] = LoadActorWithParams("../../chorddensitygraph.lua", {sizing = {
    Width = width,
    Height = height,
    NPSThickness = 1.5,
    TextSize = 0.45,
}}) .. {
    BeginCommand = function(self)
        -- properly rename this actor to overrite the default name coming from chorddensitygraph.lua
        self:name("PracticeCDGraph")

        self:playcommand("SetUpMovableValues")
        self:playcommand("LoadDensityGraph", {steps = GAMESTATE:GetCurrentSteps(), song = GAMESTATE:GetCurrentSong()})
        -- doing this in a really awkward way to inject the desired behavior into the existing SeekBar
        local seekbar = self:GetChild("SeekBar")
        local bg = self:GetChild("BG")
        if seekbar and bg then
            bg:addcommand("HandleRegionSetting", function(self, params)
                local positionGiven = params.positionGiven
                handleRegionSetting(positionGiven)
            end)
        end
        registerActorToCustomizeGameplayUI({
            actor = self,
            coordInc = {5,1},
            sizeInc = {0.1,0.05},
        })
    end,
    SetUpMovableValuesMessageCommand = function(self)
        self:xy(MovableValues.PracticeCDGraphX, MovableValues.PracticeCDGraphY)
        local wb4 = width
        local hb4 = height
        width = MovableValues.PracticeCDGraphWidth * 280
        height = MovableValues.PracticeCDGraphHeight * (53 / 555 * SCREEN_HEIGHT)
        self:playcommand("UpdateSizing", {sizing = {
            Width = width,
            Height = height,
        }})
        if width ~= wb4 or height ~= hb4 then
            self:playcommand("LoadDensityGraph", {steps = GAMESTATE:GetCurrentSteps(), song = GAMESTATE:GetCurrentSong()})
        end
        musicratio = GAMESTATE:GetCurrentSteps():GetLastSecond() / width
        self:finishtweening()
    end,
    PracticeModeReloadMessageCommand = function(self)
        self:playcommand("LoadDensityGraph", {steps = GAMESTATE:GetCurrentSteps(), song = GAMESTATE:GetCurrentSong()})
    end,
}
-- extra quad for bookmark position and region
t[#t+1] = Def.Quad {
    Name = "BookmarkPos",
    InitCommand = function(self)
        -- trickery
        self:valign(0)
        self:zoomto(bookmarkWidth, height)
        self:diffuse(bookmarkColor)
        self:diffusealpha(bookmarkAlpha)
        self:draworder(1100)
        self:visible(false)
    end,
    FirstUpdateCommand = function(self)
        -- have to call this late because the graph is named late (in BeginCommand)
        self:SetFakeParent(self:GetParent():GetChild("PracticeCDGraph"))
    end,
    SetCommand = function(self)
        self:visible(true)
        self:zoomto(bookmarkWidth, height)
        self:diffuse(bookmarkColor)
        self:diffusealpha(bookmarkAlpha)
        self:x(loopStartPos / musicratio)
    end,
    RegionSetMessageCommand = function(self, params)
        if not params or not params.loopLength then
            self:playcommand("Set")
        else
            self:visible(true)
            self:halign(0)
            self:x(loopStartPos / musicratio)
            self:zoomto(params.loopLength / musicratio, height)
            self:diffuse(regionColor)
            self:diffusealpha(regionAlpha)
        end
    end,
    CurrentRateChangedMessageCommand = function(self)
        if not loopEndPos and loopStartPos then
            self:playcommand("Set")
        elseif loopEndPos and loopStartPos then
            self:playcommand("RegionSet", {loopLength = (loopEndPos - loopStartPos)})
        end
    end,
    PracticeModeReloadMessageCommand = function(self)
        self:playcommand("CurrentRateChanged")
    end
}

return t
