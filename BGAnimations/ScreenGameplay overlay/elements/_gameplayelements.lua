-- gameplay elements







local function spaceNotefieldCols(inc)
	if inc == nil then inc = 0 end
	local hCols = math.floor(#noteColumns/2)
	for i, col in ipairs(noteColumns) do
	    col:addx((i-hCols-1) * inc)
	end
end

local t = Def.ActorFrame {
    Name = "GameplayElementsController",

    BeginCommand = function(self)
        updateDiscordStatusForGameplay()
        updateNowPlaying()

        local screen = SCREENMAN:GetTopScreen()
        local usingReverse = GAMESTATE:GetPlayerState():GetCurrentPlayerOptions():UsingReverse()

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
            nf:addy(MovableValues.NotefieldY * (usingReverse and 1 or -1))
            nf:addx(MovableValues.NotefieldX)
            local noteColumns = nf:get_column_actors()
            for i, actor in ipairs(noteColumns) do
                actor:zoomtowidth(MovableValues.NotefieldWidth)
                actor:zoomtoheight(MovableValues.NotefieldHeight)
            end
            spaceNotefieldCols(MovableValues.NotefieldSpacing)
        end
    end,
    DoneLoadingNextSongMessageCommand = function(self)
		local screen = SCREENMAN:GetTopScreen()

        -- playlists reset notefield positioning ??
		if screen ~= nil and screen:GetChild("PlayerP1") ~= nil then
			Notefield = screen:GetChild("PlayerP1"):GetChild("NoteField")
			Notefield:addy(MovableValues.NotefieldY * (usingReverse and 1 or -1))
		end
		-- update all stats in gameplay (as if it was a reset) when loading a new song
		-- particularly for playlists
		self:playcommand("PracticeModeReset")
	end,
	JudgmentMessageCommand = function(self, msg)
        -- for each judgment, every tap and hold judge
		tDiff = msg.WifeDifferential
		wifey = notShit.floor(msg.WifePercent * 100) / 100
		jdgct = msg.Val
		if msg.Offset ~= nil then
			dvCur = msg.Offset
		else
			dvCur = nil
		end
		if msg.WifePBGoal ~= nil and targetTrackerMode ~= 0 then
			pbtarget = msg.WifePBGoal
			tDiff = msg.WifePBDifferential
		end
		jdgCur = msg.Judgment
		self:playcommand("SpottedOffset")
	end,
	PracticeModeResetMessageCommand = function(self)
        -- reset stats for practice mode reverts mostly
		tDiff = 0
		wifey = 0
		jdgct = 0
		dvCur = nil
		jdgCur = nil
		self:playcommand("SpottedOffset")
	end
}

if true then
    t[#t+1] = LoadActor("bpmdisplay")
end

if playerConfig:get_data().DisplayPercent then
    t[#t+1] = LoadActor("displaypercent")
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

if playerConfig:get_data().LaneCover then
	t[#t+1] = LoadActor("lanecover")
end

if true then
	t[#t+1] = LoadActor("leaderboard")
end

if playerConfig:get_data().DisplayMean then
	t[#t+1] = LoadActor("meandisplay")
end

if true then
    t[#t+1] = LoadActor("measurecounter")
end

if playerConfig:get_data().MiniProgressBar then
    t[#t+1] = LoadActor("miniprogressbar")
end

if true then
    t[#t+1] = LoadActor("npsdisplay")
end

if playerConfig:get_data().PlayerInfo then
	t[#t+1] = LoadActor("playerinfo")
end

if true then
    t[#t+1] = LoadActor("ratedisplay")
end

if playerConfig:get_data().TargetTracker then
    t[#t+1] = LoadActor("targettracker")
end

return t