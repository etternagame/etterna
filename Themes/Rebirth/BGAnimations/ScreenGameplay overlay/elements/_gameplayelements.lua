-- gameplay elements

setMovableKeymode(getCurrentKeyMode())

local targetTrackerMode = playerConfig:get_data().TargetTrackerMode

local t = Def.ActorFrame {
    Name = "GameplayElementsController",

    BeginCommand = function(self)
        updateDiscordStatus(false)
        updateNowPlaying()

        -- queue so it doesnt reach the children
        self:queuecommand("SetUpMovableValues")
    end,
    EndCommand = function(self)
        -- exiting the screen saves customization changes
        playerConfig:get_data().CurrentWidth = SCREEN_WIDTH
        playerConfig:get_data().CurrentHeight = SCREEN_HEIGHT
        playerConfig:save()
    end,
    SetUpMovableValuesMessageCommand = function(self)
        local screen = SCREENMAN:GetTopScreen()
        local usingReverse = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse()

        -- screen scale
        local screenscale = MovableValues.ScreenZoom
        screen:zoom(screenscale)
        screen:xy(SCREEN_WIDTH * (1 - screenscale) / 2, SCREEN_HEIGHT * (1 - screenscale) / 2)
        screen:addx(MovableValues.ScreenX)
        screen:addy(MovableValues.ScreenY)

        -- lifebar movement
        local lifebar = screen:GetLifeMeter(PLAYER_1)
        if lifebar ~= nil then
            lifebar:zoomtowidth(MovableValues.LifeP1Width)
            lifebar:zoomtoheight(MovableValues.LifeP1Height)
            lifebar:xy(MovableValues.LifeP1X, MovableValues.LifeP1Y)
            lifebar:rotationz(MovableValues.LifeP1Rotation)
        end

        -- notefield movement
        -- notefield column movement
        local nf = screen:GetChild("PlayerP1"):GetChild("NoteField")
        if nf then
            local noteColumns = nf:get_column_actors()
            nf:y(0)
            nf:addy(MovableValues.NoteFieldY * (usingReverse and -1 or 1))
            nf:x(0)
            nf:addx(MovableValues.NoteFieldX)

            -- notefield column sizing
            for i, actor in ipairs(noteColumns) do
                actor:zoomtowidth(MovableValues.NoteFieldWidth)
                actor:zoomtoheight(MovableValues.NoteFieldHeight)
            end
            -- notefield column movement
            local inc = MovableValues.NoteFieldSpacing
            if inc == nil then inc = 0 end
            local hCols = math.floor(#noteColumns/2)
            for i, col in ipairs(noteColumns) do
                col:x(0)
                col:addx((i-hCols-1) * inc)
            end
        end
    end,
    DoneLoadingNextSongMessageCommand = function(self)
        local screen = SCREENMAN:GetTopScreen()

        -- playlists reset notefield positioning ??
        if screen ~= nil and screen:GetChild("PlayerP1") ~= nil then
            NoteField = screen:GetChild("PlayerP1"):GetChild("NoteField")
            NoteField:addy(MovableValues.NoteFieldY * (usingReverse and 1 or -1))
        end
        -- update all stats in gameplay (as if it was a reset) when loading a new song
        -- particularly for playlists
        self:playcommand("PracticeModeReset")
    end,
    JudgmentMessageCommand = function(self, msg)
        -- for each judgment, every tap and hold judge
        local targetDiff = msg.WifeDifferential
        local wifePercent = notShit.floor(msg.WifePercent * 100) / 100
        local judgeCount = msg.Val
        local dvCur = nil
        if msg.Offset ~= nil then
            dvCur = msg.Offset
        end
        local pbTarget = nil
        if msg.WifePBGoal ~= nil and targetTrackerMode ~= 0 then
            pbTarget = msg.WifePBGoal
            targetDiff = msg.WifePBDifferential
        end
        local jdgCur = msg.Judgment

        self:playcommand("SpottedOffset", {
            targetDiff = targetDiff, -- wifepoints difference from target goal
            pbTarget = pbTarget, -- goal target equivalent to current rate pb
            wifePercent = wifePercent, -- visual wifepercent converted from internal wifepercent value
            judgeCount = judgeCount, -- current count of the given judgment that sent the JudgmentMessage
            judgeOffset = dvCur, -- offset assigned to judged tap; nil if is a hold judgment
            judgeCurrent = jdgCur, -- the judgment that triggered this JudgmentMessage
        })
    end,
    PracticeModeResetMessageCommand = function(self)
        -- reset stats for practice mode reverts mostly
        self:playcommand("SpottedOffset", {
            targetDiff = 0,
            pbTarget = 0,
            wifePercent = 0,
            judgeCount = 0,
            judgeOffset = nil,
            judgeCurrent = nil,
        })
    end
}

if playerConfig:get_data().BPMDisplay then
    t[#t+1] = LoadActor("bpmdisplay")
end

if playerConfig:get_data().DisplayEWMA then
    t[#t+1] = LoadActor("displayewma")
end

if playerConfig:get_data().DisplayMean then
    t[#t+1] = LoadActor("displaymean")
end

if playerConfig:get_data().DisplayPercent then
    t[#t+1] = LoadActor("displaypercent")
end

if playerConfig:get_data().DisplayStdDev then
    t[#t+1] = LoadActor("displaystddev")
end

if playerConfig:get_data().ErrorBar ~= 0 then
    t[#t+1] = LoadActor("errorbar")
end

if playerConfig:get_data().FullProgressBar then
    t[#t+1] = LoadActor("fullprogressbar")
end

if playerConfig:get_data().JudgeCounter then
    t[#t+1] = LoadActor("judgecounter")
end

-- lane cover is in Graphics/NoteField cover.lua

if (NSMAN:IsETTP() and Var("LoadingScreen"):find("Net") ~= nil) or
(playerConfig:get_data().Leaderboard == 1 and DLMAN:IsLoggedIn()) or (playerConfig:get_data().Leaderboard == 2) then
    t[#t+1] = LoadActor("leaderboard")
end

if playerConfig:get_data().MeasureCounter then
    t[#t+1] = LoadActor("measurecounter")
end

if playerConfig:get_data().MiniProgressBar then
    t[#t+1] = LoadActor("miniprogressbar")
end

if playerConfig:get_data().NPSDisplay or playerConfig:get_data().NPSGraph then
    t[#t+1] = LoadActor("npsdisplay")
end

if playerConfig:get_data().PlayerInfo then
    t[#t+1] = LoadActor("playerinfo")
end

if playerConfig:get_data().RateDisplay then
    t[#t+1] = LoadActor("ratedisplay")
end

if playerConfig:get_data().TargetTracker then
    t[#t+1] = LoadActor("targettracker")
end

return t
